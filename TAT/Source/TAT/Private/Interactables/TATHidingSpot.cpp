// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATHidingSpot.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Loot/TATLootInventory.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"
#include "OSEProjectSettings.h"

// ue5
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHidingSpot)

// Sets default values
ATATHidingSpot::ATATHidingSpot()
{
	PrimaryActorTick.bCanEverTick = false;

   bReplicates = true;
   NetDormancy = DORM_Initial;

   _rootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
   _rootComponent->Mobility = EComponentMobility::Static;
   RootComponent = _rootComponent;

   _exitCapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ExitCapsule"));
   _exitCapsuleComponent->ShapeColor = FColor(255, 138, 5, 255);
   _exitCapsuleComponent->bDrawOnlyIfSelected = true;
   _exitCapsuleComponent->InitCapsuleSize(40.0f, 100.0f);
   _exitCapsuleComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
   _exitCapsuleComponent->bShouldCollideWhenPlacing = true;
   _exitCapsuleComponent->Mobility = EComponentMobility::Static;
   _exitCapsuleComponent->SetRelativeLocation(FVector(70, 0, 100.f));
   _exitCapsuleComponent->SetupAttachment(RootComponent);
}

void ATATHidingSpot::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATHidingSpot, _state);
}

bool ATATHidingSpot::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   IGameplayTagAssetInterface* tagSource = Cast<IGameplayTagAssetInterface>(interactingCharacter);
   if (tagSource == nullptr)
   {
      return false;
   }

   if (!tagSource->HasAllMatchingGameplayTags(_requiredEnterTags) || tagSource->HasAnyMatchingGameplayTags(_blockedEnterTags))
   {
      return false;
   }

   return true;
}

void ATATHidingSpot::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = _hideInteractPrompt;
}

FInteractStartResult ATATHidingSpot::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (_GetHidingCharacter())
   {
      FInteractStartResult result;
      result.Message = _hideFullMessage;
      return result;
   }

   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter);
   if (asc && (asc->IsOwnerActorAuthoritative() || asc->CanPredict()))
   {
      if (HasAuthority())
      {
         _SetHidingCharacter(interactingCharacter);
         UTATAISense_Hearing::ReportNoiseEvent(this, _enterHearingStim, interactingCharacter->GetActorLocation(), this);
      }

      // add gameplay effect (this should be predicted, since interaction has a valid prediction scope)
      // 
      // Doing this via an effect rather than an ability (for now, at least) so that targets do not have
      // to have an ability for this specific case already, and it can still be locally predicted.
      // (the effect grants abilities, but it does not do the transition. This does mean that some of
      // the transition occurs here (see below).
      const TSubclassOf<UGameplayEffect> effectClass = _hidingEffect;
      FGameplayEffectContextHandle context = asc->MakeEffectContext();
      context.AddInstigator(this, this);
      context.AddOrigin(interactingCharacter->GetActorLocation());
      FPredictionKey predictionKey = asc->GetPredictionKeyForNewAction();
      FActiveGameplayEffectHandle effectHandle = asc->ApplyGameplayEffectToSelf(effectClass.GetDefaultObject(), 0, context, predictionKey);
      _appliedEffects.Add(interactingCharacter, effectHandle);

      // Force view target, so it can be predicted
      if (APlayerController* pc = interactingCharacter->GetController<APlayerController>())
      {
         pc->SetViewTarget(GetIntendedViewTarget(), _enterCameraTransition);
      }

      // position character at exit location first, so there is no movement on exit
      interactingCharacter->SetActorLocation(_FindExitLocationForCharacter(interactingCharacter), false, nullptr, ETeleportType::ResetPhysics);

      if (!asc->IsOwnerActorAuthoritative())
      {
         predictionKey.NewRejectOrCaughtUpDelegate(FPredictionKeyEvent::CreateUObject(this, &ATATHidingSpot::_OnPredictiveHideCatchup, MakeWeakObjectPtr(interactingCharacter)));
      }
   }

   return FInteractStartResult();
}

