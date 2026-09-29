// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "UI/TATToastSubsystem.h"

// tat
#include "Developer/TATProjectSettings.h"

// wwise
#include "AkAudioEvent.h"

// ue
#include "Logging/LogVerbosity.h"
#include "Engine/AssetManager.h"
#include "Paper2D/Classes/PaperSprite.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToastSubsystem)

DECLARE_STATS_GROUP(TEXT("Toast Subsystem"), STATGROUP_Toasts, STATCAT_Advanced)

DECLARE_DWORD_ACCUMULATOR_STAT(TEXT("Queued Message Count"), STAT_QueuedMessageCount, STATGROUP_Toasts);
DECLARE_FLOAT_ACCUMULATOR_STAT(TEXT("Next Toast Slot Wait Time"), STAT_NextToastSlotWaitTime, STATGROUP_Toasts)

DEFINE_LOG_CATEGORY(LogTATToastSubsystem);

// Allows toggling the toast queue's cooldown logging
#define TAT_ENABLE_TOAST_QUEUE_COOLDOWN_LOGGING 1

// How frequently to check the toast queue for new messages
static constexpr int32 kToastQueueTicksPerSecond = 10;

namespace ToastHelpers
{
   template<typename AssetType, typename Lambda>
   static void AsyncLoadAsset(TWeakObjectPtr<UTATToastSubsystem> toastSubsystem, TSoftObjectPtr<AssetType> assetSoftPtr, Lambda&& callback)
   {
      check(toastSubsystem.IsValid());
      if (assetSoftPtr.IsNull())
      {
         callback(toastSubsystem.Get(), nullptr);
      }
      else if (assetSoftPtr.IsValid())
      {
         callback(toastSubsystem.Get(), assetSoftPtr.Get());
      }
      else
      {
         UAssetManager::GetStreamableManager().RequestAsyncLoad(assetSoftPtr.ToSoftObjectPath(), [toastSubsystem, assetSoftPtr, callback = MoveTemp(callback)]()
         {
            if (!toastSubsystem.IsValid())
            {
               return;
            }
            callback(toastSubsystem.Get(), assetSoftPtr.Get());
         });
      }
   }
}

FPendingToastMessage::FPendingToastMessage(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& icon, double queueSubsystemTime)
   : ToastId(toastId)
   , Message(message)
   , Icon(icon)
   , QueueSubsystemTime(queueSubsystemTime)
{
}

void UTATToastSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // Record the time the subsystem was initialized so we can use timestamps that are relative to the subsystem's running time.
   // Mostly this just makes debugging a lot easier.
   _subsystemStartTimeSeconds = FPlatformTime::Seconds();

   FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UTATToastSubsystem::_PostLoadMap);

   // Set up a tick method that we can enable only when we have pending outgoing messages
   static constexpr float tickInterval = 1.0f / static_cast<float>(kToastQueueTicksPerSecond);
   _tickTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UTATToastSubsystem::_Tick), tickInterval);

   const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();

   // One timer slot per simultaneous message
   _toastCooldownSlots.SetNum(FMath::Max(1, tatSettings.MaxNumSimultaneousToastMessages));

   // TODO: Async load in the future
   if (UDataTable* toastConfigs = tatSettings.ToastConfigsDataTable.LoadSynchronous())
   {
      toastConfigs->ForeachRow<FTATToastConfig>(TEXT("ToastConfig"),
         [this](const FName& Key, const FTATToastConfig& toastConfig)
      {
         if (_toastStatus.Contains(toastConfig.ToastId) || toastConfig.ToastId == FGameplayTag::EmptyTag)
         {
            UE_LOG(LogTATToastSubsystem, Error, TEXT("Skipping duplicate or empty toast type '%s'"), *toastConfig.ToastId.ToString());
            return;
         }

         const FToastStatus& status = _toastStatus.Emplace(toastConfig.ToastId, toastConfig);
         const FGameplayTag toastId = toastConfig.ToastId;

         // If the sound effect isn't already loaded, async-load it (NB. the FToastStatus constructor will assign SoundEffect if it's already loaded)
         if (!toastConfig.SoundEffect.IsNull() && status.SoundEffect.Get() == nullptr)
         {
            ToastHelpers::AsyncLoadAsset<UAkAudioEvent>(this, toastConfig.SoundEffect, [toastId](UTATToastSubsystem* self, UAkAudioEvent* sfx)
            {
               check(self->_toastStatus.Contains(toastId));
               self->_toastStatus[toastId].SoundEffect = sfx;
            });
         }
      });
   }
   else
   {
      UE_LOG(LogTATToastSubsystem, Error, TEXT("Could not load toast configs data table from project settings"));
   }
}

