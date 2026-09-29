// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATSpawnStats.h"

// tat
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/TATSpawnPlan.h"
#include "Quests/Spawn/TATQuestSpawnTypes.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/FileHelper.h"
#include "Misc/UObjectToken.h"

namespace StatHelpers
{
   static void WriteToCsvFile(const TCHAR* prefix, const FString& name, const TArray<FString>& lines)
   {
      FDateTime now = FDateTime::Now();
      FString fileName = FString::Printf(TEXT("%s_%s_%d_%d-%d-%d_%d-%d-%d.csv"),
         prefix,
         *name,
         FEngineVersion::Current().GetChangelist(),
         now.GetMonth(),
         now.GetDay(),
         now.GetYear(),
         now.GetHour(),
         now.GetMinute(),
         now.GetSecond());

      // file path
      FString filePath = FPaths::Combine(FPlatformMisc::ProjectDir(), TEXT("Saved"), TEXT("Spawners"), fileName);

      // write
      FFileHelper::SaveStringArrayToFile(lines, *filePath);
   }
}

void FTATRawSpawnStats::Append(const FTATSpawnPlan& plan)
{
   for (const FTATSpawnPlanEntry& entry : plan.Spawns)
   {
      SpawnSequences.FindOrAdd(entry.Spawner).Add(entry.DidSpawn);
   }
}

void FTATRawSpawnStats::WriteToCsv(const FString& name) const
{
   TArray<FString> lineArray;
   lineArray.Reserve(SpawnSequences.Num());

   for (const TPair<TWeakObjectPtr<UTATSpawnerComponent>, TBitArray<>>& pair : SpawnSequences)
   {
      const UTATSpawnerComponent* spawner = pair.Key.Get();
      if (!ensure(spawner))
      {
         continue;
      }
      
      FString& newLine = lineArray.Emplace_GetRef();
      newLine.Append(spawner->GetReadableName());
      newLine.Reserve(newLine.Len() + (pair.Value.Num() * 2));

      for (TBitArray<>::FConstIterator it(pair.Value); it; ++it)
      {
         newLine.AppendChar(',');
         newLine.AppendChar(it.GetValue() ? '1' : '0');
      }
   }

   StatHelpers::WriteToCsvFile(TEXT("rawspawners"), name, lineArray);
}

void FTATSpawnerCountStats::Append(const FTATSpawnPlan& plan)
{
   for (const FTATSpawnPlanEntry& entry : plan.Spawns)
   {
      // want to make sure to create the entry
      SpawnCounts.FindOrAdd(entry.Spawner) += (entry.DidSpawn ? 1 : 0);
   }
   SimulationCount++;
}

void FTATSpawnerCountStats::WriteToCsv(const FString& name) const
{
   TArray<FString> lineArray;
   lineArray.Reserve(SpawnCounts.Num());

   for (const TPair<TWeakObjectPtr<UTATSpawnerComponent>, int32>& pair : SpawnCounts)
   {
      const UTATSpawnerComponent* spawner = pair.Key.Get();
      if (!ensure(spawner))
      {
         continue;
      }

      FString& newLine = lineArray.Emplace_GetRef();
      newLine.Append(pair.Key->GetReadableName());
      newLine.AppendChar(',');
      newLine.Appendf(TEXT("%.2f"), _ComputeChance(pair));
   }

   StatHelpers::WriteToCsvFile(TEXT("spawnerpercent"), name, lineArray);
}

void FTATSpawnerCountStats::WriteToLog(FMessageLog& msgLog) const
{
   for (const TPair<TWeakObjectPtr<UTATSpawnerComponent>, int32>& pair : SpawnCounts)
   {
      const UTATSpawnerComponent* spawner = pair.Key.Get();
      if (!ensure(spawner))
      {
         continue;
      }

      msgLog.Info()
         ->AddToken(FUObjectToken::Create(spawner->GetOwner(), FText::FromString(spawner->GetReadableName())))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%.2f%%"), _ComputeChance(pair)))));
   }
}

float FTATSpawnerCountStats::_ComputeChance(const TPair<TWeakObjectPtr<UTATSpawnerComponent>, int32>& pair) const
{
   return 100 * pair.Value / static_cast<float>(SimulationCount * FMath::Max(pair.Key->GetMaxInstancesToSpawn(), 1));
}

