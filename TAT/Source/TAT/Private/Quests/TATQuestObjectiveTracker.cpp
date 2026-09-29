// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATQuestObjectiveTracker.h"

// tat

// ue

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestObjectiveTracker)

bool UTATQuestObjectiveTracker::IsCompleteForMatchEnd(bool escaped, const FMatchPersistentData& matchData) const
{
   return IsComplete() && escaped;
}

void UTATQuestObjectiveTracker::CheatComplete()
{
}

void UTATQuestObjectiveTracker::_SetIsComplete(bool complete)
{
   _SetProgress(FTATQuestObjectiveState {.IsComplete = complete });
}

void UTATQuestObjectiveTracker::_SetProgress(FTATQuestObjectiveState newProgress)
{
   if (_firstStateUpdate || _state != newProgress)
   {
      _state = newProgress;
      _firstStateUpdate = false;
      OnProgressChanged.ExecuteIfBound(newProgress);
   }
}