void UTATToastSubsystem::Deinitialize()
{
   Super::Deinitialize();

   // Unregister our tick function
   FTSTicker::GetCoreTicker().RemoveTicker(_tickTimerHandle);
   _tickTimerHandle.Reset();

   // Strictly speaking we don't need to clear data here, but for now lets do so because it will log any deleted toasts.
   constexpr bool matchToastsOnly = false;
   _ClearToastQueueAndResetAllCooldowns(matchToastsOnly);
}

bool UTATToastSubsystem::IsValidToastType(FGameplayTag toastId) const
{
   return _toastStatus.Contains(toastId);
}

bool UTATToastSubsystem::IsToastOnCooldown(FGameplayTag toastId) const
{
   return IsValidToastType(toastId) && _toastStatus[toastId].IsOnCooldown(_GetSubsystemTimeSeconds());
}

bool UTATToastSubsystem::RequestToastMessage(FGameplayTag toastId, const FText& message, TSoftObjectPtr<UPaperSprite> icon, bool discardIfNotShownImmediately)
{
   const double currentTime = _GetSubsystemTimeSeconds();
   UE_LOG(LogTATToastSubsystem, Log, TEXT("Requesting toast message '%s', '%s' at time %.2f"), *toastId.ToString(), *message.ToString(), currentTime);

   if (!IsValidToastType(toastId))
   {
      UE_LOG(LogTATToastSubsystem, Error, TEXT("Attempted to post toast of unregistered type: %s"), *toastId.ToString());
      if(GEngine)
      {
         GEngine->AddOnScreenDebugMessage(
            INDEX_NONE,
            5.f,
            FColor::Red,
            FString::Printf(TEXT("Attempted to post toast of unregistered type: %s - Message: %s"), *toastId.ToString(), *message.ToString())
         );
      }
      return false;
   }

   FToastStatus& toastState = _toastStatus[toastId];
   FToastCooldownSlot* nextAvailableCooldownSlot = _FindNextAvailableToastCooldownSlot(currentTime);

   // Use the default icon for this toast type if the passed in icon is explicitly null
   const TSoftObjectPtr<UPaperSprite>& messageIcon = !icon.IsNull() ? icon : toastState.DefaultIcon;

   // If there are no available slots on-screen, there are other queued messages, OR if the toast type is on cooldown, add the message to the queue
   if (nextAvailableCooldownSlot == nullptr || _toastQueue.Num() > 0 || IsToastOnCooldown(toastId))
   {
      if (discardIfNotShownImmediately)
      {
         return false;
      }

      // In theory this should never happen unless caused by a gameplay bug
      if (_toastQueue.Num() >= kMaxToastQueueSize)
      {
         UE_LOG(LogTATToastSubsystem, Error, TEXT("Discarding toast message (toastId='%s', message='%s') - toast queue is at capacity with %i pending messages!"),
            *toastId.ToString(), *message.ToString(), _toastQueue.Num());
         return false;
      }

      // If this toast type is a singleton, only add it to the queue if there isn't a toast with that same type in use or in the queue
      if (toastState.SingletonMode)
      {
         // Check if any toasts currently on-screen have the same id
         for (const FToastCooldownSlot& cooldownSlot : _toastCooldownSlots)
         {
            if (cooldownSlot.IsOnCooldown(currentTime) && cooldownSlot.CurrentToastId == toastId)
            {
               return false;
            }
         }

         // Check if any queued toasts have the same id
         for (const FPendingToastMessage& msg : _toastQueue)
         {
            if (msg.ToastId == toastId)
            {
               return false;
            }
         }
      }

      _QueueToastMessage(toastId, message, messageIcon);
      return true;
   }

   // There is room on-screen to display the toast, and the toast type is not on cooldown - just fire the message now
   toastState.NotifyToastSent(currentTime);
   ensure(nextAvailableCooldownSlot != nullptr);
   _FireToastMessage(currentTime, toastId, message, messageIcon, toastState.SoundEffect.Get(), nextAvailableCooldownSlot);
   return true;
}

