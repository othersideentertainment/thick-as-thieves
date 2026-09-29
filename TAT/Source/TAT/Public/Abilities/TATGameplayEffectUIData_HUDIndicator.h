// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATHUDIndicatorTypes.h"

// ue
#include "GameplayEffectUIData.h"

#include "TATGameplayEffectUIData_HUDIndicator.generated.h"

class UPaperSprite;

UCLASS(DisplayName="[TAT] HUD Indicator")
class TAT_API UTATGameplayEffectUIData_HUDIndicator : public UGameplayEffectUIData
{
   GENERATED_BODY()

public:
   UTATGameplayEffectUIData_HUDIndicator();

   /// The icon displayed on the screen.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Indicator")
   TSoftObjectPtr<UPaperSprite> Icon;

   /// The style of the indicator, based on the intended effect
   /// Pick a style based on the design intent of the gameplay effect. For example, Negative for a debuff or Positive for a buff.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Indicator")
   ETATHUDIndicatorStyle Style = ETATHUDIndicatorStyle::Neutral;

   /// Show the time remaining on the indicator.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Indicator")
   bool ShowTimer = false;

   /// Should the timer count down or up?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Indicator", Meta = (EditCondition = "ShowTimer"))
   ETATHUDIndicatorTimerDirection TimerDirection = ETATHUDIndicatorTimerDirection::Decreasing;

   /// Show the number of stacks as a counter.
   /// Requires the gameplay effect to support stacking.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Indicator")
   bool ShowStackCount = false;

   /// Optional text label appearing on or near the icon.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD Indicator")
   FText Label;
};
