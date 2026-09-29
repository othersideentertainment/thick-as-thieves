// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Rewards/TATQuestReward.h"

// tat
#include "Quests/Rewards/Requirements/TATQuestRewardRequirementType.h"
#include "Quests/Rewards/TATQuestRewardType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestReward)
DEFINE_LOG_CATEGORY_STATIC(LogTATQuestReward, Log, All);

void FTATQuestReward::Grant(const FTATQuestRewardContext& context) const
{
   const UTATQuestRewardType* rewardCdo = RewardType.GetDefaultObject();
   if (ensure(rewardCdo))
   {
      rewardCdo->Grant(Quantity, context);
   }
}

bool FTATQuestReward::IsValid() const
{
   return RewardType != nullptr;
}

bool FTATQuestConditionalReward::RequirementsMet(const FTATQuestRewardRequirementContext& context) const
{
   for (const TInstancedStruct<FTATQuestRewardRequirementType>& instancedStruct : Requirements)
   {
      const FTATQuestRewardRequirementType* requirement = instancedStruct.GetPtr();
      UE_CLOG(requirement == nullptr, LogTATQuestReward, Warning, TEXT("Undefined TATQuestRewardRequirementType found in FTATQuestReward!"));
      if (requirement)
      {
         if (!requirement->IsMet(context))
         {
            return false;
         }
      }
   }
   return true;
}

void FTATQuestConditionalReward::GrantRewards(const FTATQuestRewardContext& context) const
{
   for (const FTATQuestReward& reward : Rewards)
   {
      reward.Grant(context);
   }
}
