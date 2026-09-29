// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEDuplicateWorldSettingsFixupCommandlet.h"

// ose

// ue4
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/FeedbackContext.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDuplicateWorldSettingsFixupCommandlet)

UOSEDuplicateWorldSettingsFixupCommandlet::UOSEDuplicateWorldSettingsFixupCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

int UOSEDuplicateWorldSettingsFixupCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   _RunFixup();
   return 0;
}

void UOSEDuplicateWorldSettingsFixupCommandlet::_RunFixup()
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
   TArray<FAssetData> worldAssets;
   assetRegistryModule.Get().GetAssetsByClass(UWorld::StaticClass()->GetClassPathName(), worldAssets);

   int numMapsFixedUp = 0;
   for(const FAssetData& worldAsset : worldAssets)
   {
      const bool kLoadAsTemplate = false;
      if (!FEditorFileUtils::LoadMap(worldAsset.PackageName.ToString(), kLoadAsTemplate))
      {
         UE_LOG(LogOSECommandlet, Error, TEXT("Failed to load %s!"), *worldAsset.PackageName.ToString());
         continue;
      }

      check(GWorld);

      AWorldSettings* actualWorldSettings = GWorld->GetWorldSettings();

      TArray<UPackage*> packagesToSave;
      for (TActorIterator<AActor> it(GWorld, AWorldSettings::StaticClass()); it; ++it)
      {
         AActor* actor = (*it);

         if (actor != actualWorldSettings)
         {
            UE_LOG(LogOSECommandlet, Log, TEXT("Destroying world settings actor %s"), *actor->GetName());
            actor->Destroy();
            packagesToSave.Add(worldAsset.GetPackage());
         }
      }

      if (packagesToSave.Num() > 0)
      {
         UE_LOG(LogOSECommandlet, Log, TEXT("Checking out %s..."), *worldAsset.PackageName.ToString());
         if (FEditorFileUtils::CheckoutPackages(packagesToSave, nullptr, false))
         {
            UE_LOG(LogOSECommandlet, Log, TEXT("Saving %s..."), *worldAsset.PackageName.ToString());
            if (FEditorFileUtils::PromptForCheckoutAndSave(packagesToSave, false, false, nullptr, true, false) != FEditorFileUtils::PR_Success)
            {
               UE_LOG(LogOSECommandlet, Error, TEXT("Failed to save %s!"), *worldAsset.PackageName.ToString());
            }
            else
            {
               ++numMapsFixedUp;
            }
         }
         else
         {
            UE_LOG(LogOSECommandlet, Error, TEXT("Failed to check out %s!"), *worldAsset.PackageName.ToString());
         }
      }
   }

   UE_LOG(LogOSECommandlet, Log, TEXT("Fixed up %d maps"), numMapsFixedUp);
}
