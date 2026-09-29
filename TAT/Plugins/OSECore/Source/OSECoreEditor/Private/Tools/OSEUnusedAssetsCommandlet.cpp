// (c) 2023-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEUnusedAssetsCommandlet.h"

// ue5
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUnusedAssetsCommandlet)

namespace UnusedAssetsHelpers
{
   static bool ShouldSkipPackage(const FAssetData& assetData)
   {
      const TCHAR* ignoredFolderRoots[] =
      {
         TEXT("/Game/__ExternalActors__/"),
         TEXT("/Game/__ExternalObjects__/")
      };

      const FString& packagePath = assetData.PackagePath.ToString();
      for (const TCHAR* folderRoot : ignoredFolderRoots)
      {
         if (packagePath.StartsWith(folderRoot))
         {
            return true;
         }
      }

      return false;
   };
}

UOSEUnusedAssetsCommandlet::UOSEUnusedAssetsCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

int UOSEUnusedAssetsCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   _FindUnusedAssets();

   return 0;
}

void UOSEUnusedAssetsCommandlet::_FindUnusedAssets()
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
   IAssetRegistry& assetRegistry = assetRegistryModule.Get();

   FARFilter assetFilter;
   assetFilter.PackagePaths.Add(TEXT("/Game"));
   assetFilter.bRecursivePaths = true;

   TArray<FAssetData> assetDataList;
   assetRegistry.GetAssets(assetFilter, assetDataList);

   TArray<FAssetIdentifier> foundReferencers;

   int32 unusedAssetCount = 0;

   // get rid of anything that we don't need based on our static filters
   for (const FAssetData& assetData : assetDataList)
   {
      if (UnusedAssetsHelpers::ShouldSkipPackage(assetData))
      {
         continue;
      }

      foundReferencers.Reset();
      assetRegistry.GetReferencers(assetData.PackageName, foundReferencers);

      if (foundReferencers.Num() == 0)
      {
         UE_LOG(LogOSECommandlet, Display, TEXT("Asset with no references: %s"), *assetData.PackageName.ToString());
         unusedAssetCount++;
      }
   }

   UE_LOG(LogOSECommandlet, Display, TEXT("Found %d unreferenced assets"), unusedAssetCount);
   UE_LOG(LogOSECommandlet, Display, TEXT("(reminder this only counts references from other assets)"));
}
