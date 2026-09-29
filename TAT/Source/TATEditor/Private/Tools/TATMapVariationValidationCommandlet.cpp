// (c) 2021-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATMapVariationValidationCommandlet.h"

// tat
#include "Editor/TATWorldAssetTags.h"
#include "GameFramework/TATWorldSettings.h"
#include "Variation/MapVariationValidationUtl.h"

// ose

// ue4
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "Logging/MessageLog.h"
#include "Misc/FeedbackContext.h"
#include "Settings/ProjectPackagingSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMapVariationValidationCommandlet)

UTATMapVariationValidationCommandlet::UTATMapVariationValidationCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

int UTATMapVariationValidationCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{

   TArray<FString> tokens, switches;
   ParseCommandLine(*fullCommandLine, tokens, switches);


   const bool onlyCooked = !switches.Contains(TEXT("all"));
   _RunValidation(onlyCooked);
   return 0;
}

void UTATMapVariationValidationCommandlet::_RunValidation(bool onlyCooked)
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

   FARFilter arFilter;
   arFilter.bRecursivePaths = true;
   arFilter.PackagePaths.Emplace(TEXT("/Game/Maps"));
   arFilter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
   arFilter.TagsAndValues.AddUnique(TATWorldAssetTags::kLevelIsRandomized, TOptional<FString>(TEXT("True")));

   TArray<FAssetData> assetList;
   assetRegistryModule.Get().GetAssets(arFilter, assetList);

   UProjectPackagingSettings* packagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
   check(packagingSettings);

   FMessageLog messageLog(MapVariationValidationHelper::kValidationLogName);

   for (const FAssetData& assetData : assetList)
   {
      const FString longPackageName = assetData.GetObjectPathString();

      // Only check maps explicitly in the cook list for parity with OSEMapCheck
      // TODO: this does not include maps that are indirectly pulled into the cook by other assets, which should probably be improved
      if (onlyCooked && !packagingSettings->MapsToCook.ContainsByPredicate([&longPackageName](const FFilePath& path) { return longPackageName.StartsWith(path.FilePath); }))
      {
         UE_LOG(LogOSECommandlet, Display, TEXT("Skipping Uncooked Map: %s"), *longPackageName);
         continue;
      }

      UE_LOG(LogOSECommandlet, Display, TEXT("Loading Map: %s"), *longPackageName);

      if (FEditorFileUtils::LoadMap(*longPackageName))
      {
         UWorld* world = GWorld;
         check(world);

         ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
         if (worldSettings == nullptr || worldSettings->SpawnData == nullptr)
         {
            // should only happen if the asset registry is out of sync
            UE_LOG(LogOSECommandlet, Warning, TEXT("Skipping world without spawners %s..."), *assetData.AssetName.ToString());
            continue;
         }

         UE_LOG(LogOSECommandlet, Display, TEXT("Validating spawners in world %s..."), *assetData.AssetName.ToString());

         MapVariationValidationHelper::ValidateSpawnConfig(*worldSettings->SpawnData, world, messageLog);
      }
      else
      {
         UE_LOG(LogOSECommandlet, Error, TEXT("Failed to load world %s"), *longPackageName);
      }
   }
}

