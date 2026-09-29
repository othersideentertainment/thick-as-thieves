// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/FixupAbsolutePaths.h"

// ue4
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimSequence.h"
#include "Curves/CurveBase.h"
#include "Engine/CurveTable.h"
#include "Engine/DataTable.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterialMask.h"
#include "Sound/SoundWave.h"
#include "Misc/MessageDialog.h"

DEFINE_LOG_CATEGORY_STATIC(LogFixupArtAbsolutePaths, Log, All); 

namespace FixupArtAbsolutePaths
{
   void FixupArtContentAbsolutePathsFromDir(const FString& dir)
   {
      FString packagePath;
      FString pathConvertError;
      FPackageName::TryConvertFilenameToLongPackageName(dir, packagePath, &pathConvertError);

      const bool kRecursive = true;
      const bool kIncludeOnlyOnDiskAssets = true;
      FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
      TArray<FAssetData> assetData;
      assetRegistryModule.Get().GetAssetsByPath(FName(*packagePath), assetData, kRecursive, kIncludeOnlyOnDiskAssets);

      TArray<FAssetData> assets;
      assets.SetNum(assetData.Num());
      for (const FAssetData& asset : assetData)
      {
         if (DoesArtContentHaveAssetImportData(asset))
         {
            assets.Emplace(asset);
         }
      }
      FixupArtContentAbsolutePathsFromSelection(assets);
   }

   void FixupArtContentAbsolutePathsFromCurrentSelection()
   {
      // Get selected assets.
      TArray<FAssetData> selections;
      GEditor->GetContentBrowserSelections(selections);
      FixupArtContentAbsolutePathsFromSelection(selections);
   }

