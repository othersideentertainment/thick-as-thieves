// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForceDisableCollision.h"

// tat
#include "Collision/Overlay/CollisionOverlayInterface.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_ApplyRootMotionMoveToForceDisableCollision)

UAbilityTask_ApplyRootMotionMoveToForceDisableCollision* UAbilityTask_ApplyRootMotionMoveToForceDisableCollision::ApplyRootMotionMoveToForceDisableCollision(UGameplayAbility* owningAbility, FName taskInstanceName, FVector targetLocation, float duration, bool setNewMovementMode, EMovementMode movementMode, bool restrictSpeedToExpected, UCurveVector* pathOffsetCurve, UCurveFloat* motionLerpCurve, ERootMotionFinishVelocityMode velocityOnFinishMode, FVector setVelocityOnFinish, float clampVelocityOnFinish)
{
   UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(duration);

   UAbilityTask_ApplyRootMotionMoveToForceDisableCollision* myTask = NewAbilityTask<UAbilityTask_ApplyRootMotionMoveToForceDisableCollision>(owningAbility, taskInstanceName);

   myTask->ForceName = taskInstanceName;
   myTask->_targetLocation = targetLocation;
   myTask->_duration = FMath::Max(duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
   myTask->_setNewMovementMode = setNewMovementMode;
   myTask->_newMovementMode = movementMode;
   myTask->_restrictSpeedToExpected = restrictSpeedToExpected;
   myTask->_pathOffsetCurve = pathOffsetCurve;
   myTask->_motionLerpCurve = motionLerpCurve;
   myTask->FinishVelocityMode = velocityOnFinishMode;
   myTask->FinishSetVelocity = setVelocityOnFinish;
   myTask->FinishClampVelocity = clampVelocityOnFinish;
   if (AActor* avatarActor = myTask->GetAvatarActor())
   {
      myTask->_startLocation = avatarActor->GetActorLocation();
   }
   else
   {
      checkf(false, TEXT("UAbilityTask_ApplyRootMotionMoveToForceDisableCollision called without valid avatar actor to get start location from."));
      myTask->_startLocation = targetLocation;
   }
   myTask->SharedInitAndApply();

   return myTask;
}

void UAbilityTask_ApplyRootMotionMoveToForceDisableCollision::SharedInitAndApply()
{
   AActor* avatar = AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get();
   if (avatar)
   {
      _rootComponent = avatar->GetRootComponent();
      if (_rootComponent)
      {
         // changing responses to ignore rather than disabling all colllision, since actually disabling collision breaks root motion
         _rootComponent->AddCollisionOverlay(this, ECR_Ignore);
      }
   }

   Super::SharedInitAndApply();
}

void UAbilityTask_ApplyRootMotionMoveToForceDisableCollision::OnDestroy(bool abilityIsEnding)
{
   if (_rootComponent)
   {
      _rootComponent->RemoveCollisionOverlayByKey(this);
   }

   Super::OnDestroy(abilityIsEnding);
}

