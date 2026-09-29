// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Rewards/TATQuestRewardUtils.h"

// tat
#include "Quests/Rewards/TATQuestReward.h"
#include "Quests/Rewards/TATQuestRewardType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestRewardUtils)

TSoftObjectPtr<UPaperSprite> UTATQuestRewardUtils::GetIconForRewardType(TSubclassOf<UTATQuestRewardType> rewardType, UObject* worldContext)
{
   if (const UTATQuestRewardType* rewardCdo = rewardType.GetDefaultObject())
   {
      return rewardCdo->GetIcon(worldContext);
   }
   else
   {
      return TSoftObjectPtr<UPaperSprite>();
   }
}

const FText& UTATQuestRewardUtils::GetNameForRewardType(TSubclassOf<UTATQuestRewardType> rewardType, UObject* worldContext)
{
   if (const UTATQuestRewardType* rewardCdo = rewardType.GetDefaultObject())
   {
      return rewardCdo->GetName(worldContext);
   }
   else
   {
      return FText::GetEmpty();
   }
}

TSoftObjectPtr<UPaperSprite> UTATQuestRewardUtils::GetIconForReward(const FTATQuestReward& reward, UObject* worldContext)
{
   return GetIconForRewardType(reward.RewardType, worldContext);
}

const FText& UTATQuestRewardUtils::GetNameForReward(const FTATQuestReward& reward, UObject* worldContext)
{
   return GetNameForRewardType(reward.RewardType, worldContext);
}