   void FixupArtContentAbsolutePathsFromSelection(const TArray<FAssetData>& selection)
   {
      int totalAssetsAttempted = 0;
      int totalAssetsSuccess = 0;
      int totalAssetsFailure = 0;
      int totalAssetsSkipped = 0;
      int totalUnfixedAssetsSkipped = 0;
      TArray<FString> errors;
      for (const FAssetData& assetData : selection)
      {
         TArray<FFixupStats> statsArr;
         if (UObject* obj = assetData.GetAsset())
         {
            // this if/else chain is stupid, but there's no way to get at the asset import data in a generic way.  at least it's all named the same on all the classes so I can template it...?
            if (UAnimSequence* animSeq = Cast<UAnimSequence>(obj))
            {
               FixupArtContentAbsolutePathsForType<UAnimSequence>(animSeq, statsArr);
            }
            else if (USkeletalMesh* skelMesh = Cast<USkeletalMesh>(obj))
            {
               FixupArtContentAbsolutePathsForType<USkeletalMesh>(skelMesh, statsArr);
            }
            else if (UStaticMesh* staticMesh = Cast<UStaticMesh>(obj))
            {
               FixupArtContentAbsolutePathsForType<UStaticMesh>(staticMesh, statsArr);
            }
            else if (UTexture* tex = Cast<UTexture>(obj))
            {
               FixupArtContentAbsolutePathsForType<UTexture>(tex, statsArr);
            }
            else if (UMaterialInterface* mi = Cast<UMaterialInterface>(obj))
            {
               FixupArtContentAbsolutePathsForType<UMaterialInterface>(mi, statsArr);
            }
            else if (UPhysicalMaterialMask* physicsMatMask = Cast<UPhysicalMaterialMask>(obj))
            {
               FixupArtContentAbsolutePathsForType<UPhysicalMaterialMask>(physicsMatMask, statsArr);
            }
            else if (USoundWave* soundWave = Cast<USoundWave>(obj))
            {
               FixupArtContentAbsolutePathsForType<USoundWave>(soundWave, statsArr);
            }
            else if (UDataTable* dt = Cast<UDataTable>(obj))
            {
               FixupArtContentAbsolutePathsForType<UDataTable>(dt, statsArr);
            }
            else if (UCurveTable* ct = Cast<UCurveTable>(obj))
            {
               FixupArtContentAbsolutePathsForType<UCurveTable>(ct, statsArr);
            }
            else if (UCurveBase* cb = Cast<UCurveBase>(obj))
            {
               FixupArtContentAbsolutePathsForType<UCurveBase>(cb, statsArr);
            }
         }

         for (const FFixupStats& stat : statsArr)
         {
            switch (stat.State)
            {
            case FFixupStats::Success:
               {
                  totalAssetsSuccess += 1;
               }
               break;
            case FFixupStats::Skipped:
               {
                  totalAssetsSkipped += 1;
               }
               break;
            case FFixupStats::Error:
               {
                  totalAssetsFailure += 1;
               }
               break;
            }
            if (!stat.ErrorStr.IsEmpty())
               errors.Add(stat.ErrorStr);
            totalAssetsAttempted += stat.Attempted;
         }
      }

      FString attemptedText = FString::Printf(TEXT("Attempted fixups: %d"), totalAssetsAttempted);
      FString skippedText = FString::Printf(TEXT("Skipped fixups (total): %d"), totalAssetsSkipped);
      FString successfulText = FString::Printf(TEXT("Successful fixups: %d"), totalAssetsSuccess);
      FString failedText = FString::Printf(TEXT("Failed fixups: %d"), totalAssetsFailure);
      FString errorText;
      for (const FString& err : errors)
      {
         errorText += FString::Printf(TEXT("%s\n"), *err);
      }
      FString messageStr = FString::Printf(TEXT("%s\n%s\n%s\n%s\n\n%s"),
                           *attemptedText,
                           *skippedText,
                           *successfulText,
                           *failedText,
                           *errorText);
      
      FText messageText = FText::FromString(messageStr);
      FText titleText = FText::FromString(TEXT("Fixup Results"));
      FMessageDialog::Open(EAppMsgType::Type::Ok, messageText, titleText);
      UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("Errors Found Start:"));
      for (const FString& err : errors)
      {
         UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("%s"), *err);
      }
      UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("Errors Found End"));
   }

   template <class T>
   void FixupArtContentAbsolutePathsForType(T* objWithImportAssetData, TArray<FFixupStats>& statsArr)
   {
      IFileManager& fileManager = IFileManager::Get();
      const FString& kContentDirName = TEXT("Content");
      const FString& kProjectDir = FPaths::ProjectDir();
      const FString& kArtSrcDir = FPaths::Combine(kProjectDir, TEXT("ArtSrc"));
      const FString& kAudioSrcDir = FPaths::Combine(kProjectDir, TEXT("AudioSrc"));
      const FString& kArtContentRootDir = FPaths::Combine(kArtSrcDir, TEXT("SourceAssets"), kContentDirName);
      UAssetImportData* assetImportData = GetAssetImportData(objWithImportAssetData);

      if (assetImportData)
      {
         UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("Fixing up the import path for %s..."), *objWithImportAssetData->GetName());
         for (FAssetImportInfo::FSourceFile& sourceFile : assetImportData->SourceData.SourceFiles)
         {
            FFixupStats& stats = statsArr[statsArr.Add(FFixupStats())];
            stats.Attempted += 1;

            // relative file name is a lie -- it can be absolute, see the comment in the struct
            FString oldFilePath = *sourceFile.RelativeFilename;
            FString filePath = *sourceFile.RelativeFilename;

            // skip, it's already linked to something that exists
            const bool containsDriveLetter = filePath.Contains(TEXT(":/")) || filePath.Contains(TEXT(":\\"));
            if (!containsDriveLetter && fileManager.FileExists(*filePath))
            {
               UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("Already in the correct folder, skipping %s"), *filePath);
               stats.State = FFixupStats::Skipped;
               continue;
            }

            UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("Found non-existent file path %s, trying to fix it up..."), *filePath);

            // trim everything up through content
            // D:/dev/TAT_Raw/SourceAssets/Content/Art/Animation/Player/Blondie/AN_Blondie_Idle.fbx
            int idx = filePath.Find(kContentDirName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
            if (idx != INDEX_NONE)
            {
               // snip
               filePath.RemoveAt(0, idx + kContentDirName.Len() + 1);

               // replace with our relative path
               filePath = FPaths::Combine(kArtContentRootDir, filePath);
            }

            // does that file exist?
            if (!fileManager.FileExists(*filePath))
            {
               // no?  ok let's just see if we can find this file in the source asset trees
               TArray<FString> sourceDirs = { kArtSrcDir, kAudioSrcDir };
               TArray<FString> foundFiles;
               const FString fileName = FPaths::GetCleanFilename(filePath);
               for(const FString& sourceDir : sourceDirs)
               {
                  fileManager.FindFilesRecursive(foundFiles, *sourceDir, *fileName, true, false, false);
               }

               if (foundFiles.Num() == 1)
               {
                  // one file, let's assume this one is it!
                  filePath = foundFiles[0];
               }
               else if (foundFiles.Num() > 1)
               {
                  // hmmm we probably shouldn't make assumptions about assets that share identical names
                  FString errStr = FString::Printf(TEXT("%s (%s) found multiple possible remappings, can't choose one!"), *objWithImportAssetData->GetName(), *oldFilePath);
                  UE_LOG(LogFixupArtAbsolutePaths, Warning, TEXT("%s"), *errStr);
                  stats.State = FFixupStats::Error;
                  stats.ErrorStr = errStr;
                  continue;
               }
               else
               {
                  // still not found :(
                  FString errStr = FString::Printf(TEXT("%s (%s) file %s not found in the tree!"), *objWithImportAssetData->GetName(), *oldFilePath, *fileName);
                  UE_LOG(LogFixupArtAbsolutePaths, Warning, TEXT("%s"), *errStr);
                  stats.State = FFixupStats::Error;
                  stats.ErrorStr = errStr;
                  continue;
               }
            }

            // set relative path
            sourceFile.RelativeFilename = filePath;

            // mark dirty for save in editor
            objWithImportAssetData->MarkPackageDirty();

            UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("Done fixing up %s"), *objWithImportAssetData->GetName());
            UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("  - Was: %s"), *oldFilePath);
            UE_LOG(LogFixupArtAbsolutePaths, Log, TEXT("  - Now: %s"), *filePath);

            // done!
            stats.State = FFixupStats::Success;
         }
      }
      else
      {
         UE_LOG(LogFixupArtAbsolutePaths, Error, TEXT("%s has no asset import data!"), *objWithImportAssetData->GetName());
      }
   }

   bool DoesArtContentHaveAssetImportData(const FAssetData& asset)
   {
      return asset.GetClass()->IsChildOf<UAnimSequence>() ||
             asset.GetClass()->IsChildOf<USkeletalMesh>() ||
             asset.GetClass()->IsChildOf<UStaticMesh>() ||
             asset.GetClass()->IsChildOf<UTexture>() ||
             asset.GetClass()->IsChildOf<UMaterialInterface>() ||
             asset.GetClass()->IsChildOf<UPhysicalMaterialMask>() ||
             asset.GetClass()->IsChildOf<USoundWave>() ||
             asset.GetClass()->IsChildOf<UDataTable>() ||
             asset.GetClass()->IsChildOf<UCurveTable>() ||
             asset.GetClass()->IsChildOf<UCurveBase>();
   }
}
