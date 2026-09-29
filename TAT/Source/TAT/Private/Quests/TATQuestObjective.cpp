// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATQuestObjective.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestObjective)

FGameplayTag FTATQuestObjectiveInfo::GetRelatedLootTag() const
{
   return FGameplayTag();
}

void FTATQuestObjectiveInfo::ForEachRelatedLootTag(TFunctionRef<void(const FGameplayTag&)> visitor) const
{
   FGameplayTag relatedLootTag = GetRelatedLootTag();
   if (relatedLootTag.IsValid())
   {
      visitor(relatedLootTag);
   }
}

FText FTATQuestObjectiveInfo::GetObjectiveText(const UObject* worldContext) const
{
   return FText();
}

FTATObjectiveTrackerPayload FTATQuestObjectiveInfo::CreateTracker() const
{
   return FTATObjectiveTrackerPayload();
}

FTATMinimalObjective FTATQuestObjectiveInfo::ToMinimalObjective(const UObject* worldContext) const
{
   return FTATMinimalObjective {
      .Id = reinterpret_cast<UPTRINT>(this), // NB: pointer-to-int cast is well-defined, just not the other direction
      .ObjectiveText = GetObjectiveText(worldContext),
      .Tracker = CreateTracker(),
      .TargetProgress = GetTargetProgress(),
      .ProgressStyle = GetProgressStyle(),
   };
}
