// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATDummyObjective.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDummyObjective)

FTATObjectiveTrackerPayload FTATDummyObjectiveInfo::CreateTracker() const
{
   return FTATObjectiveTrackerPayload{ UTATDummyObjectiveTracker::StaticClass() };
}

void UTATDummyObjectiveTracker::CheatComplete()
{
   _SetIsComplete(true);
}