double UTATToastSubsystem::_GetSubsystemTimeSeconds() const
{
   return FMath::Max(0.0, FPlatformTime::Seconds() - _subsystemStartTimeSeconds);
}

void UTATToastSubsystem::_ClearToastQueueAndResetAllCooldowns(bool clearMatchToastsOnly)
{
   auto isMatchRelated = [this](FGameplayTag toastId) -> bool
   {
      const FToastStatus* status = _toastStatus.Find(toastId);
      return status != nullptr && status->MatchRelatedToast;
   };

   // Reset toast status per-toast-type
   for (auto& pair : _toastStatus)
   {
      if (!clearMatchToastsOnly || pair.Value.MatchRelatedToast)
      {
         pair.Value.ResetCooldown();
      }
   }

   // Reset on-screen toast slot cooldowns
   for (FToastCooldownSlot& slot : _toastCooldownSlots)
   {
      if (!clearMatchToastsOnly || isMatchRelated(slot.CurrentToastId))
      {
         slot.Reset();
      }
   }

   // For now, warn about toast messages we're deleting from the queue without ever showing
   for (const FPendingToastMessage& toast : _toastQueue)
   {
      if (!clearMatchToastsOnly || isMatchRelated(toast.ToastId))
      {
         UE_LOG(LogTATToastSubsystem, Warning, TEXT("Deleting unsent toast message (type='%s'): '%s'"), *toast.ToastId.ToString(), *toast.Message.ToString());
      }
   }

   // Remove all matching queued toasts
   if (clearMatchToastsOnly)
   {
      _toastQueue.RemoveAll([isMatchRelated](const FPendingToastMessage& msg) { return isMatchRelated(msg.ToastId); });
   }
   else
   {
      _toastQueue.Reset();
   }
}

void UTATToastSubsystem::_PostLoadMap(UWorld* world)
{
   // After a map is loaded, clear all toasts with a toast-type that is marked as match-specific
   constexpr bool clearMatchToastsOnly = true;
   _ClearToastQueueAndResetAllCooldowns(clearMatchToastsOnly);
}

