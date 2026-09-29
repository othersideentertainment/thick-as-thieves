// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Security/TATPatrollingSecurityCamera.h"

// ose

#include "Components/SphereComponent.h"
#include "GameFramework/TATFlyingPatrolFollowingMovementComponent.h"
#include "Interactables/TATInteractHighlightUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPatrollingSecurityCamera)

ATATPatrollingSecurityCamera::ATATPatrollingSecurityCamera() : Super()
{
   _movementComponent = CreateDefaultSubobject<UTATFlyingPatrolFollowingMovementComponent>(TEXT("MovementComponent"));
   _colliderComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Root"));
   RootComponent = _colliderComponent;
   _interpolatedVisualsComponent = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
   _interpolatedVisualsComponent->SetupAttachment(RootComponent);
   SetReplicateMovement(true);
}

void ATATPatrollingSecurityCamera::BeginPlay()
{
   Super::BeginPlay();
   if(_movementComponent && _interpolatedVisualsComponent)
   {
      _movementComponent->SetInterpolatedComponent(_interpolatedVisualsComponent);
   }
}

void ATATPatrollingSecurityCamera::SetOn(bool newOn)
{
   Super::SetOn(newOn);
   _movementComponent->SetComponentTickEnabled(newOn);
}

void ATATPatrollingSecurityCamera::_OnDeviceStateChanged_Implementation(ETATVisionPerceptionDeviceState deviceState,
                                                              ETATVisionPerceptionDeviceState oldDeviceState)
{
   Super::_OnDeviceStateChanged_Implementation(deviceState, oldDeviceState);
   if(deviceState == ETATVisionPerceptionDeviceState::Triggered)
   {
      _movementComponent->SetComponentTickEnabled(false);
      return;
   }
   if(oldDeviceState == ETATVisionPerceptionDeviceState::Triggered)
   {
      if(_movementResetHandle.IsValid())
      {
         GetWorld()->GetTimerManager().ClearTimer(_movementResetHandle);
      }
      GetWorld()->GetTimerManager().SetTimer(_movementResetHandle, this, &ThisClass::_OnMovementResetTimerExpired,
                                             DelayBeforeMovingAfterDetectingPlayer);
   }
}

bool ATATPatrollingSecurityCamera::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return true;
}

void ATATPatrollingSecurityCamera::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = State.bIsOn ? TurnOffPrompt : TurnOnPrompt;
}

FInteractStartResult ATATPatrollingSecurityCamera::StartInteract_Implementation(ACharacter* InteractingCharacter)
{
   SetOn(!State.bIsOn);
   FInteractStartResult result;
   result.InstantAnimationTag = IsOn() ? TurnOnAnimationTag : TurnOffAnimationTag;
   return result;
}

void ATATPatrollingSecurityCamera::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

void ATATPatrollingSecurityCamera::PostNetReceiveLocationAndRotation()
{
   if (_movementComponent && _movementComponent->UpdatedComponent)
   {
      const FRepMovement& constRepMovement = GetReplicatedMovement();
      const FVector newLocation = FRepMovement::RebaseOntoLocalOrigin(constRepMovement.Location, this);
      _movementComponent->MoveInterpolationTarget(newLocation, constRepMovement.Rotation);
   }
   else
   {
      Super::PostNetReceiveLocationAndRotation();
   }
}

void ATATPatrollingSecurityCamera::_OnMovementResetTimerExpired() const
{
   _movementComponent->SetComponentTickEnabled(IsOn());
}

