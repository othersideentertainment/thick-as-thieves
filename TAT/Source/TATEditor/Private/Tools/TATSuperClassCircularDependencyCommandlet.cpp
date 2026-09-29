// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATSuperClassCircularDependencyCommandlet.h"

// tat
#include "TATEditorModuleSettings.h"
#include "Editor/TATCommonCircularDependencyCheck.h"

// ue4
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/FeedbackContext.h"
#include "Settings/ProjectPackagingSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSuperClassCircularDependencyCommandlet)


DEFINE_LOG_CATEGORY_STATIC(LogTATSuperClassCircularDependencyCommandlet, Log, All);

int UTATSuperClassCircularDependencyCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   int32 numberOfAssetsWithCircularDependencies = _RunValidationOnAssets();
   return numberOfAssetsWithCircularDependencies;
}

bool UTATSuperClassCircularDependencyCommandlet::_ShouldSkipAssetPath(const FString& assetPath) const
{
   for (const FString& prefixToSkip : UTATEditorModuleSettings::Get().FilePathPrefixesToSkipCircularAssetValidation)
   {
      if (assetPath.StartsWith(prefixToSkip))
      {
         return true;
      }
   }

   return false;
}

int32 UTATSuperClassCircularDependencyCommandlet::_RunValidationOnAssets()
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

   FARFilter arFilter;
   arFilter.bIncludeOnlyOnDiskAssets = true;
   arFilter.bRecursiveClasses = true;
   arFilter.bRecursivePaths = true;
   arFilter.PackagePaths.Emplace(TEXT("/Game"));

   TArray<FAssetData> assetList;
   assetRegistryModule.Get().GetAssets(arFilter, assetList);
   
   TATCommonCircularDependencyCheck::FAssetCircularDependencyCheckState dependencyCheckState;

   for (int32 assetIdx = 0; assetIdx < assetList.Num(); ++assetIdx)
   {
      const FAssetData& assetData = assetList[assetIdx];
      const FName assetName = assetData.PackageName;
      const FString objectPath = assetData.GetObjectPathString();

      // Check if this is a path we don't want to bother dealing with
      if (_ShouldSkipAssetPath(objectPath))
      {
         continue;
      }
   
      UE_LOG(LogTATSuperClassCircularDependencyCommandlet, Verbose, TEXT("Processing dependencies for asset (%i/%i): %s"), assetIdx, assetList.Num(), *objectPath);
   
      // Get all hard package dependencies, which we can use to find indirect dependencies between assets
      const UE::AssetRegistry::EDependencyQuery dependencyQuery = UE::AssetRegistry::EDependencyQuery::Hard;
      const UE::AssetRegistry::EDependencyCategory dependencyCategory = UE::AssetRegistry::EDependencyCategory::Package;
      TArray<FName> dependencies;
      assetRegistryModule.Get().GetDependencies(assetName, dependencies, dependencyCategory, dependencyQuery);

      dependencyCheckState.AssetsToDependenciesMap.Emplace(assetName, dependencies);

      // Get the generated class name (if we're a BP class we will have one), and the parent class name
      // We use these to fill out the Child -> Parent class name map, and the BPClass -> asset name map
      // Note that the parent may be a native class, but if so it will not end up in blueprintClassToAssetMap
      FString GeneratedClassName;
      if (assetData.GetTagValue(FBlueprintTags::GeneratedClassPath, GeneratedClassName))
      {
         dependencyCheckState.BlueprintClassToAssetMap.Emplace(GeneratedClassName, assetName);

         FString ParentClassName;
         if (assetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassName))
         {
            dependencyCheckState.BlueprintClassToSuperClassMap.Emplace(GeneratedClassName, ParentClassName);
         }
      }
   }

   // Now that we've computed dependency information and blueprint class/parent class info for each asset,
   // we check if it any class has a dependency on one of its child classes
   TArray<TATCommonCircularDependencyCheck::BPAssetCycleInfo> assetCycles;
   TATCommonCircularDependencyCheck::FindBlueprintAssetCycles(dependencyCheckState, assetCycles);

   for (const auto& assetCycle : assetCycles)
   {
      UE_LOG(LogTATSuperClassCircularDependencyCommandlet, Warning,
         TEXT("Found cycle in assets with class '%s', which is a subclass of class '%s'. ")
         TEXT("The parent has an asset dependency on the child, which can cause issues during loading."),
         *assetCycle.BPChildClassName, *assetCycle.BPParentClassName);

      UE_LOG(LogTATSuperClassCircularDependencyCommandlet, Warning, TEXT("Full dependency chain: %s"), *assetCycle.FullCycleChainString);
   }

   return assetCycles.Num();
}

