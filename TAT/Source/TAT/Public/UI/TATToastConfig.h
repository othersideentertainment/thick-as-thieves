// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "TATToastConfig.generated.h"

class UPaperSprite;
class UAkAudioEvent;

UENUM()
enum class ETATToastCooldownType
{
   /// Always show the toast (possibly adding it to a queue if there is already a message on the screen)
   NoCooldown,

   /// Prevent this toast from displaying if it has been displayed recently (see also: ToastCooldownTimer)
   Timer,
};

USTRUCT(BlueprintType)
struct TAT_API FTATToastConfig : public FTableRowBase
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Toast.Type"))
   FGameplayTag ToastId;

   /// Should the toast's cooldown be based on how many times we've requested it, or how much time has passed?
   UPROPERTY(EditDefaultsOnly)
   ETATToastCooldownType CooldownType = ETATToastCooldownType::NoCooldown;

   /// If enabled, the toast system will only allow one message of this type at a time.
   /// Additional toast messages with the same type will be discarded if there is already one on-screen or queued.
   UPROPERTY(EditDefaultsOnly)
   bool SingletonToast = false;

   /// If enabled, this toast is related to the match it was sent during.
   /// If the match ends while the toast is queued but not yet displayed, it will be deleted from the toast queue.
   UPROPERTY(EditDefaultsOnly)
   bool MatchRelatedToast = true;

   /// The toast should only be displayed if it hasn't been displayed in the last N seconds
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "CooldownType == ETATToastCooldownType::Timer", EditConditionHides, ClampMin = "0.0", UIMin = "0.0"))
   float ToastCooldownTimer = 60.0f;

   /// Default icon used for toast messages in this category if they don't specify one in the message
   UPROPERTY(EditDefaultsOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> DefaultIcon;

   /// SFX to play when the toast message appears on-screen
   UPROPERTY(EditDefaultsOnly)
   TSoftObjectPtr<UAkAudioEvent> SoundEffect;
};