void ATATHidingSpot::TryFinishExit(ACharacter* exitingCharacter)
{
   // Tolerating calls from non-hiding character to allow for (not-yet-tested) predicted rollback

   if (APlayerController* pc = exitingCharacter->GetController<APlayerController>())
   {
      if (const UCameraComponent* camera = _FindFirstActiveCamera())
      {
         FVector cameraForward = camera->GetForwardVector();
         cameraForward.Z = 0;

         // Keep facing the hiding spot if KOed, since it looks bad in third person
         if (_ShouldFaceHidingSpotOnExit(exitingCharacter))
         {
            cameraForward *= -1;
         }

         const FRotator newRotation = FRotationMatrix::MakeFromX(cameraForward).Rotator();

         // Set control/pawn rotation to match the (2d) rotation of the camera, so the transition is fluid (and not confusing) 
         pc->SetControlRotation(newRotation);
         exitingCharacter->SetActorRotation(newRotation, ETeleportType::ResetPhysics);
      }

      // Restore view target
      if(pc->GetViewTarget() == this || pc->GetViewTarget() == GetIntendedViewTarget())
      {
         pc->SetViewTarget(exitingCharacter, _exitCameraTransition);
      }
   }

   if (HasAuthority() && _GetHidingCharacter() == exitingCharacter)
   {
      UTATAISense_Hearing::ReportNoiseEvent(this, _exitHearingStim, exitingCharacter->GetActorLocation(), exitingCharacter);
      _SetHidingCharacter(nullptr);
   }

   _appliedEffects.CancelByActor(exitingCharacter);
}

#if WITH_EDITOR
void ATATHidingSpot::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      // Check for collision slightly inset
      const FCollisionShape capsuleShape = FCollisionShape::MakeCapsule(_exitCapsuleComponent->GetScaledCapsuleRadius() - 5, _exitCapsuleComponent->GetScaledCapsuleHalfHeight() - 5);
      if(GetWorld()->OverlapBlockingTestByChannel(_exitCapsuleComponent->GetComponentLocation(), FQuat::Identity, ECC_Pawn, capsuleShape, FCollisionQueryParams(SCENE_QUERY_STAT(ATATHidingSpot_CheckForErrors), false, this)))
      {
         FMessageLog("MapCheck").Error()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("Hiding spot %s has an exit point occluded by geometry"), *GetActorLabel()))));
      }
   }
}
#endif

void ATATHidingSpot::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

void ATATHidingSpot::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (endPlayReason == EEndPlayReason::Destroyed)
   {
      _appliedEffects.CancelAll();
   }

   Super::EndPlay(endPlayReason);
}

const class UCameraComponent* ATATHidingSpot::_FindFirstActiveCamera() const
{
   TInlineComponentArray<UCameraComponent*> cameras;
   GetComponents(cameras);

   for (UCameraComponent* cameraComponent : cameras)
   {
      if (cameraComponent->IsActive())
      {
         return cameraComponent;
      }
   }

   return nullptr;
}

void ATATHidingSpot::_SetHidingCharacter(ACharacter* hidingCharacter)
{
   check(HasAuthority());
   FlushNetDormancy();

   const FTATHidingSpotState oldState = _state;
   _state.HidingCharacter = hidingCharacter;
   _state.ChangedServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   _OnRep_State(oldState);
}

void ATATHidingSpot::_OnPredictiveHideCatchup(TWeakObjectPtr<ACharacter> weakHidingCharacter)
{
   ACharacter* hidingCharacter = weakHidingCharacter.Get();
   if (hidingCharacter == nullptr) return;

   UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(hidingCharacter);
   if (asc == nullptr) return;

   // NOTE: it is important that this rollback check against state on the ASC (or the same actor),
   //       as there are no strong guarantees about inter-actor replication ordering
   // taking a slight shortcut with tag to avoid having to scrape for active effects
   if(!asc->HasMatchingGameplayTag(_hidingTag))
   {
      TryFinishExit(hidingCharacter);
   }
}

FVector ATATHidingSpot::_FindExitLocationForCharacter(const ACharacter* hidingCharacter) const
{
   const float heightDelta = hidingCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - _exitCapsuleComponent->GetScaledCapsuleHalfHeight();
   return _exitCapsuleComponent->GetComponentLocation() + FVector(0, 0, heightDelta);
}

bool ATATHidingSpot::_ShouldFaceHidingSpotOnExit(const ACharacter* hidingCharacter) const
{
   const IGameplayTagAssetInterface* tagSource = Cast<IGameplayTagAssetInterface>(hidingCharacter);
   if (tagSource == nullptr)
   {
      return false;
   }
   
   // Keep facing the hiding spot if KOed, since it looks bad in third person
   return tagSource->HasMatchingGameplayTag(UOSEProjectSettings::Get().ConditionUnconsciousTag);
}

void ATATHidingSpot::_OnRep_State(const FTATHidingSpotState& oldState)
{
   if (oldState.HidingCharacter != _state.HidingCharacter)
   {
      if (!UOSEInteractionHelpers::IsOld(this, _state.ChangedServerTime))
      {
         BP_OnHidingCharacterRecentlyChanged(_state.HidingCharacter != nullptr);
      }
   }
}

