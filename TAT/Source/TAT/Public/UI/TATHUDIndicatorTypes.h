// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"

#include "TATHUDIndicatorTypes.generated.h"

class UPaperSprite;
class UUserWidget;

UENUM(BlueprintType)
enum class ETATHUDIndicatorTimerDirection : uint8
{
   Decreasing,
   Increasing,
};

/// Different HUD indicator styles and/or color schemes.
UENUM(BlueprintType)
enum class ETATHUDIndicatorStyle : uint8
{
   Neutral,
   Positive,
   Negative,
};

/// The visual state data for a HUD indicator
USTRUCT(BlueprintType)
struct TAT_API FTATHUDIndicatorState
{
   GENERATED_BODY()

   /// The indicator icon
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   TSoftObjectPtr<UPaperSprite> Icon;

   /// The style of the indicator, based on the intended effect
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   ETATHUDIndicatorStyle Style = ETATHUDIndicatorStyle::Neutral;

   /// A value between 0 and 1 indicating the "progress" of the indicator.
   /// Only visible if greater than zero.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   float Progress = 0.0f;

   /// The world time when the timer started.
   /// The timer is only visible when the start time and duration are both greater than zero.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   float TimerStartTime = 0.0f;

   /// The duration of the timer indicator.
   /// The timer is only visible when the start time and duration are both greater than zero.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   float TimerDuration = 0.0f;

   /// Should the timer indicator be increasing or decreasing?
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   ETATHUDIndicatorTimerDirection TimerDirection = ETATHUDIndicatorTimerDirection::Decreasing;

   /// If not zero, shows the value on the indicator. Used for things like stack count.
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   int32 Count = 0;

   /// If the indicator should be highlighted/glowing
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   bool Highlight = false;

   /// Text label appearing on or near the icon
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator State")
   FText Label;

   float GetTimerElapsedSeconds(float currentWorldTime) const
   {
      return (TimerStartTime > 0) ? (currentWorldTime - TimerStartTime) : 0.0f;
   }

   float GetTimerNormalizedValue(float currentWorldTime) const
   {
      if (TimerStartTime > 0 && TimerDuration > 0)
      {
         const float value = FMath::Clamp((currentWorldTime - TimerStartTime) / TimerDuration, 0.0f, 1.0f);
         switch (TimerDirection)
         {
         case ETATHUDIndicatorTimerDirection::Decreasing:
            return 1.0f - value;
         case ETATHUDIndicatorTimerDirection::Increasing:
            return value;
         }
      }
      return 0.0f;
   }

   bool operator==(const FTATHUDIndicatorState& rhs) const
   {
      return Icon == rhs.Icon
         && Style == rhs.Style
         && FMath::IsNearlyEqual(Progress, rhs.Progress, 0.0001f)
         && TimerStartTime == rhs.TimerStartTime
         && FMath::IsNearlyEqual(TimerDuration, rhs.TimerDuration, 0.0001f)
         && TimerDirection == rhs.TimerDirection
         && Count == rhs.Count
         && Highlight == rhs.Highlight
         && Label.IdenticalTo(rhs.Label);
   }
};

template<>
struct TStructOpsTypeTraits<FTATHUDIndicatorState> : public TStructOpsTypeTraitsBase2<FTATHUDIndicatorState>
{
   enum
   {
      WithIdenticalViaEquality = true,
   };
};

USTRUCT(BlueprintType)
struct TAT_API FTATHUDIndicatorInfo
{
   GENERATED_BODY()

   /// The gameplay effect that owns this indicator
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator Info")
   FActiveGameplayEffectHandle EffectHandle;

   /// The UMG widget created to represent this indicator
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator Info")
   TObjectPtr<UUserWidget> Widget;

   /// The current visual state
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Indicator Info")
   FTATHUDIndicatorState State;
};
