// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "FileHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOSEAssetRegistryUtl, Log, All);

namespace OSEAssetRegistryUtl
{
   // Constructs asset registry search filter with given parameters
   FARFilter GenerateAssetRegistryFilter(TConstArrayView<FString>& assetSearchPaths, const FTopLevelAssetPath& classPathName);

   TArray<FAssetData> FindAssets(TConstArrayView<FString> assetSearchPaths, const FTopLevelAssetPath& classPathName);

   // Returns all assets of given type in specified directories (relative to content folder)
   template<class T>
   inline TArray<T*> FindAndLoadAssets(TConstArrayView<FString> assetSearchPaths)
   {
      TArray<FAssetData> assetDataList = FindAssets(assetSearchPaths, T::StaticClass()->GetClassPathName());

      TArray<T*> assets;
      for (FAssetData& assetData : assetDataList)
      {
         assets.Add(CastChecked<T>(assetData.GetAsset()));
      }
      return assets;
   }

   // Returns all assets of given type
   template<class T>
   inline TArray<T*> FindAndLoadAssets()
   {
      const TArray<FString> assetSearchPaths;
      return FindAndLoadAssets<T>(assetSearchPaths);
   }

   // Attempts to save changes to packages, either with source control (i.e. checkout packages) or without (i.e. mark writable)
   bool SavePackages(const TArray<UPackage*>& packagesToSave, bool noSourceControl = false);
}
