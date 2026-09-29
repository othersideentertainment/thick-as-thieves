// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UtilityAISettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Utility AI Settings"))
class OSEAI_API UUtilityAISettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UUtilityAISettings* Get() { return GetDefault<UUtilityAISettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "Performance|Goals", meta=(ForceUnits=ms))
   float MaxFrameBudgetForGoals { 1.f };
   UPROPERTY(Config, EditDefaultsOnly, Category = "Performance|Behaviors", meta=(ForceUnits=ms))
   float MaxFrameBudgetForBehaviors { 1.f };
   UPROPERTY(Config, EditDefaultsOnly, Category = "Performance|Behaviors", meta=(ForceUnits=ms))
   float MinTimeBetweenConsiderationEvaluations { 0.1f };
};
