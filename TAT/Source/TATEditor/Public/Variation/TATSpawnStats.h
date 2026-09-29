// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UTATQuestGraphNode;
class UTATSceneSetAsset;
class UTATSceneVariantConfig;
class UTATSpawnerComponent;
struct FTATScenePhaseSummary;
struct FTATSpawnPlan;
struct FTATQuestActorSpawn;
struct FTATQuestChoicePlan;

struct FTATRawSpawnStats
{
   void Append(const FTATSpawnPlan& plan);
   void WriteToCsv(const FString& name) const;

   TMap<TWeakObjectPtr<UTATSpawnerComponent>, TBitArray<>> SpawnSequences;
};

struct FTATSpawnerCountStats
{
   void Append(const FTATSpawnPlan& plan);
   void WriteToCsv(const FString& name) const;
   void WriteToLog(FMessageLog& msgLog) const;

   int32 SimulationCount = 0;
   TMap<TWeakObjectPtr<UTATSpawnerComponent>, int32> SpawnCounts;

private:
   float _ComputeChance(const TPair<TWeakObjectPtr<UTATSpawnerComponent>, int32>& entry) const;
};

struct FTATQuestCountStats
{
   void Append(const TArray<FTATQuestActorSpawn>& plan);
   void WriteToLog(FMessageLog& msgLog) const;

   int32 SimulationCount = 0;
   TMap<TWeakObjectPtr<UActorComponent>, int32> SpawnCounts;

private:
   float _ComputeChance(const TPair<TWeakObjectPtr<UActorComponent>, int32>& entry) const;
};

// Basic stats about spawn group counts, excluding histograms/percentiles
struct FTATSpawnGroupCountStats
{
   void Append(const FTATSpawnPlan& plan);
   void WriteToLog(FMessageLog& msgLog) const;

   struct FGroupStatsEntry
   {
      FGroupStatsEntry() = default;
      FGroupStatsEntry(FGameplayTag tag) : GroupTag(tag)
      {}

      bool operator==(const FGameplayTag& tag) { return GroupTag == tag; }

      FGameplayTag GroupTag;
      int32 BelowMinCount = 0;
      int32 AboveMaxCount = 0;
      int32 BelowDesiredCount = 0;
      int32 Sum = 0;
      int32 Max = 0;
      int32 Min = std::numeric_limits<int32>::max();
   };

   int32 SimulationCount = 0;
   TArray<FGroupStatsEntry> GroupStats;
};

struct FTATVariantFreqencyStats
{
   void Append(TConstArrayView<const UTATSceneVariantConfig*> variants);
   void WriteVariantToLog(const UTATSceneVariantConfig* variant, FMessageLog& msgLog) const;

   int32 SimulationCount = 0;
   TMap<TWeakObjectPtr<const UTATSceneVariantConfig>, int32> VariantCounts;
};

struct FTATScenePhaseStats
{
   void Append(TConstArrayView<FTATScenePhaseSummary> phases);
   void WriteToLog(FMessageLog& msgLog) const;

   struct FPhaseStatsEntry
   {
      TWeakObjectPtr<const UTATSceneSetAsset> SceneSet;
      FGameplayTag PhaseTag;
      int32 BelowMinCount = 0;
      int32 AboveMaxCount = 0;
      int32 BelowDesiredCount = 0;
      int32 Sum = 0;
      int32 Max = 0;
      int32 Min = std::numeric_limits<int32>::max();
   };

   int32 SimulationCount = 0;
   TArray<FPhaseStatsEntry> PhaseStats;
};

template<typename TElem>
struct TTATFrequencyStats
{
   // just to allow strong->weak implicit conv to happen here
   template<typename TInput>
   void Append(const TInput& elems)
   {
      for (const typename TInput::ElementType& elem : elems)
      {
         Counts.FindOrAdd(elem) += 1;
      }
      SimulationCount++;
   }

   int32 SimulationCount = 0;
   TMap<TElem, int32> Counts;
};
using FTATQuestGraphNodeStats = TTATFrequencyStats<TWeakObjectPtr<const UTATQuestGraphNode>>;

struct FTATTagFrequencyStats
{
   void Append(const FGameplayTagContainer& tagContainer);
   void WriteToLog(FMessageLog& msgLog) const;

   TMap<FGameplayTag, int32> ChoiceCounts;
   int32 SimulationCount = 0;
};
