// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATWaitForLocalPlayerState.h"

// ue
#include "GameFramework/PlayerController.h"
#include "FindSessionsCallbackProxy.h"
#include "Player/TATPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWaitForLocalPlayerState)

DEFINE_LOG_CATEGORY_STATIC(LogTATWaitForLocalPlayerState, Log, All);

// static
UTATWaitForLocalPlayerState* UTATWaitForLocalPlayerState::WaitForLocalTATPlayerState(UObject* contextObj, float maxWaitTimeSeconds)
{
   UTATWaitForLocalPlayerState* proxy = NewObject<UTATWaitForLocalPlayerState>();
   proxy->_contextObj = contextObj;
   proxy->_maxWaitTimeSeconds = FMath::Max(0.0f, maxWaitTimeSeconds);
   return proxy;
}

void UTATWaitForLocalPlayerState::Activate()
{
   Super::Activate();

   UWorld* world = GEngine->GetWorldFromContextObject(_contextObj, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      OnFailure.Broadcast(nullptr);
      SetReadyToDestroy();
      return;
   }

   // If we already have a local player state, don't bother setting up the tick function
   if (ATATPlayerState* playerState = ATATPlayerState::GetLocalTATPlayerState(_contextObj))
   {
      OnFoundLocalPlayerState.Broadcast(playerState);
      SetReadyToDestroy();
      return;
   }

   _realStartTimeSeconds = world->GetRealTimeSeconds();
   _lastWarningTimeSeconds = _realStartTimeSeconds;

   static constexpr float tickInterval = 1.0f / 30.0f;
   static constexpr bool looping = true;
   world->GetTimerManager().SetTimer(_timerHandle, this, &UTATWaitForLocalPlayerState::_Update, tickInterval, looping);
   RegisterWithGameInstance(_contextObj);
}

void UTATWaitForLocalPlayerState::SetReadyToDestroy()
{
   if (_timerHandle.IsValid())
   {
      if (UGameInstance* oldGameInstance = RegisteredWithGameInstance.Get())
      {
         oldGameInstance->GetTimerManager().ClearTimer(_timerHandle);
      }
   }

   Super::SetReadyToDestroy();
}

void UTATWaitForLocalPlayerState::_Update()
{
   UWorld* world = GEngine->GetWorldFromContextObject(_contextObj, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      OnFailure.Broadcast(nullptr);
      SetReadyToDestroy();
      return;
   }

   if (ATATPlayerState* playerState = ATATPlayerState::GetLocalTATPlayerState(world))
   {
      OnFoundLocalPlayerState.Broadcast(playerState);
      SetReadyToDestroy();
      return;
   }

   const double now = world->GetRealTimeSeconds();
   const double elapsedTimeSeconds = now - _realStartTimeSeconds;

   if (_maxWaitTimeSeconds > 0 && elapsedTimeSeconds >= _maxWaitTimeSeconds)
   {
      OnFailure.Broadcast(nullptr);
      SetReadyToDestroy();
      return;
   }

   // In case this runs for an excessively long time, log some warnings on an interval
   const double timeSinceLastWarning = now - _lastWarningTimeSeconds;
   if (elapsedTimeSeconds >= 30.0f && timeSinceLastWarning >= 10.0f)
   {
      UE_LOG(LogTATWaitForLocalPlayerState, Warning, TEXT("UTATWaitForLocalPlayerState (%s) has been waiting for a local player state for %.2f seconds."),
         *GetNameSafe(_contextObj), elapsedTimeSeconds);
      _lastWarningTimeSeconds = now;
   }
}
