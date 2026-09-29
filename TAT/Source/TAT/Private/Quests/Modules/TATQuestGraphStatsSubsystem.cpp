// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Modules/TATQuestGraphStatsSubsystem.h"

#if WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphStatsSubsystem)

TOptional<float> UTATQuestGraphStatsSubsystem::GetLastSimulatedSpawnRate(TWeakObjectPtr<const UTATQuestGraphNode> node) const
{
   if (const float* found = _spawnRates.Find(node))
   {
      return *found;
   }

   return {};
}

void UTATQuestGraphStatsSubsystem::SetSpawnRates(const TMap<TWeakObjectPtr<const UTATQuestGraphNode>, int32>& spawnCounts, int simulationCount)
{
   _spawnRates.Reset();
   for (const TPair<TWeakObjectPtr<const UTATQuestGraphNode>, int32>& pair : spawnCounts)
   {
      const float chance = (100.f * pair.Value) / FMath::Max(simulationCount, 1);
      _spawnRates.Add(pair.Key, chance);
   }
}
#endif