// NB. The float param that this function receives is the world delta seconds, NOT the time since this tick function last ran.
bool UTATToastSubsystem::_Tick(float /* worldDeltaTime */)
{
   check(IsInGameThread());

   // Ticker delegates should return true to automatically reschedule at the same delay, or false for a one-shot.
   constexpr bool continueTicking = true;

   // Update toast stats
   auto getNextToastSlotWaitTime = [this]() -> double
   {
      const double now = _GetSubsystemTimeSeconds();
      const int32 nextIndex = _GetNextToastCooldownSlotIndex();
      if (!_toastCooldownSlots.IsValidIndex(nextIndex))
      {
         // This shouldn't happen - it only occurs if there are no configured cooldown slots
         return -1.0;
      }
      return FMath::Max(0.0, _toastCooldownSlots[nextIndex].NextAllowedDisplayTimeSeconds - now);
   };
   SET_FLOAT_STAT(STAT_NextToastSlotWaitTime, getNextToastSlotWaitTime());
   SET_DWORD_STAT(STAT_QueuedMessageCount, _toastQueue.Num());

   if (_toastQueue.Num() == 0)
   {
      return continueTicking;
   }

   const double currentTime = _GetSubsystemTimeSeconds();

   // Check if we have any open slots for firing a message
   FToastCooldownSlot* nextAvailableCooldownSlot = _FindNextAvailableToastCooldownSlot(currentTime);
   if (nextAvailableCooldownSlot == nullptr)
   {
#if TAT_ENABLE_TOAST_QUEUE_COOLDOWN_LOGGING
      // Log a message showing how long until the next slot opens up.
      // Only show this once per second so it doesn't get _too_ spammy.
      static int32 logInterval = 0;
      if (logInterval == 0)
      {
         const int32 nextAvailableCooldownSlotIndex = _GetNextToastCooldownSlotIndex();
         if (_toastCooldownSlots.IsValidIndex(nextAvailableCooldownSlotIndex))
         {
            UE_LOG(LogTATToastSubsystem, Warning, TEXT("Toast queue has %i pending messages, but all slots are on cooldown. Next slot (%i) will be ready in %.2f seconds (at %.2f)."),
               _toastQueue.Num(),
               nextAvailableCooldownSlotIndex,
               _toastCooldownSlots[nextAvailableCooldownSlotIndex].NextAllowedDisplayTimeSeconds - currentTime,
               _toastCooldownSlots[nextAvailableCooldownSlotIndex].NextAllowedDisplayTimeSeconds);
         }
         else
         {
            UE_LOG(LogTATToastSubsystem, Error, TEXT("Toast queue has %i pending messages but all cooldown slots are invalid!"), _toastQueue.Num());
         }
      }

      logInterval = (logInterval + 1) % kToastQueueTicksPerSecond;
#endif

      return continueTicking;
   }

   // Find the first queued message that can be sent and send it if possible
   int32 queueIdx = 0;
   while (queueIdx < _toastQueue.Num())
   {
      const FPendingToastMessage& queuedToast = _toastQueue[queueIdx];
      FToastStatus& toastState = _toastStatus[queuedToast.ToastId];
      if (!toastState.IsOnCooldown(currentTime))
      {
         toastState.NotifyToastSent(currentTime);
         check(nextAvailableCooldownSlot != nullptr);

#if TAT_ENABLE_TOAST_QUEUE_COOLDOWN_LOGGING
         // Log a warning if this message is being fired late-ish
         const double secondsInQueue = currentTime - queuedToast.QueueSubsystemTime;
         if (secondsInQueue >= 10.0)
         {
            UE_LOG(LogTATToastSubsystem, Error, TEXT("Processing queued toast message %.2f seconds after it was queued (id='%s', message='%s', queueTime=%.2f, currentTime=%.2f)"),
               secondsInQueue,
               *queuedToast.ToastId.ToString(),
               *queuedToast.Message.ToString(),
               queuedToast.QueueSubsystemTime,
               currentTime);
         }
#endif

         _FireToastMessage(currentTime, queuedToast.ToastId, queuedToast.Message, queuedToast.Icon, toastState.SoundEffect.Get(), nextAvailableCooldownSlot);
         check(nextAvailableCooldownSlot->IsOnCooldown(currentTime));

         // Remove the queue entry
         _toastQueue.RemoveAt(queueIdx, EAllowShrinking::No);

         // If we have another slot available, loop around again, otherwise we're done
         nextAvailableCooldownSlot = _FindNextAvailableToastCooldownSlot(currentTime);
         if (nextAvailableCooldownSlot != nullptr)
         {
            // We just removed an entry, so intentionally do not increment queueIdx
            continue;
         }
         else
         {
            // No cooldown slots remaining
            break;
         }
      }

      ++queueIdx;
   }

   return continueTicking;
}

void UTATToastSubsystem::_QueueToastMessage(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& icon)
{
   const double currentTime = _GetSubsystemTimeSeconds();
   UE_LOG(LogTATToastSubsystem, Log, TEXT("Queueing toast message '%s', '%s' at time %.2f"), *toastId.ToString(), *message.ToString(), currentTime);
   _toastQueue.Add(FPendingToastMessage(toastId, message, icon, currentTime));
}

