// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEAssetRegistryUtl.h"

// ue
#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"

namespace OSEAssetRegistryUtl
{
   TArray<FAssetData> FindAssets(TConstArrayView<FString> assetSearchPaths, const FTopLevelAssetPath& classPathName)
   {
      const FARFilter arFilter = OSEAssetRegistryUtl::GenerateAssetRegistryFilter(assetSearchPaths, classPathName);
      const FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
      TArray<FAssetData> assetDataList;
      assetRegistryModule.Get().GetAssets(arFilter, assetDataList);

      return assetDataList;
   }

   bool SavePackages(const TArray<UPackage*>& packagesToSave, bool noSourceControl)
   {
      if (noSourceControl)
      {
         for (UPackage* package : packagesToSave)
         {
            // Set files writable
            FString packageFilename = SourceControlHelpers::PackageFilename(package);
            if (IPlatformFile::GetPlatformPhysical().FileExists(*packageFilename))
            {
               if (!IPlatformFile::GetPlatformPhysical().SetReadOnly(*packageFilename, false))
               {
                  UE_LOG(LogOSEAssetRegistryUtl, Error, TEXT("Error setting %s writable"), *packageFilename);
               }
            }
            else
            {
               UE_LOG(LogOSEAssetRegistryUtl, Error, TEXT("Could not find package: %s"), *packageFilename);
            }
         }
      }
      else
      {
         // Checkout files
         FScopedSourceControl sourceControlScope;
         if (ISourceControlModule::Get().IsEnabled())
         {
            const bool errorIfAlreadyCheckedOut = false;
            TArray<UPackage*>* outPackagesCheckedOut = nullptr;
            FEditorFileUtils::CheckoutPackages(packagesToSave, outPackagesCheckedOut, errorIfAlreadyCheckedOut);
         }
         else
         {
            // Skip package save if no source control module and -nosourcecontrol not specified
            UE_LOG(LogOSEAssetRegistryUtl, Error, TEXT("Failed to checkout packages - no source control module configured! Setup source control in the editor, \
   or pass -nosourcecontrol to modify packages without source control"));
            return false;
         }
      }

      // Save checked-out files
      const bool checkDirty = false;
      const bool promptToSave = false;
      TArray<UPackage*> outFailedPackages;
      const bool alreadyCheckedOut = true;
      const bool canBeDeclined = false;
      FEditorFileUtils::PromptForCheckoutAndSave(packagesToSave, checkDirty, promptToSave, &outFailedPackages, alreadyCheckedOut, canBeDeclined);
      for (UPackage* failedPackage : outFailedPackages)
      {
         UE_LOG(LogOSEAssetRegistryUtl, Error, TEXT("Failed to checkout and save package %s!"), *failedPackage->GetName());
      }
      return true;
   }

   FARFilter GenerateAssetRegistryFilter(TConstArrayView<FString>& assetSearchPaths, const FTopLevelAssetPath& classPathName)
   {
      FARFilter arFilter;

      // Filter for packages of specified class in specified directories
      arFilter.ClassPaths.Add(classPathName);
      arFilter.bRecursivePaths = true;
      for (const FString& assetSearchPath : assetSearchPaths)
      {
         const FName pathName = FName(*assetSearchPath, EFindName::FNAME_Find);
         if (pathName.IsValid())
         {
            arFilter.PackagePaths.Add(FName(*assetSearchPath));
         }
         else
         {
            UE_LOG(LogOSEAssetRegistryUtl, Error, TEXT("Failed to lookup FName for package path: %s"), *assetSearchPath);
         }
      }
      return arFilter;
   }
}
