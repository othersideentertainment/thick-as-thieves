// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Matchmaking/TATRankingFunctionLibrary.h"

// tat
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Matchmaking/TATScoringSettings.h"
#include "Player/TATPlayerState.h"

// ue
#include "Curves/CurveFloat.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRankingFunctionLibrary)
DEFINE_LOG_CATEGORY_STATIC(LogTATRankingFunctionLibrary, Log, All);

int32 UTATRankingFunctionLibrary::GenerateMatchRankingForPlayer(const FMatchPersistentData& matchPersistentData)
{
   const UTATScoringSettings& scoringSettings = UTATScoringSettings::Get();
   int32 totalRanking = 0;

   totalRanking += matchPersistentData.ContractResult.IsObjectiveComplete ? scoringSettings.ContractCompleteBonus : 0.f;
   totalRanking += matchPersistentData.MissionResult.IsObjectiveComplete ? scoringSettings.MissionCompleteBonus : 0.f;

   // Apply loot bonuses
   totalRanking += matchPersistentData.KeptCarriedLootValue * scoringSettings.CarriedLootValueMultiplierBonus;
   totalRanking += matchPersistentData.StashedLootValue * scoringSettings.StashedLootValueMultiplierBonus;

   // Curve should have been async-loaded by now in TATPvPGameMode::BeginPlay()
   if (UCurveFloat* escapeOrderBonusCurve = scoringSettings.EscapeOrderBonus.LoadSynchronous())
   {
      totalRanking += escapeOrderBonusCurve->GetFloatValue(matchPersistentData.EscapeOrderPlacement);
   }
   else
   {
      UE_LOG(LogTATRankingFunctionLibrary, Error, TEXT("EscapeOrderBonus is unassigned in TATScoringSettings!"));
   }

   return totalRanking;
}
