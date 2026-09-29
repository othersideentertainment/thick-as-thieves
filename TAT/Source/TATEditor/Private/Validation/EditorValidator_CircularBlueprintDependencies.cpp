// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_CircularBlueprintDependencies.h"

// tat
#include "TATEditorModuleSettings.h"
#include "Editor/TATCommonCircularDependencyCheck.h"

// ue5
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_CircularBlueprintDependencies)

#define LOCTEXT_NAMESPACE "AssetValidation"

using namespace TATCommonCircularDependencyCheck;

bool UEditorValidator_CircularBlueprintDependencies::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   const TArray<FString>& filePathsToSkip = UTATEditorModuleSettings::Get().FilePathPrefixesToSkipCircularAssetValidation;

   const FString pathName = asset->GetPathName(nullptr);
   return !filePathsToSkip.ContainsByPredicate([&pathName](const FString& pattern) { return pathName.Contains(pattern); });
}

// Find all the assets that are indirect dependencies of the starting asset
static void GetAllIndirectDependenciesForAsset(FAssetCircularDependencyCheckState& dependencyState, const FAssetData& startingAsset, FAssetRegistryModule& assetRegistryModule)
{
   TArray<FName, TInlineAllocator<64>> workQueue;
   workQueue.Add(startingAsset.PackageName);

   while (!workQueue.IsEmpty())
   {
      FName workItem = workQueue.Pop();

      const UE::AssetRegistry::EDependencyQuery dependencyQuery = UE::AssetRegistry::EDependencyQuery::Hard;
      const UE::AssetRegistry::EDependencyCategory dependencyCategory = UE::AssetRegistry::EDependencyCategory::Package;

      TArray<FName> assetDependencies;
      assetRegistryModule.Get().GetDependencies(workItem, assetDependencies, dependencyCategory, dependencyQuery);

      dependencyState.AssetsToDependenciesMap.Emplace(workItem, assetDependencies);

      for (FName dependency : assetDependencies)
      {
         // If it hasn't been visited yet, also explore its dependencies
         if (!dependencyState.AssetsToDependenciesMap.Contains(dependency))
         {
            workQueue.Add(dependency);
         }
      }
   }
}

static void FillOutBlueprintClassAndParentClassTagData(FAssetCircularDependencyCheckState& dependencyState, FAssetRegistryModule& assetRegistryModule)
{
   TArray<FAssetData> assetDataAtPath;
   for (const auto& assetPair : dependencyState.AssetsToDependenciesMap)
   {
      assetDataAtPath.Reset();
      assetRegistryModule.Get().GetAssetsByPackageName(assetPair.Key, assetDataAtPath, true);

      // If we got any results, just take the first one: it'll be the blueprint asset we're interested in
      if (assetDataAtPath.Num() > 0)
      {
         FAssetData assetData = assetDataAtPath[0];

         FString GeneratedClassName;
         if (assetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassName))
         {
            dependencyState.BlueprintClassToAssetMap.Emplace(GeneratedClassName, assetPair.Key);

            FString ParentClassName;
            if (assetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassName))
            {
               dependencyState.BlueprintClassToSuperClassMap.Emplace(GeneratedClassName, ParentClassName);
            }
         }
      }
   }
}

EDataValidationResult UEditorValidator_CircularBlueprintDependencies::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

   FAssetCircularDependencyCheckState checkState;

   // Compute all assets that are direct or indirect dependencies of the starting asset
   // Using that, we can determine if the newly saved asset is potentially part of a problematic asset cycle where a base class
   // has a child class as a dependency
   // Because if we've created a cycle, we will be able to reach every asset in the cycle by exploring our own dependency graph
   GetAllIndirectDependenciesForAsset(checkState, assetData, assetRegistryModule);

   // Once we've gotten the list of assets and their dependencies, we also look for asset tags to find the Blueprint generated class, and parent class
   // for each asset. We can use this to determine if a parent class has a cyclical dependency on its child classes
   FillOutBlueprintClassAndParentClassTagData(checkState, assetRegistryModule);

   TArray<BPAssetCycleInfo> bpAssetCycles;
   FindBlueprintAssetCycles(checkState, bpAssetCycles);

   for (const auto& bpAssetCycle : bpAssetCycles)
   {
      AssetFails(asset, FText::Format(
         LOCTEXT("EditorValidator_CircularBlueprintDependencies",
            "Blueprint {0} has an asset dependency on {1} which is a blueprint subclass of it. This can cause bugs during loading. "
            "Full dependency chain: {2}"),
         FText::FromString(bpAssetCycle.BPParentClassName),
         FText::FromString(bpAssetCycle.BPChildClassName),
         FText::FromString(bpAssetCycle.FullCycleChainString)));
   }

   if (bpAssetCycles.Num() > 0)
   {
      for (const auto& bpAssetCycle : bpAssetCycles)
      {
         AssetFails(asset, FText::Format(
            LOCTEXT("EditorValidator_CircularBlueprintDependencies",
               "Blueprint {0} has an asset dependency on {1} which is a blueprint subclass of it. This can cause bugs during loading. "
               "Full dependency chain: {2}"),
            FText::FromString(bpAssetCycle.BPParentClassName),
            FText::FromString(bpAssetCycle.BPChildClassName),
            FText::FromString(bpAssetCycle.FullCycleChainString)));
      }

      return EDataValidationResult::Invalid;
   }
   else
   {
      AssetPasses(asset);
      return EDataValidationResult::Valid;
   }
}


