// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Utl/AsyncTaskInterpolate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskInterpolate)

DEFINE_LOG_CATEGORY_STATIC(LogAsyncTaskInterpolate, Log, All);

// static
UOSEAsyncTaskInterpolate* UOSEAsyncTaskInterpolate::SpawnAsyncTaskInterpolate(UObject* owner, float durationSeconds, float speedMultiplier,
   bool forward, float valueStart, float valueEnd, EOSEInterpMode valueInterpMode)
{
   if (!IsValid(owner))
   {
      UE_LOG(LogAsyncTaskInterpolate, Error, TEXT("SpawnAsyncTaskInterpolate was passed a null owner!"));
      return nullptr;
   }

   UWorld* world = owner->GetWorld();
   if (world == nullptr)
   {
      UE_LOG(LogAsyncTaskInterpolate, Error, TEXT("SpawnAsyncTaskInterpolate can't be used in blueprint graphs without a valid world context"));
      return nullptr;
   }

   UOSEAsyncTaskInterpolate* task = NewObject<UOSEAsyncTaskInterpolate>();
   task->_owner = owner;
   task->_durationSeconds = durationSeconds;
   task->_speedMultiplier = speedMultiplier;
   task->_forward = forward;
   task->_valueStart = valueStart;
   task->_valueEnd = valueEnd;
   task->_valueInterpMode = valueInterpMode;

   task->_Init();

   return task;
}

void UOSEAsyncTaskInterpolate::Reverse()
{
   _forward = !_forward;
}

void UOSEAsyncTaskInterpolate::CancelTask()
{
   if (_didCancel)
   {
      return;
   }
   _didCancel = true;
   OnCancel.Broadcast(GetCurrentValue(), _forward);
   _CleanupAndDestroyTask();
}

float UOSEAsyncTaskInterpolate::GetCurrentValue() const
{
   return UOSEMathFunctionLibrary::Interpolate(_valueStart, _valueEnd, _currentNormalizedValue, _valueInterpMode);
}

void UOSEAsyncTaskInterpolate::SetCurrentValue(float newValue)
{
   _currentNormalizedValue = FMath::GetMappedRangeValueClamped(FVector2f(_valueStart, _valueEnd), FVector2f(0.0f, 1.0f), newValue);
}

void UOSEAsyncTaskInterpolate::SetInterpMode(EOSEInterpMode newInterpMode)
{
   _valueInterpMode = newInterpMode;
}

void UOSEAsyncTaskInterpolate::SetSpeedMultiplier(float newSpeedMultiplier)
{
   _speedMultiplier = FMath::Max(0.0f, newSpeedMultiplier);
}

double UOSEAsyncTaskInterpolate::_GetTimeSeconds() const
{
   if (ensure(IsValid(_owner)))
   {
      return _owner->GetWorld()->GetTimeSeconds();
   }
   return 0.0;
}

void UOSEAsyncTaskInterpolate::_CleanupAndDestroyTask()
{
   if (_updateTimerHandle.IsValid())
   {
      FTSTicker::GetCoreTicker().RemoveTicker(_updateTimerHandle);
      _updateTimerHandle.Reset();
   }

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UOSEAsyncTaskInterpolate::_Init()
{
   check(_owner != nullptr);
   UWorld* world = _owner->GetWorld();
   check(world != nullptr);

   // Behave sanely if we have no duration - just immediately call OnUpdate followed by OnComplete
   if (_durationSeconds <= 0)
   {
      const float value = _forward ? _valueEnd : _valueStart;
      OnUpdate.Broadcast(value, _forward);
      OnComplete.Broadcast(value, _forward);
      return;
   }

   _lastUpdateTimeSeconds = world->GetTimeSeconds();
   _currentNormalizedValue = _forward ? 0.0f : 1.0f;

   _Update(0.0f);

   _updateTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UOSEAsyncTaskInterpolate::_Update));
}

bool UOSEAsyncTaskInterpolate::_Update(float tickerDeltaTime)
{
   check(_owner != nullptr);

   const double updateStartTimeSeconds = _GetTimeSeconds();

   const double timeSinceLastUpdate = FMath::Max(0.0, updateStartTimeSeconds - _lastUpdateTimeSeconds);
   const double normalizedDeltaValue = FMath::Max(0.0, (timeSinceLastUpdate / _durationSeconds) * _speedMultiplier);
   const float directionMultiplier = _forward ? 1.0f : -1.0f;
   _currentNormalizedValue = FMath::Clamp(_currentNormalizedValue + (normalizedDeltaValue * directionMultiplier), 0.0f, 1.0f);

   OnUpdate.Broadcast(GetCurrentValue(), _forward);

   auto isTaskComplete = [this]() -> bool
   {
      return (_forward && _currentNormalizedValue >= 1) || (!_forward && _currentNormalizedValue <= 0);
   };

   if (isTaskComplete())
   {
      OnComplete.Broadcast(GetCurrentValue(), _forward);

      // Check again if the task is complete - this allows the user to call Reverse() in the OnComplete callback to implement a pingpong effect
      if (isTaskComplete())
      {
         _CleanupAndDestroyTask();
      }
   }

   _lastUpdateTimeSeconds = updateStartTimeSeconds;

   // Returning true means we should continue ticking
   return !isTaskComplete();
}
