// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Engine/DeveloperSettings.h"

// ue
#include "UObject/SoftObjectPtr.h"

#include "TATScoringSettings.generated.h"

class UCurveFloat;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Scoring Settings"))
class TAT_API UTATScoringSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // bp access
   UFUNCTION(BlueprintPure, Category = "TAT Scoring Settings")
   static UTATScoringSettings* GetTATScoringSettings() { return GetMutableDefault<UTATScoringSettings>(); }

   // C++ access
   static const UTATScoringSettings& Get() { return *GetDefault<UTATScoringSettings>(); }

   UPROPERTY(EditAnywhere, Config, Category = "Match Rank")
   float StashedLootValueMultiplierBonus = 1.f;

   UPROPERTY(EditAnywhere, Config, Category = "Match Rank")
   float CarriedLootValueMultiplierBonus = 1.f;

   // Bonus applied based on the order the player escaped (relative to others). If player fails to escape, no bonus is granted.
   // 0 = escaped 1st, 1 = escaped 2nd, etc
   UPROPERTY(EditAnywhere, Config, Category = "Match Rank")
   TSoftObjectPtr<UCurveFloat> EscapeOrderBonus = nullptr;

   UPROPERTY(EditAnywhere, Config, Category = "Match Rank", meta = (UIMin = 0, ClampMin = 0))
   float MissionCompleteBonus = 0.f;

   UPROPERTY(EditAnywhere, Config, Category = "Match Rank", meta = (UIMin = 0, ClampMin = 0))
   float ContractCompleteBonus = 0.f;

   // Used in FTATPlayerPerformanceRankingData to calculate an exponential moving average of various performance metrics
   UPROPERTY(EditAnywhere, Config, Category = "Player Ranking", meta = (UIMin = 1, ClampMin = 1))
   int32 RankingMatchCountMemory = 4;
};
