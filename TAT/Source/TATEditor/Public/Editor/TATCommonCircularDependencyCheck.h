// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

namespace TATCommonCircularDependencyCheck
{
   struct BPAssetCycleInfo
   {
      FString BPChildClassName;
      FString BPParentClassName;
      FString FullCycleChainString;
   };

   struct FAssetCircularDependencyCheckState
   {
      TMap<FName, TArray<FName>> AssetsToDependenciesMap;
      TMap<FString, FName> BlueprintClassToAssetMap;
      TMap<FString, FString> BlueprintClassToSuperClassMap;
   };

   // Look for cases where a parent blueprint class has a cyclical asset dependency on a child class
   // Assumes that the user supplies the data in dependencyState based on the asset registry
   void FindBlueprintAssetCycles(const FAssetCircularDependencyCheckState& dependencyState, TArray<BPAssetCycleInfo>& outCycleInfo);
};
