// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class UTATQuestActorSpawnerComponent;

struct FTATQuestActorSpawn;
struct FTATQuestActorSpawnRequest;
struct FTATSceneVariantCollection;

using FTATQuestSpawnPlan = TArray<FTATQuestActorSpawn>;

struct TAT_API FTATQuestSpawnPlanParams
{
   TConstArrayView<FTATQuestActorSpawnRequest> SpawnRequests;
   TConstArrayView<TObjectPtr<UTATQuestActorSpawnerComponent>> QuestSpawners;
   const FTATSceneVariantCollection* Variants = nullptr;
   int32 Seed = 0;

   FTATQuestSpawnPlanParams() = default;
   UE_NONCOPYABLE(FTATQuestSpawnPlanParams);
};

namespace TATQuestSpawnUtils
{
   void TAT_API GeneratePlan(FTATQuestSpawnPlan& outPlan, const FTATQuestSpawnPlanParams& params);
   void TAT_API ExecutePlan(const FTATQuestSpawnPlan& plan);
   void TAT_API EmitSpawnerActors(TArray<const AActor*>& outActors, const FTATQuestSpawnPlan& plan);

#if WITH_EDITOR
   // Separate (more expensive) code-path for scraping for quest spawners for validation/sim
   // Largely because UTATActiveQuestSubsystem doesn't exist then. Not worried about the extra
   // expense, but is it saving much to do this?
   void TAT_API ScrapeSpawnersInLevel(UWorld* world, TArray<TObjectPtr<UTATQuestActorSpawnerComponent>>& outSpawners);
#endif
}
