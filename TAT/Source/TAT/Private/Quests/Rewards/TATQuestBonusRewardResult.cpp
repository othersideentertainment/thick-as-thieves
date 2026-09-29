// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Rewards/TATQuestBonusRewardResult.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestBonusRewardResult)

FTATQuestBonusRewardResult::FTATQuestBonusRewardResult(const FTATQuestConditionalReward& bonusReward, bool unlocked)
   : Rewards(bonusReward.Rewards), Unlocked(unlocked)
{
   
}
