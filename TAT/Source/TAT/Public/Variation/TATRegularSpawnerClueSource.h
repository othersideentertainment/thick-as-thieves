// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

struct FTATPendingClueSource;
struct FTATSpawnPlan;

namespace TATRegularSpawnerClueSource
{
   void TAT_API AddClueSourcesFromSpawnPlan(const FTATSpawnPlan& plan, TArray<FTATPendingClueSource>& outSources, int32 seed);
};
