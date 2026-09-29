// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/TATSpawnerFwd.h"

// ue5
#include "CoreMinimal.h"

struct FTATSpawnGroup;
struct FTATSceneVariantCollection;

struct FTATSpawnGenerationParams
{
   TConstArrayView<FTATSpawnGroup> SpawnGroups;
   TConstArrayView<UTATSpawnerComponent*> Spawners;
   TConstArrayView<const AActor*> ExternalSpawners;
   const FTATSceneVariantCollection* SceneVariants = nullptr;
   int32 Seed = 0;

   FTATSpawnGenerationParams() = default;
   UE_NONCOPYABLE(FTATSpawnGenerationParams);
};

namespace SpawnHelpers
{
   void TAT_API GeneratePlan(FTATSpawnPlan& outPlan, const FTATSpawnGenerationParams& params);
   void TAT_API ValidateSpawners(TConstArrayView<FTATSpawnGroup> spawnGroups, TConstArrayView<UTATSpawnerComponent*> spawners, FMessageLog& msgLog);
}
