// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"

// ue
#include "Kismet/BlueprintAsyncActionBase.h"

#include "AsyncTaskInterpolate.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOSEAsyncInterpolateDelegate, float, value, bool, forward);

UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class OSECORE_API UOSEAsyncTaskInterpolate : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintAssignable)
   FOSEAsyncInterpolateDelegate OnUpdate;

   UPROPERTY(BlueprintAssignable)
   FOSEAsyncInterpolateDelegate OnCancel;

   UPROPERTY(BlueprintAssignable)
   FOSEAsyncInterpolateDelegate OnComplete;

public:
   /// Interpolates a value over time, with controls to reverse, cancel, or change the play rate during execution.
   /// Similar to a timeline but usable in non-actor contexts such as UMG widget blueprints
   /// @param durationSeconds Number of seconds that one iteration takes to interpolate from valueStart to valueEnd
   /// @param speedMultiplier Multiplier to apply to the rate at which the value changes over time.
   /// @param forward If enabled, the first iteration will interpolate from valueStart to valueEnd. If disabled we go from valueEnd to valueStart.
   /// @param valueStart Output value on the first update event of a loop iteration
   /// @param valueEnd Output value on the last update event of a loop iteration
   /// @param valueInterpMode Interpolation curve to apply to the output value
   UFUNCTION(BlueprintCallable, DisplayName = "Async Interpolate Value", Category = "OSE", meta = (BlueprintInternalUseOnly = "true", DefaultToSelf = "owner"))
   static UOSEAsyncTaskInterpolate* SpawnAsyncTaskInterpolate(UObject* owner, float durationSeconds = 1.0f, float speedMultiplier = 1.0f, bool forward = true,
      float valueStart = 0.0f, float valueEnd = 1.0f, EOSEInterpMode valueInterpMode = EOSEInterpMode::Linear);

   UFUNCTION(BlueprintCallable, Meta = (CompactNodeTitle = "Reverse"))
   void Reverse();

   UFUNCTION(BlueprintCallable, Meta = (CompactNodeTitle = "Cancel"))
   void CancelTask();

   UFUNCTION(BlueprintPure, Meta = (CompactNodeTitle = "Value"))
   float GetCurrentValue() const;

   UFUNCTION(BlueprintCallable)
   void SetCurrentValue(float newValue);

   UFUNCTION(BlueprintPure, Meta = (CompactNodeTitle = "InterpMode"))
   inline EOSEInterpMode GetInterpMode() const { return _valueInterpMode; }

   UFUNCTION(BlueprintCallable)
   void SetInterpMode(EOSEInterpMode newInterpMode);

   UFUNCTION(BlueprintCallable)
   void SetSpeedMultiplier(float newSpeedMultiplier);

   UFUNCTION(BlueprintPure)
   float GetSpeedMultiplier() const { return _speedMultiplier; }

private:
   double _GetTimeSeconds() const;
   void _CleanupAndDestroyTask();

   void _Init();
   bool _Update(float tickerDeltaTime);

private:
   UPROPERTY(Transient)
   TObjectPtr<UObject> _owner = nullptr;

   float _durationSeconds = 1.0f;
   float _speedMultiplier = 1.0f;
   bool _forward = true;
   float _valueStart = 0.0f;
   float _valueEnd = 1.0f;
   EOSEInterpMode _valueInterpMode = EOSEInterpMode::Linear;

   bool _didCancel = false;
   double _lastUpdateTimeSeconds = 0.0;
   float _currentNormalizedValue = 0.0f;

   FTSTicker::FDelegateHandle _updateTimerHandle;
};
