// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

namespace FixupArtAbsolutePaths
{
   struct FFixupStats
   {
      enum EFixupState
      {
         Success,
         Skipped,
         Error,
      };
      EFixupState State = EFixupState::Error;
      FString ErrorStr;
      int Attempted = 0;
   };

   void FixupArtContentAbsolutePathsFromDir(const FString& dir);
   void FixupArtContentAbsolutePathsFromCurrentSelection();
   void FixupArtContentAbsolutePathsFromSelection(const TArray<FAssetData>& selection);
   template <class T>
   inline UAssetImportData* GetAssetImportData(T* objWithImportAssetData)
   {
      return objWithImportAssetData->AssetImportData;
   }
   template <>
   inline UAssetImportData* GetAssetImportData(USkeletalMesh* objWithImportAssetData)
   {
      return objWithImportAssetData->GetAssetImportData();
   }

   template <class T>
   void FixupArtContentAbsolutePathsForType(T* objWithImportAssetData, TArray<FFixupStats>& statsArr);
   bool DoesArtContentHaveAssetImportData(const FAssetData& asset);
}
