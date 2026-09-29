// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "TATQuestBonusRewardResult.generated.h"

struct FTATQuestConditionalReward;
struct FTATQuestReward;

/// Used to pass player-facing bonus reward results to blueprint for UI representation.
USTRUCT(BlueprintType)
struct TAT_API FTATQuestBonusRewardResult
{
   GENERATED_BODY()

   FTATQuestBonusRewardResult() {}

   FTATQuestBonusRewardResult(const FTATQuestConditionalReward& bonusReward, bool unlocked);

   UPROPERTY(BlueprintReadOnly)
   TArray<FTATQuestReward> Rewards;

   UPROPERTY(BlueprintReadOnly)
   bool Unlocked = false;
};

