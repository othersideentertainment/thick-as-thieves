// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/TATSpawnerFwd.h"

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATSpawnPlan.generated.h"

USTRUCT()
struct TAT_API FTATSpawnPlanEntry
{
   GENERATED_BODY()

   FTATSpawnPlanEntry() = default;

   FTATSpawnPlanEntry(UTATSpawnerComponent* spawner)
   : Spawner(spawner)
   {
   }

   UPROPERTY()
   TObjectPtr<UTATSpawnerComponent> Spawner = nullptr;

   UPROPERTY()
   TSoftClassPtr<AActor> ClassToSpawn;

   int32 RandomSeed = 0;
   uint16 ModifierCount = 0;
   bool DidSpawn = false;
};

// technically redundant, and not used for gameplay, but light enough that probably fine
struct TAT_API FTATSpawnGroupSummary
{
   FTATSpawnGroupSummary(FGameplayTag groupTag)
      : SpawnGroupTag(groupTag)
   {
   }

   FGameplayTag SpawnGroupTag;
   int32 NumSpawned = 0;
   int32 NumDesired = 0;
   int32 PossibleSpawners = 0;
   int32 MinSpawners = 0;
   int32 MaxSpawners = 0;
};

USTRUCT()
struct TAT_API FTATSpawnPlan
{
   GENERATED_BODY()

   UPROPERTY()
   TArray<FTATSpawnPlanEntry> Spawns;

   UPROPERTY()
   TArray<TObjectPtr<UTATSpawnModifier>> Modifiers;

   TArray<FTATSpawnGroupSummary> GroupSummaries;

   void Reset();

   void Execute(ETATSpawnTiming spawnTiming) const;
   TSet<FSoftObjectPath> CollectPreloadAssets() const;

   void EmitNotSpawn(UTATSpawnerComponent* spawner, int32 randomSeed)
   {
      Spawns.Emplace_GetRef(spawner).RandomSeed = randomSeed;
   }

   void LogGroupSummaries(FMessageLog& messageLog) const;
};

// Partial progress for a spawn execution
struct FTATSpawnCursor
{
   FTATSpawnCursor(ETATSpawnTiming timing) : _timing(timing)
   { }

   bool IsValid(const FTATSpawnPlan& plan) const;
   void Step(const FTATSpawnPlan& plan);

   void Execute(const FTATSpawnPlan& plan);
private:
   ETATSpawnTiming _timing;
   int _index = 0;
   int _modifierIndex = 0;
};


