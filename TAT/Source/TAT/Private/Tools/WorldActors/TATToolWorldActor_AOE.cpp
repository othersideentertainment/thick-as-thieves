// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_AOE.h"

// tat
#include "Developer/TATToolSettings.h"
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolSetComponent.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/ShapeComponent.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_AOE)


DEFINE_LOG_CATEGORY_STATIC(LogTATToolWorldActor_AOE, Log, All)

ATATToolWorldActor_AOE::ATATToolWorldActor_AOE()
{
   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));
   _collisionTrackerComponent->OnlyRunOnAuthority = true;
   _collisionTrackerComponent->AutomaticallyFindShapeComponent = false;
}

void ATATToolWorldActor_AOE::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATToolWorldActor_AOE, _triggeredTimestamp);
}

#if WITH_EDITOR
EDataValidationResult ATATToolWorldActor_AOE::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract) && TriggerTargetCriteria.IsEmpty())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("TATToolWorldActor_AOE '%s' has an empty TriggerTargetCriteria"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif

void ATATToolWorldActor_AOE::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      if (SecondsAfterSpawnToActivate == 0.0f)
      {
         _AuthorityActivateAOE();
      }
      else
      {
         FTimerHandle handle;
         GetWorld()->GetTimerManager().SetTimer(handle, this, &ATATToolWorldActor_AOE::_AuthorityActivateAOE, SecondsAfterSpawnToActivate);
      }

      _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ATATToolWorldActor_AOE::_AuthorityOnActorEnterShape);
      _collisionTrackerComponent->OnActorExitedShape.AddUniqueDynamic(this, &ATATToolWorldActor_AOE::_AuthorityOnActorExitShape);
   }
}

void ATATToolWorldActor_AOE::EndPlay(EEndPlayReason::Type reason)
{
   _actorsWithEffectApplied.CancelAll();

   Super::EndPlay(reason);
}

void ATATToolWorldActor_AOE::_AuthorityActivateAOE()
{
   // NB: SetShapeComponents takes a default-allocator TArray explicitly, so no inline allocator
   TArray<UShapeComponent*> shapeComps;
   GetComponents(shapeComps);

   if (shapeComps.Num() > 0)
   {
      _collisionTrackerComponent->SetShapeComponents(shapeComps);
      // NB: If we already are overlapping an actor, make sure we start tracking it
      _collisionTrackerComponent->RefreshInitialOverlaps();
   }
   else
   {
      UE_LOG(LogTATToolWorldActor_AOE, Warning,
         TEXT("ATATToolWorldActor_AOE '%s' did not find any shape components to use for its collision tracking"),
         *GetName());
   }
}

void ATATToolWorldActor_AOE::_AuthorityOnActorEnterShape(AActor* actor)
{
   check(HasAuthority());

   // Ignore our instigator
   if (IgnoreInstigator && actor == GetInstigator())
   {
      return;
   }

   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      FGameplayTagContainer tagContainer;
      asc->GetOwnedGameplayTags(tagContainer);

      if (TriggerTargetCriteria.Matches(tagContainer))
      {
         // Add any gameplay effects that are added to actors while they're in our AoE
         for (TSubclassOf<UGameplayEffect> EffectToApply : EffectsToApply)
         {
            FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
            effectContext.AddInstigator(this, this);
            FActiveGameplayEffectHandle effectHandle = asc->ApplyGameplayEffectToSelf(EffectToApply.GetDefaultObject(), 0.0f, effectContext);
            _actorsWithEffectApplied.Add(actor, effectHandle);
         }

         // BP callback for additional functionality
         BP_Authority_OnActorEnterShape(actor);

         // If this is the first time we're triggered, set the timestamp to let clients know
         if (_triggeredTimestamp < 0.0f)
         {
            const bool justHappened = true;
            BP_OnFirstActivated(justHappened);

            _triggeredTimestamp = UOSEInteractionHelpers::GetServerTimeForWrite(GetWorld());
         }
      }
   }
}

void ATATToolWorldActor_AOE::_AuthorityOnActorExitShape(AActor* actor)
{
   check(HasAuthority());

   // N.B. We don't need to check TriggerTargetCriteria or IgnoreInstigator here, since
   // if actor is not in _actorsWithEffectApplied, this is a no-op
   _actorsWithEffectApplied.CancelByActor(actor);
}

void ATATToolWorldActor_AOE::_OnRep_TriggeredTimestamp()
{
   if (_triggeredTimestamp >= 0.0f)
   {
      // It's possible due to net relevancy that we don't hear about the change for some time
      // In that case, we'd still want any state changes to occur, but transient VFX would know to not play
      bool justHappened = !UOSEInteractionHelpers::IsOld(GetWorld(), _triggeredTimestamp);
      BP_OnFirstActivated(justHappened);
   }
}

