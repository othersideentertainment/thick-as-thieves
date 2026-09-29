// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEDataValidationCommandlet.h"

// ue
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorValidatorSubsystem.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDataValidationCommandlet)

// TODO: Move this to an OSE-level config file?
static constexpr FStringView kHardcodedIgnoreFolderRoots[] =
   {
      TEXTVIEW("/Game/__ExternalActors__/"),
      TEXTVIEW("/Game/__ExternalObjects__/"),
      TEXTVIEW("/Game/Developers/"),
      TEXTVIEW("/Game/ThirdParty/")
   };

UOSEDataValidationCommandlet::UOSEDataValidationCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

int UOSEDataValidationCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   TArray<FString> tokens;
   TArray<FString> switches;
   TMap<FString, FString> params;
   ParseCommandLine(*fullCommandLine, tokens, switches, params);

   TArray<FString> ignoreFolderRoots;
   if (const FString* extraRootsPtr = params.Find(TEXT("ignoreFolderRoots")))
   {
      const FString& extraRoots = *extraRootsPtr;
      FString buf;
      buf.Reserve(127);
      for (int32 i = 0; i < extraRoots.Len(); i++)
      {
         const TCHAR ch = extraRoots[i];
         // Semicolon is a standard path separator on Windows but it can be annoying to use here - if unescaped/unquoted, powershell and bash will both
         // parse it as the end of a command. Colon is the best option as it's simplest for command line parsing.
         // As per the principle of least astonishment here, we'll just allow comma, colon, or semicolon, as none of them should appear in a valid path anyway.
         if (ch == ',' || ch == ':' || ch == ';')
         {
            if (buf.Len() > 0)
            {
               ignoreFolderRoots.Add(buf);
               buf.Reset();
            }
         }
         else
         {
            buf.AppendChar(ch);
         }
      }
      if (buf.Len() > 0)
      {
         ignoreFolderRoots.Add(buf);
      }
   }

   for (const FString& path : ignoreFolderRoots)
   {
      UE_LOG(LogOSECommandlet, Display, TEXT("Ignoring paths starting with '%s' for validation (from ignoreFolderRoots command line parameter)"), *path);
   }

   for (const FStringView& path : kHardcodedIgnoreFolderRoots)
   {
      UE_LOG(LogOSECommandlet, Display, TEXT("Ignoring paths starting with '%s' for validation (from kHardcodedIgnoreFolderRoots)"), path.GetData());
   }

   // validate data
   if (!ValidateData(ignoreFolderRoots))
   {
      UE_LOG(LogOSECommandlet, Warning, TEXT("Errors occurred while validating data"));
      return 2; // return something other than 1 for error since the engine will return 1 if any other system (possibly unrelated) logged errors during execution.
   }

   return 0;
}

/* static */
bool UOSEDataValidationCommandlet::ValidateData(TArrayView<FString> ignoreFolderRoots)
{
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

   FARFilter assetFilter;
   assetFilter.PackagePaths.Add(TEXT("/Game"));
   assetFilter.bRecursivePaths = true;

   TArray<FAssetData> assetDataList;
   assetRegistryModule.Get().EnumerateAllAssets([&assetDataList, ignoreFolderRoots](const FAssetData& assetData) {
      const FString& packagePath = assetData.PackagePath.ToString();
      for (const FString& folderRoot : ignoreFolderRoots)
      {
         if (packagePath.StartsWith(folderRoot))
         {
            UE_LOG(LogOSECommandlet, Display, TEXT("Ignoring path %s (ignoreFolderRoots flag includes path '%s')"), *packagePath, *folderRoot);
            return true;
         }
      }

      for (const FStringView& folderRoot : kHardcodedIgnoreFolderRoots)
      {
         if (packagePath.StartsWith(folderRoot))
         {
            UE_LOG(LogOSECommandlet, Log, TEXT("Ignoring package %s (kHardcodedIgnoreFolderRoots includes path '%s')"), *packagePath, folderRoot.GetData());
            return true;
         }
      }

      // Skip maps, since those should be covered by map-check
      // TODO: add configuration once there is a use-case
      if (assetData.AssetClassPath == UWorld::StaticClass()->GetClassPathName())
      {
         return true;
      }

      assetDataList.Add(assetData);
      return true;
   });

   UEditorValidatorSubsystem* editorValidationSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
   check(editorValidationSubsystem);

   FValidateAssetsSettings settings;
   settings.bSkipExcludedDirectories = true;
   settings.bShowIfNoFailures = true;
   settings.ValidationUsecase = EDataValidationUsecase::Commandlet;

   FValidateAssetsResults results;
   editorValidationSubsystem->ValidateAssetsWithSettings(assetDataList, settings, results);

   return true;
}