void FTATSpawnGroupCountStats::Append(const FTATSpawnPlan& plan)
{
   SimulationCount++;
   GroupStats.Reserve(plan.GroupSummaries.Num());
   for (const FTATSpawnGroupSummary& summary : plan.GroupSummaries)
   {
      FGroupStatsEntry* entry = GroupStats.FindByKey(summary.SpawnGroupTag);
      if (entry == nullptr)
      {
         entry = &GroupStats.Emplace_GetRef(summary.SpawnGroupTag);
      }

      entry->Sum += summary.NumSpawned;
      entry->Max = FMath::Max(entry->Max, summary.NumSpawned);
      entry->Min = FMath::Min(entry->Min, summary.NumSpawned);
      if (summary.NumSpawned < summary.MinSpawners)
      {
         entry->BelowMinCount += 1;
      }
      if (summary.NumSpawned < summary.NumDesired)
      {
         entry->BelowDesiredCount += 1;
      }
      if (summary.NumSpawned > summary.MaxSpawners)
      {
         entry->AboveMaxCount += 1;
      }
   }
}

void FTATSpawnGroupCountStats::WriteToLog(FMessageLog& msgLog) const
{
   for (const FGroupStatsEntry& entry : GroupStats)
   {
      const int32 mean = FMath::DivideAndRoundNearest(entry.Sum, SimulationCount);
      msgLog.Info(FText::FromString(FString::Format(TEXT("Group {0} Counts (Mean: {1} Lowest: {2} Highest: {3}) "),
         { entry.GroupTag.ToString(), mean, entry.Min, entry.Max })));

      if (entry.BelowMinCount > 0)
      {
         const int32 count = entry.BelowMinCount;
         const int32 percent = FMath::DivideAndRoundNearest(count * 100, SimulationCount);
         msgLog.Error(FText::FromString(FString::Format(TEXT("Group {0} spawned fewer than the minimum {1}% of the time ({2} total)"),
            { entry.GroupTag.ToString(), percent, count })));
      }
      if (entry.BelowDesiredCount > 0)
      {
         const int32 count = entry.BelowDesiredCount;
         const int32 percent = FMath::DivideAndRoundNearest(count * 100, SimulationCount);
         msgLog.Warning(FText::FromString(FString::Format(TEXT("Group {0} spawned fewer than desired {1}% of the time ({2} total)"),
            { entry.GroupTag.ToString(), percent, count })));
      }
      if (entry.AboveMaxCount > 0)
      {
         const int32 count = entry.AboveMaxCount;
         const int32 percent = FMath::DivideAndRoundNearest(count * 100, SimulationCount);
         msgLog.Warning(FText::FromString(FString::Format(TEXT("Group {0} spawned more than the max {1}% of the time ({2} total)"),
            { entry.GroupTag.ToString(), percent, count })));
      }
   }
}

void FTATVariantFreqencyStats::Append(TConstArrayView<const UTATSceneVariantConfig*> variants)
{
   SimulationCount += 1;
   for (const UTATSceneVariantConfig* variant : variants)
   {
      VariantCounts.FindOrAdd(variant) += 1;
   }
}

void FTATVariantFreqencyStats::WriteVariantToLog(const UTATSceneVariantConfig* variant, FMessageLog& msgLog) const
{
   if(variant == nullptr) return;

   const int32 count = VariantCounts.FindRef(variant);
   const float chance = 100 * count / static_cast<float>(SimulationCount);
   msgLog.Info()
      ->AddToken(FUObjectToken::Create(variant))
      ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%.2f%%"), chance))));
}

void FTATScenePhaseStats::Append(TConstArrayView<FTATScenePhaseSummary> phases)
{
   SimulationCount++;
   PhaseStats.Reserve(phases.Num());
   for (const FTATScenePhaseSummary& summary : phases)
   {
      FPhaseStatsEntry* entry = PhaseStats.FindByPredicate([&summary](const FPhaseStatsEntry& e) {return e.SceneSet == summary.SceneSet && e.PhaseTag == summary.PhaseTag;});
      if (entry == nullptr)
      {
         entry = &PhaseStats.Emplace_GetRef();
         entry->SceneSet = summary.SceneSet;
         entry->PhaseTag = summary.PhaseTag;
      }

      entry->Sum += summary.NumChosen;
      entry->Max = FMath::Max(entry->Max, summary.NumChosen);
      entry->Min = FMath::Min(entry->Min, summary.NumChosen);
      if (summary.NumChosen < summary.Min)
      {
         entry->BelowMinCount += 1;
      }
      if (summary.NumChosen < summary.NumDesired)
      {
         entry->BelowDesiredCount += 1;
      }
      if (summary.NumChosen > summary.Max)
      {
         entry->AboveMaxCount += 1;
      }
   }
}

