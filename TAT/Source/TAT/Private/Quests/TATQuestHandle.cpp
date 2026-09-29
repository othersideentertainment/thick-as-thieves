// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATQuestHandle.h"

// tat
#include "Quests/TATQuestInfo.h"
#include "Quests/TATQuestTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestHandle)

FGameplayTag FTATQuestHandle::GetQuestTag() const
{
   // Note: The row name should be 1:1 to the tag (as enforced by validation)
   // Is this RequestGameplayTag actually faster than looking up the row handle?
   return _rowHandle.RowName.IsNone() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(_rowHandle.RowName);
}

const FTATQuestInfo* FTATQuestHandle::GetQuest() const
{
   return _rowHandle.GetRow<FTATQuestInfo>(TEXT("QuestHandle"));
}

const FTATContractInfo* FTATQuestHandle::GetContract() const
{
   return _rowHandle.GetRow<FTATContractInfo>(TEXT("QuestHandle"));
}

const FTATMissionInfo* FTATQuestHandle::GetMission() const
{
   return _rowHandle.GetRow<FTATMissionInfo>(TEXT("QuestHandle"));
}

const FTATMinimalQuestInfo* FTATQuestHandle::GetMinimalQuest() const
{
   return _rowHandle.GetRow<FTATMinimalQuestInfo>(TEXT("QuestHandle"));
}
