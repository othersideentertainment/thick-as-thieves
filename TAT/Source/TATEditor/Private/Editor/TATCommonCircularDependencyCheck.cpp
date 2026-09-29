// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Editor/TATCommonCircularDependencyCheck.h"

// tat
#include "TATEditorModuleSettings.h"

namespace TATCommonCircularDependencyCheck
{

static FString GetCyclicDependencyChainString(const FName rootAsset, const FName destAsset, const TMap<FName, FName>& assetToReferencerMap)
{
   // For ease of use, DependencyChain contains the dependencies in reverse order
   // So we start at DestAsset and work backwards
   TArray<FName, TInlineAllocator<16>> dependencyChain;
   dependencyChain.Add(destAsset);

   FName cycleItem = assetToReferencerMap[destAsset];
   while (cycleItem != rootAsset)
   {
      dependencyChain.Add(cycleItem);

      cycleItem = assetToReferencerMap[cycleItem];
   }

   // Add in the starting asset: this is the last element in the array, but the root of the reference chain
   dependencyChain.Add(rootAsset);

   FString dependencyChainStr;
   dependencyChainStr.Reserve(1024);

   // Print the chain out in reverse order, starting with the root and ending with the dest
   for (int32 i = dependencyChain.Num() - 1; i >= 0; i--)
   {
      dependencyChainStr += dependencyChain[i].ToString();
      if (i > 0)
      {
         dependencyChainStr += TEXT(" -> ");
      }
   }

   return dependencyChainStr;
}

void FindBlueprintAssetCycles(const FAssetCircularDependencyCheckState& dependencyState, TArray<BPAssetCycleInfo>& outCycleInfo)
{
   int32 numberOfAssetsWithCircularDependencies = 0;

   // For each case where a BP class derives from a BP class, check if the parent has an asset dependency on the child
   // Do this by doing a breadth-first search of all the parent's dependencies and seeing if we ever end up loading the child class
   // If we do, we have a circular dependency that is potentially problematic
   for (const auto& classPair : dependencyState.BlueprintClassToSuperClassMap)
   {
      FString childClassName = classPair.Key;
      FString parentClassName = classPair.Value;

      // If the parent class name does not map to an asset,
      // then it's not a blueprint and is instead a native class
      // In that case, its dependencies don't matter
      if (!dependencyState.BlueprintClassToAssetMap.Contains(parentClassName))
      {
         continue;
      }

      // Figure out which assets load in the parent and the child BP classes
      FName childAsset = dependencyState.BlueprintClassToAssetMap[childClassName];
      FName parentAsset = dependencyState.BlueprintClassToAssetMap[parentClassName];

      // A map of which asset is being loaded: since this is a breadth-first search, tracing back using this map
      // will show us the shortest path from the parent to the child
      TMap<FName, FName> assetToReferencerMap;

      TArray<FName, TInlineAllocator<64>> workQueue;
      workQueue.Add(parentAsset);

      // NOTE: Maybe good to just list all cycles? Or is that too much noise w/ the dups?
      bool hasFoundCycle = false;
      while (!hasFoundCycle && !workQueue.IsEmpty())
      {
         FName workItem = workQueue.Pop();

         if (dependencyState.AssetsToDependenciesMap.Contains(workItem))
         {
            for (FName dependencyAsset : dependencyState.AssetsToDependenciesMap[workItem])
            {
               if (!assetToReferencerMap.Contains(dependencyAsset))
               {
                  assetToReferencerMap.Add(dependencyAsset, workItem);
                  workQueue.Add(dependencyAsset);

                  if (dependencyAsset == childAsset)
                  {
                     // We found a cycle, so figure out some information about it, then trace back through the dependency chain so we know exactly what caused it
                     // We'll give this to the caller who can then forward it to the user or log it
                     BPAssetCycleInfo cycleInfo;
                     cycleInfo.BPChildClassName = childClassName;
                     cycleInfo.BPParentClassName = parentClassName;
                     cycleInfo.FullCycleChainString = GetCyclicDependencyChainString(parentAsset, childAsset, assetToReferencerMap);

                     outCycleInfo.Add(cycleInfo);

                     numberOfAssetsWithCircularDependencies++;

                     hasFoundCycle = true;
                     break;
                  }
               }
            }
         }
      }
   }
}

}