void UTATToastSubsystem::_FireToastMessage(double currentSubsystemTimeSeconds, FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& icon, UAkAudioEvent* sfx, FToastCooldownSlot* cooldownSlot)
{
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();

   UE_LOG(LogTATToastSubsystem, Log, TEXT("Firing toast message '%s', '%s' at time %.2f"), *toastId.ToString(), *message.ToString(), currentSubsystemTimeSeconds);

   // Manage the SFX cooldown
   const bool playToastSfx = sfx != nullptr && !ToastHelpers::IsCooldownActive(currentSubsystemTimeSeconds, _nextAllowedSFXPlayTimeSeconds);
   if (playToastSfx)
   {
      _nextAllowedSFXPlayTimeSeconds = currentSubsystemTimeSeconds + projectSettings.MinSecondsBetweenToastSFX;
   }

   UE_LOG(LogTATToastSubsystem, Log, TEXT("[TOAST (%s)] %s"), *toastId.ToString(), *message.ToString());

   ToastReadyEvent.Broadcast(message, projectSettings.ToastDuration, icon, playToastSfx ? sfx : nullptr);

   if (cooldownSlot != nullptr)
   {
      cooldownSlot->Set(currentSubsystemTimeSeconds, projectSettings.ToastDuration, toastId);
   }
}

auto UTATToastSubsystem::_FindNextAvailableToastCooldownSlot(double currentSubsystemTimeSeconds) -> FToastCooldownSlot*
{
   for (int32 i = 0; i < _toastCooldownSlots.Num(); i++)
   {
      if (!_toastCooldownSlots[i].IsOnCooldown(currentSubsystemTimeSeconds))
      {
         return &_toastCooldownSlots[i];
      }
   }
   return nullptr;
}

int32 UTATToastSubsystem::_GetNextToastCooldownSlotIndex() const
{
   int32 slotIndex = INDEX_NONE;
   double minNextAllowedTime = DBL_MAX;
   for (int32 i = 0; i < _toastCooldownSlots.Num(); i++)
   {
      if (_toastCooldownSlots[i].NextAllowedDisplayTimeSeconds < minNextAllowedTime)
      {
         slotIndex = i;
         minNextAllowedTime = _toastCooldownSlots[i].NextAllowedDisplayTimeSeconds;
      }
   }
   return slotIndex;
}

FToastStatus::FToastStatus(const FTATToastConfig& toastConfig)
   : CooldownType(toastConfig.CooldownType)
   , SingletonMode(toastConfig.SingletonToast)
   , MatchRelatedToast(toastConfig.MatchRelatedToast)
   , DefaultIcon(toastConfig.DefaultIcon)
   , SoundEffect(toastConfig.SoundEffect.Get())
{
   if (toastConfig.CooldownType == ETATToastCooldownType::Timer)
   {
      CooldownSeconds = toastConfig.ToastCooldownTimer;
   }
}

bool FToastStatus::IsOnCooldown(double currentSubsystemTimeSeconds) const
{
   if (CooldownType == ETATToastCooldownType::NoCooldown)
   {
      return false;
   }
   else if (CooldownType == ETATToastCooldownType::Timer)
   {
      // Fire if enough time has elapsed since the last toast with this id
      return ToastHelpers::IsCooldownActive(currentSubsystemTimeSeconds, NextAllowedToastDisplayTime);
   }
   else
   {
      checkNoEntry();
      return false;
   }
}

void FToastStatus::NotifyToastSent(double currentSubsystemTimeSeconds)
{
   // Update the next allowed time for timer-type cooldowns
   if (CooldownType == ETATToastCooldownType::Timer)
   {
      NextAllowedToastDisplayTime = currentSubsystemTimeSeconds + static_cast<double>(CooldownSeconds);
   }
}
