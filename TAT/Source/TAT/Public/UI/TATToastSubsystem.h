// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATToastConfig.h"

// ue
#include "Containers/Map.h"
#include "GameplayTags.h"
#include "Internationalization/Text.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "TATToastSubsystem.generated.h"

class UAkAudioEvent;
class UPaperSprite;

DECLARE_LOG_CATEGORY_EXTERN(LogTATToastSubsystem, Log, All)

/// Private struct used by UTATToastSubsystem to store per-toast type settings and cooldowns
USTRUCT()
struct FToastStatus
{
   GENERATED_BODY()

public:
   ETATToastCooldownType CooldownType = ETATToastCooldownType::NoCooldown;
   bool SingletonMode = false;
   bool MatchRelatedToast = false;
   float CooldownSeconds = 0.0f;
   double NextAllowedToastDisplayTime = 0.0;

   UPROPERTY()
   TSoftObjectPtr<UPaperSprite> DefaultIcon;

   UPROPERTY()
   TObjectPtr<UAkAudioEvent> SoundEffect;

   FToastStatus() = default;
   FToastStatus(const FToastStatus&) = default;
   explicit FToastStatus(const FTATToastConfig& toastConfig);

   bool IsOnCooldown(double currentWorldTimeSeconds) const;
   void NotifyToastSent(double currentWorldTimeSeconds);
   void ResetCooldown() { NextAllowedToastDisplayTime = 0.0; }
};

/// Private struct used by UTATToastSubsystem to store a queued message
USTRUCT()
struct FPendingToastMessage
{
   GENERATED_BODY()

public:
   FGameplayTag ToastId;
   FText Message;

   UPROPERTY()
   TSoftObjectPtr<UPaperSprite> Icon;

   double QueueSubsystemTime = 0.0;

   FPendingToastMessage() = default;
   FPendingToastMessage(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& icon, double queueSubsystemTime);
};

namespace ToastHelpers
{
   FORCEINLINE bool IsCooldownActive(double currentWorldTimeSeconds, double nextAllowedDisplayTimeSeconds)
   {
      return nextAllowedDisplayTimeSeconds > 0 && currentWorldTimeSeconds < nextAllowedDisplayTimeSeconds;
   }
}

UCLASS()
class TAT_API UTATToastSubsystem : public ULocalPlayerSubsystem
{
   GENERATED_BODY()

private:
   struct FToastCooldownSlot;

public:
   // From USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void Deinitialize() override;

   /// Checks if a gameplay tag is valid for toast messages
   UFUNCTION(BlueprintPure, Category = "TAT|Toast Subsystem")
   bool IsValidToastType(UPARAM(Meta = (Categories = "Toast.Type")) FGameplayTag toastId) const;

   /// Checks if a toast id is still on cooldown
   UFUNCTION(BlueprintPure, Category = "TAT|Toast Subsystem")
   bool IsToastOnCooldown(UPARAM(Meta = (Categories = "Toast.Type")) FGameplayTag toastId) const;

   /// Request that we display a toast of this type and with this message. The subsystem will handle cooldowns, de-duplication, etc.
   /// If this returns true, the message was successfully displayed or queued. Returning false indicates that the message was discarded.
   /// 
   /// If discardIfNotShownImmediately is true and the message can't be immediately displayed (eg. toast cooldown; no available space on-screen, etc.),
   /// the message will be discarded. Otherwise the message may be queued for display when the relevant cooldowns are up.
   UFUNCTION(BlueprintCallable, Category = "TAT|Toast Subsystem")
   bool RequestToastMessage(UPARAM(Meta = (Categories = "Toast.Type")) FGameplayTag toastId, const FText& message, TSoftObjectPtr<UPaperSprite> icon = nullptr, bool discardIfNotShownImmediately = false);

   DECLARE_EVENT_FourParams(UTATToastSubsystem, FOnToastReady, const FText&, float, const TSoftObjectPtr<UPaperSprite>&, UAkAudioEvent*);
   FOnToastReady ToastReadyEvent;

private:
   // Never let the toast queue grow past this number of messages.
   // This is intended to be a higher number than it should ever grow - if we're sending the player
   // hundreds of messages in a relatively short timeframe, that points to a bug in gameplay code somewhere.
   static constexpr int32 kMaxToastQueueSize = 255;

   /// Gets the time in seconds since this subsystem was initialized
   double _GetSubsystemTimeSeconds() const;

   /// Empties the toast queue and resets all toast-related cooldowns.
   /// If clearMatchToastsOnly is true, this will only remove toast messages that have a toast type with MatchRelatedToast enabled.
   void _ClearToastQueueAndResetAllCooldowns(bool clearMatchToastsOnly);

   /// Called after a map is loaded
   void _PostLoadMap(UWorld* world);

   bool _Tick(float worldDeltaTime);

   void _QueueToastMessage(FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& icon);

   void _FireToastMessage(double currentWorldTimeSeconds, FGameplayTag toastId, const FText& message, const TSoftObjectPtr<UPaperSprite>& icon, UAkAudioEvent* sfx, FToastCooldownSlot* cooldownSlot);

   // Returns a pointer to the next allowed display time for a toast message, or nullptr if all slots are in use
   FToastCooldownSlot* _FindNextAvailableToastCooldownSlot(double currentWorldTimeSeconds);

   // Returns the index into _toastCooldownSlots for the cooldown slot with the next available cooldown (regardless of if it's ready yet or not)
   int32 _GetNextToastCooldownSlotIndex() const;

   // FTicker::GetCoreTicker() handle to manage this subsystem's _Tick method
   FTSTicker::FDelegateHandle _tickTimerHandle;

   struct FToastCooldownSlot
   {
      double NextAllowedDisplayTimeSeconds = 0.0;
      FGameplayTag CurrentToastId;

      FORCEINLINE void Set(double currentWorldTimeSeconds, float displayDurationSeconds, FGameplayTag toastId)
      {
         NextAllowedDisplayTimeSeconds = currentWorldTimeSeconds + (double)displayDurationSeconds;
         CurrentToastId = toastId;
      }

      FORCEINLINE void Reset()
      {
         NextAllowedDisplayTimeSeconds = 0.0;
         CurrentToastId = FGameplayTag::EmptyTag;
      }

      FORCEINLINE bool IsOnCooldown(double currentWorldTimeSeconds) const
      {
         return ToastHelpers::IsCooldownActive(currentWorldTimeSeconds, NextAllowedDisplayTimeSeconds);
      }
   };

   // Unordered collection of cooldown slots - one for each simultaneous message that can be on-screen at once.
   // (see also: UTATProjectSettings::MaxNumSimultaneousToastMessages)
   TArray<FToastCooldownSlot> _toastCooldownSlots;

   // Per-toast-type settings, state, and cooldown management
   UPROPERTY(Transient)
   TMap<FGameplayTag, FToastStatus> _toastStatus;

   // Queue for outgoing toast messages
   UPROPERTY(Transient)
   TArray<FPendingToastMessage> _toastQueue;

   // Toast SFX cooldown. Used to prevent too many of these firing in a short timespan.
   double _nextAllowedSFXPlayTimeSeconds = 0.0;

   // System time (see FPlatformTime::Seconds) that the subsystem was initialized
   double _subsystemStartTimeSeconds = 0.0;
};
