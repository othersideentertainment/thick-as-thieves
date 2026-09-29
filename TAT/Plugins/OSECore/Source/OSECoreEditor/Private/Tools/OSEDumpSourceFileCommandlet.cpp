// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tools/OSEDumpSourceFileCommandlet.h"

// ue5
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorFramework/AssetImportData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDumpSourceFileCommandlet)

UOSEDumpSourceFileCommandlet::UOSEDumpSourceFileCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

int UOSEDumpSourceFileCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   _DumpSourceFiles();

   return 0;
}

void UOSEDumpSourceFileCommandlet::_DumpSourceFiles()
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
   IAssetRegistry& assetRegistry = assetRegistryModule.Get();

   FARFilter assetFilter;
   assetFilter.PackagePaths.Add(TEXT("/Game"));
   assetFilter.bRecursivePaths = true;

   TArray<FAssetData> assetDataList;
   assetRegistry.GetAssets(assetFilter, assetDataList);

   const FName sourceFileKey = SourceFileTagName();

   // get rid of anything that we don't need based on our static filters
   for (const FAssetData& assetData : assetDataList)
   {
      if(!assetData.FindTag(sourceFileKey))
      {
         continue;
      }

      const FString sourceFileRaw = assetData.GetTagValueRef<FString>(sourceFileKey);
      TOptional<FAssetImportInfo> importInfo = FAssetImportInfo::FromJson(sourceFileRaw);
      if (importInfo.IsSet())
      {
         for(const FAssetImportInfo::FSourceFile& sourceFile : importInfo->SourceFiles)
         {
            UE_LOG(LogOSECommandlet, Display, TEXT("%s,%s"), *assetData.PackageName.ToString(), *sourceFile.RelativeFilename);
         }
      }
   }
}
