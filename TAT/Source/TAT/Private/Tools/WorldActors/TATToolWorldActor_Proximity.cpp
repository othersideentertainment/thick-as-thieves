// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_Proximity.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_Proximity)


ATATToolWorldActor_Proximity::ATATToolWorldActor_Proximity()
{
   NetDormancy = DORM_DormantAll;

   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));
   _collisionTrackerComponent->OnlyRunOnAuthority = true;
}

void ATATToolWorldActor_Proximity::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATToolWorldActor_Proximity, _triggeredTimestamp);
}

#if WITH_EDITOR
EDataValidationResult ATATToolWorldActor_Proximity::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract) && TriggerTargetCriteria.IsEmpty())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("TATToolWorldActor_Proximity '%s' has an empty TriggerTargetCriteria"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif

void ATATToolWorldActor_Proximity::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ATATToolWorldActor_Proximity::_AuthorityOnActorEnterShape);
   }
}

void ATATToolWorldActor_Proximity::_OnAuthorityTriggeredBy(AActor* actor)
{
   if (_triggerHearingStim.IsValid())
   {
      UTATAISense_Hearing::ReportNoiseEvent(this, _triggerHearingStim, GetActorLocation(), this);
   }
}

void ATATToolWorldActor_Proximity::_AuthorityOnActorEnterShape(AActor* actor)
{
   check(HasAuthority());

   // Ignore our instigator
   if (IgnoreInstigator && actor == GetInstigator())
   {
      return;
   }

   // If we've already been triggered, ignore further cases
   if (_triggeredTimestamp >= 0.0f)
   {
      return;
   }

   // Check if the target has the necessary tags
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      FGameplayTagContainer tagContainer;
      asc->GetOwnedGameplayTags(tagContainer);

      if (TriggerTargetCriteria.Matches(tagContainer))
      {
         _OnAuthorityTriggeredBy(actor);
         OnAuthorityTriggeredBy(actor);

         const bool justHappened = true;
         OnTriggered(justHappened);

         FlushNetDormancy();
         _triggeredTimestamp = UOSEInteractionHelpers::GetServerTimeForWrite(GetWorld());

         // Stop tracking overlaps since we've triggered
         _collisionTrackerComponent->OnActorEnteredShape.RemoveAll(this);
         _collisionTrackerComponent->ClearShapeTracking();
      }
   }
}

void ATATToolWorldActor_Proximity::_OnRep_TriggeredTimestamp()
{
   if (_triggeredTimestamp >= 0.0f)
   {
      // It's possible due to net relevancy that we don't hear about the change for some time
      // In that case, we'd still want any state changes to occur, but transient VFX would know to not play
      bool justHappened = !UOSEInteractionHelpers::IsOld(GetWorld(), _triggeredTimestamp);
      OnTriggered(justHappened);
   }
}