void FTATScenePhaseStats::WriteToLog(FMessageLog& msgLog) const
{
   for (const FPhaseStatsEntry& entry : PhaseStats)
   {
      const float mean = entry.Sum / (float)SimulationCount;
      msgLog.Info(FText::FromString(FString::Printf(TEXT("Phase %s-%s Counts (Mean: %.1f Lowest: %d Highest: %d)"),
         *GetNameSafe(entry.SceneSet.Get()), *entry.PhaseTag.ToString(), mean, entry.Min, entry.Max)));

      if (entry.BelowMinCount > 0)
      {
         const int32 count = entry.BelowMinCount;
         const int32 percent = FMath::DivideAndRoundNearest(count * 100, SimulationCount);
         msgLog.Error(FText::FromString(FString::Format(TEXT("Phase {0}-{1} spawned fewer than the minimum {2}% of the time ({3} total)"),
            { GetNameSafe(entry.SceneSet.Get()), entry.PhaseTag.ToString(), percent, count })));
      }
      if (entry.BelowDesiredCount > 0)
      {
         const int32 count = entry.BelowDesiredCount;
         const int32 percent = FMath::DivideAndRoundNearest(count * 100, SimulationCount);
         msgLog.Warning(FText::FromString(FString::Format(TEXT("Phase {0}-{1} spawned fewer than desired {2}% of the time ({3} total)"),
            { GetNameSafe(entry.SceneSet.Get()), entry.PhaseTag.ToString(), percent, count })));
      }
      if (entry.AboveMaxCount > 0)
      {
         const int32 count = entry.AboveMaxCount;
         const int32 percent = FMath::DivideAndRoundNearest(count * 100, SimulationCount);
         msgLog.Warning(FText::FromString(FString::Format(TEXT("Phase {0}-{1} spawned more than the max {2}% of the time ({3} total)"),
            { GetNameSafe(entry.SceneSet.Get()), entry.PhaseTag.ToString(), percent, count })));
      }
   }
}

void FTATQuestCountStats::Append(const TArray<FTATQuestActorSpawn>& plan)
{
   for (const FTATQuestActorSpawn& entry : plan)
   {
      // want to make sure to create the entry
      SpawnCounts.FindOrAdd(entry.Spawner) += 1;
   }
   SimulationCount++;
}

void FTATQuestCountStats::WriteToLog(FMessageLog& msgLog) const
{
   for (const TPair<TWeakObjectPtr<UActorComponent>, int32>& pair : SpawnCounts)
   {
      const UActorComponent* spawner = pair.Key.Get();
      if (!ensure(spawner))
      {
         continue;
      }

      const float chance = 100 * pair.Value / static_cast<float>(SimulationCount);
      msgLog.Info()
         ->AddToken(FUObjectToken::Create(spawner->GetOwner(), FText::FromString(spawner->GetReadableName())))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%.2f%%"), chance))));
   }
}

void FTATTagFrequencyStats::Append(const FGameplayTagContainer& tagContainer)
{
   SimulationCount++;
   for (const FGameplayTag& choice : tagContainer)
   {
      int32 choiceCount = 1;
      if (int32* existingEntry = ChoiceCounts.Find(choice))
      {
         choiceCount = *existingEntry + 1;
      }
      ChoiceCounts.Emplace(choice, choiceCount);
   }
}

void FTATTagFrequencyStats::WriteToLog(FMessageLog& msgLog) const
{
   // Iterate over sorted tags so mutually-exclusive/related choices are (probably) presented alongside one another
   TArray<FGameplayTag> choiceArray;
   ChoiceCounts.GenerateKeyArray(choiceArray);
   choiceArray.Sort();
   for (FGameplayTag choice : choiceArray)
   {
      const int32 count = ChoiceCounts[choice];
      const float percent = (count / static_cast<float>(SimulationCount)) * 100.f;
      msgLog.Info()
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%s"), *choice.ToString()))))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("%.2f%%"), percent))));
   }
}
