// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Validation/EditorValidator_AssetEngineVersionSet.h"

// ue5
#include "DataValidationChangelist.h"
#include "ISourceControlChangelistState.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_AssetEngineVersionSet)

#define LOCTEXT_NAMESPACE "AssetValidation"

UEditorValidator_AssetEngineVersionSet::UEditorValidator_AssetEngineVersionSet()
{
   bIsEnabled = true;
}

bool UEditorValidator_AssetEngineVersionSet::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   return asset->IsA<UDataValidationChangelist>();
}

EDataValidationResult UEditorValidator_AssetEngineVersionSet::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UDataValidationChangelist* dataValidation = Cast<UDataValidationChangelist>(asset);
   const TSharedRef<ISourceControlChangelist, ESPMode::ThreadSafe> changeList = dataValidation->Changelist->AsShared();
   ISourceControlProvider& sourceControlProvider = ISourceControlModule::Get().GetProvider();

   const FSourceControlChangelistStatePtr changelistState = sourceControlProvider.GetState(changeList, EStateCacheUsage::Use);

   TArray<FName> filesInChangelistWithInvalidVersionData;
   
   for (const FSourceControlStateRef& file : changelistState->GetFilesStates())
   {
      // We shouldn't consider dependencies of deleted files
      if (file->IsDeleted())
      {
         continue;
      }

      FString packageName;
      if (FPackageName::TryConvertFilenameToLongPackageName(file->GetFilename(), packageName))
      {
         //Instead of loading the entire asset up, just grab the archive first.
         TUniquePtr<FArchive> externalArchive = IPackageResourceManager::Get().OpenReadExternalResource(
         EPackageExternalResource::WorkspaceDomainFile, packageName);
         if (externalArchive.IsValid())
         {
            //Then deserialize the PackageFileSummary only.
            FPackageFileSummary packageFileSummary;
            *externalArchive << packageFileSummary;
            //If the package doesn't have a changelist set BUT the engine version has a changelist set, its invalid.
            //LinkerLoad.cpp:1323 - bLoaderNeedsEngineVersionChecks
            if(packageFileSummary.SavedByEngineVersion.HasChangelist() == false)
            {
               AssetFails(asset, FText::Format(
                             LOCTEXT("VersionValidator_ChangelistNotSet",
                                     "Engine Version not set for asset {0}, sync via UGS, dirty then resave asset."),
                             FText::FromString(*packageName)));
               filesInChangelistWithInvalidVersionData.Add(*packageName);
            }
            else
            {
               AssetPasses(asset);
            }
         }
      }
   }
   //If we've tested any assets, return the result, else force Valid (Code only commits won't pass through the above validation)
   return IsValidationStateSet() ? GetValidationResult() : EDataValidationResult::Valid;
}
