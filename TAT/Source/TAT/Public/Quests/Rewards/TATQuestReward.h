// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "TATQuestReward.generated.h"

class UTATQuestRewardType;
struct FTATQuestRewardContext;
struct FTATQuestRewardRequirementContext;
struct FTATQuestRewardRequirementType;

USTRUCT(BlueprintType)
struct TAT_API FTATQuestReward
{
   GENERATED_BODY()
   
   // The specific thing to grant
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ShowDisplayNames))
   TSubclassOf<UTATQuestRewardType> RewardType;

   // The number of rewards to grant
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = 1, UIMin = 1))
   int32 Quantity = 1;

   void Grant(const FTATQuestRewardContext& context) const;
   bool IsValid() const;
};

// Collection of rewards granted for some requirement including (or in addition to) quest completion
USTRUCT(BlueprintType)
struct TAT_API FTATQuestConditionalReward
{
   GENERATED_BODY()

   // Rewards granted as part of this level
   UPROPERTY(BlueprintReadWrite, EditAnywhere,  meta = (TitleProperty = "{Quantity} {RewardType}"))
   TArray<FTATQuestReward> Rewards;

   // Conditions (in addition to quest completion) that must all be met for this reward to be granted
   UPROPERTY(EditAnywhere, Meta = (ExcludeBaseStruct))
   TArray<TInstancedStruct<FTATQuestRewardRequirementType>> Requirements;

   bool RequirementsMet(const FTATQuestRewardRequirementContext& context) const;
   void GrantRewards(const FTATQuestRewardContext& context) const;
};
