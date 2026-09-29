// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionOSEMoveToForce.h"

#include "AbilityTask_ApplyRootMotionMoveToForceDisableCollision.generated.h"

class ICollisionOverlayInterface;

// This is kept at the TAT-level, because the current assumptions about when it is safe to change collision responses are not game-agnostic
UCLASS()
class TAT_API UAbilityTask_ApplyRootMotionMoveToForceDisableCollision : public UAbilityTask_ApplyRootMotionOSEMoveToForce
{
   GENERATED_BODY()

   // Apply force to character's movement, disabling collision during travel
   // NOTE: Restoring the collision response assumes that it has not changed during the travel.
   //       The only thing that touches this currently is OnLyingDownChanged.
   //       Ensure that characters immune to any effects that would cause that to change during travel, but this may be unsound if that changes
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_ApplyRootMotionMoveToForceDisableCollision* ApplyRootMotionMoveToForceDisableCollision(UGameplayAbility* owningAbility, FName taskInstanceName, FVector targetLocation, float duration, bool setNewMovementMode, EMovementMode movementMode, bool restrictSpeedToExpected, UCurveVector* pathOffsetCurve, UCurveFloat* motionLerpCurve, ERootMotionFinishVelocityMode velocityOnFinishMode, FVector setVelocityOnFinish, float clampVelocityOnFinish);

   virtual void OnDestroy(bool abilityIsEnding) override;

protected:

   virtual void SharedInitAndApply() override;

protected:

   UPROPERTY()
   TScriptInterface<ICollisionOverlayInterface> _rootComponent;
};
