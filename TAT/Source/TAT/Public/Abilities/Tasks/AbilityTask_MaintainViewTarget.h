// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolFunctionLibrary.h"

// ue
#include "Abilities/Tasks/AbilityTask.h"

#include "AbilityTask_MaintainViewTarget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTATMaintainViewTargetDelegate, bool, hasLineOfSightToTarget, float, targetAngleDeg, float, targetDistance, float, elapsedTimeSeconds);

UCLASS()
class TAT_API UTATAbilityTask_MaintainViewTarget : public UAbilityTask
{
   GENERATED_BODY()

   UPROPERTY(BlueprintAssignable)
   FTATMaintainViewTargetDelegate OnViewTargetUpdate;

   UPROPERTY(BlueprintAssignable)
   FTATMaintainViewTargetDelegate OnMaxDurationReached;

   /// Periodically check the avatar's view target with a specific actor.
   /// Fires the view update event whenever the avatar's line-of-sight with the target changes, the view angle changes, or the distance to the changes.
   /// For view angle and distance, only changes larger than their thresholds will trigger event callbacks.
   /// If MaxDurationSeconds is greater than zero, this will stop ticking and fire the OnMaxDurationReached event after that number of seconds.
   /// NB. OnMaxDurationReached will NOT be fired if maxDurationSeconds is less than or equal to zero.
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|TAT", meta = (HidePin = "owningAbility", DefaultToSelf = "owningAbility", BlueprintInternalUseOnly = "true"))
   static UTATAbilityTask_MaintainViewTarget* MaintainViewTarget(UGameplayAbility* owningAbility, AActor* targetActor,
      const FTATLineOfSightTraceParams& traceParams, float pollInterval, float angleDegThreshold = 1.0f, float distanceThreshold = 100.0f, float maxDurationSeconds = 0.0f, bool drawDebug = false);

   virtual void Activate() override;

protected:
   virtual void OnDestroy(bool abilityIsEnding) override;

   void _OnMaxDurationReached(bool hasLineOfSightToTarget, float targetAngleDeg, float targetDistance, float elapsedTimeSeconds);

   void _Update();

protected:
   UPROPERTY(Transient)
   AActor* _targetActor = nullptr;

   FTATLineOfSightTraceParams _traceParams;
   float _pollInterval = 0.1f;
   float _angleDegThreshold = 1.0f;
   float _distanceThreshold = 100.0f;
   float _maxDurationSeconds = 0.0f;
   bool _drawDebug = false;

   FTimerHandle _timerHandle;

   double _startTimeSeconds = -1.0;
   bool _isFirstUpdate = true;
   float _prevAngleDeg = 0.0f;
   float _prevDistance = 0.0f;
   bool _prevHasLineOfSight = false;
};
