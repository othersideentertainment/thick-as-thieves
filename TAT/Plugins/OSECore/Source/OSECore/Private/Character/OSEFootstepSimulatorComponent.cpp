// (c) 2202 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Character/OSEFootstepSimulatorComponent.h"

//ue4
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEFootstepSimulatorComponent)

DEFINE_LOG_CATEGORY_STATIC(OSEFootstepSimulator, Log, Log);

UOSEFootstepSimulatorComponent::UOSEFootstepSimulatorComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UOSEFootstepSimulatorComponent::BeginPlay()
{
   Super::BeginPlay();
   
   if (ACharacter* ownerChar = CastChecked<ACharacter>(GetOwner()))
   {
      _characterMovement = ownerChar->GetCharacterMovement();
      ownerChar->OnCharacterMovementUpdated.AddUniqueDynamic(this, &ThisClass::_OnMovementUpdated);
      check(IsValid(_characterMovement));
   }
}

void UOSEFootstepSimulatorComponent::_OnMovementUpdated(float deltaSeconds, FVector oldLocation, FVector oldVelocity)
{
   
   if (!IsValid(_characterMovement))
      return;

   if (_characterMovement->IsMovingOnGround())
   {
      if (_characterMovement->bJustTeleported)
      {
         // We've just teleported, ignore this frame.
         return;
      }
      FVector moveVector = _characterMovement->GetActorLocation() - oldLocation;
      float moveDistance = moveVector.Size();
            
      if (FMath::IsNearlyZero(moveDistance) ||
         (!EmitFootstepsForVerticalOnlyMovement && FVector2D(moveVector).SizeSquared() < 1))
      {
         _StoppedMoving();
         return;
      }

      for (FFootstepSimulatorStride& stride : Strides)
      {
         stride.TravelDistance += moveDistance;

         const float stepLength = stride.TravelDistance - stride.LastStep;

         if (stepLength > stride.StrideLength)
         {
            //A step has occured.
            OnStep.Broadcast(stride.StrideTag);
            UE_LOG(OSEFootstepSimulator, Verbose, TEXT("Trigger step %s:%s with move distance of %f"), *GetOwner()->GetName(), *stride.StrideTag.ToString(), moveDistance);

            //Set the last footstep to the next step location.
            stride.LastStep += stride.StrideLength;
         }
      }
   }
   else
   {
      _StoppedMoving();
   }
}

void UOSEFootstepSimulatorComponent::_StoppedMoving()
{
   //When we stop moving set the steps back to zero so when we start moving we have to take a full stride before firing a step.
   for (FFootstepSimulatorStride& stride : Strides)
   {
      if (stride.TravelDistance != 0 && stride.StepOnClose)
      {
         OnStep.Broadcast(stride.StrideTag);
      }

      stride.TravelDistance = 0;
      stride.LastStep = 0;
   }
   UE_LOG(OSEFootstepSimulator, Verbose, TEXT("Stopped moving %s"), *GetOwner()->GetName());
}

void UOSEFootstepSimulatorComponent::ReceiveAnimationFootstep(const FGameplayTag& strideTag)
{
   // Audio hooks into this, so we need to broadcast this event, but none of the following logic if ShouldIgnoreAnimNotifies is true.
   // Otherwise the simulated step stims will be resetting constantly / triggering based on animations that aren't playing on the server
   OnAnimNotifyFootstep.Broadcast(strideTag);
   
   if (_ShouldIgnoreAnimNotifies)
      return;
   
   FFootstepSimulatorStride* stride = Strides.FindByPredicate([=](const FFootstepSimulatorStride& strideToCheck) { return strideToCheck.StrideTag == strideTag; });
   
   if (!stride)
   {
      UE_LOG(OSEFootstepSimulator, Verbose, TEXT("Footstep Component on %s received an animation footstep for %s without a matching stride"), *GetOwner()->GetName(), *strideTag.ToString());
      return;
   }

   stride->TravelDistance = 0;
   stride->LastStep = 0;
   OnStep.Broadcast(strideTag);

   UE_LOG(OSEFootstepSimulator, Verbose, TEXT("Animated Footstep %s:%s"), *GetOwner()->GetName(), *strideTag.ToString());
}

void UOSEFootstepSimulatorComponent::SetStrideLength(const FGameplayTag& strideTag, float length)
{
   if (length <= 0)
   {
      UE_LOG(OSEFootstepSimulator, Error, TEXT("Stride Length on %s:%s must be greater than 0."), *GetOwner()->GetName(), *strideTag.ToString());
      return;
   }

   FFootstepSimulatorStride* stride = Strides.FindByPredicate([=](const FFootstepSimulatorStride& strideToCheck) { return strideToCheck.StrideTag == strideTag; });

   if (!stride)
   {
      UE_LOG(OSEFootstepSimulator, Verbose, TEXT("Trying to set Stride Length on %s:%s without a matching stride entry"), *GetOwner()->GetName(), *strideTag.ToString());
      return;
   }

   stride->StrideLength = length;
}


