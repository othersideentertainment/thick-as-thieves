// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEGenerateVoiceLinesCommandlet.h"

//ose editor
#include "VoiceOver/OSEVoiceOverEditorUtilities.h"

//ose
#include "OSEProjectSettings.h"
#include "VoiceOver/OSEVoiceOverLine.h"

//ue4
#include "AkUnrealHelper.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "GameplayTagsManager.h"
#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "PackageHelperFunctions.h"
#include "Platforms/AkUEPlatform.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGenerateVoiceLinesCommandlet)

DEFINE_LOG_CATEGORY_STATIC(LogOSEGenerateAllQuipsCommandlet, All, All);

UOSEGenerateVoiceLinesCommandlet::UOSEGenerateVoiceLinesCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

int UOSEGenerateVoiceLinesCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   TArray<FString> Tokens, Switches;
   ParseCommandLine(*fullCommandLine, Tokens, Switches);

   bool noSourceControl = Switches.Contains(TEXT("nosourcecontrol"));

   TArray<UOSEVoiceOverLine*> changedLines = FOSEVoiceOverEditorUtilities::GenerateVoiceLines();

   TArray<UPackage*> packagesToSave;
   packagesToSave.Reserve(changedLines.Num());
   for (UOSEVoiceOverLine* line : changedLines)
   {
      packagesToSave.Add(line->GetPackage());
   }

   FScopedSourceControl sourceControlScope;
   ISourceControlProvider* sourceControlProvider = noSourceControl ? nullptr : &ISourceControlModule::Get().GetProvider();

   if (sourceControlProvider)
   {
      FEditorFileUtils::CheckoutPackages(packagesToSave, nullptr, false);
   }
   else
   {
      for (UPackage* package : packagesToSave)
      {
         FString packageFilename = SourceControlHelpers::PackageFilename(package);
         if (IPlatformFile::GetPlatformPhysical().FileExists(*packageFilename))
         {
            if (!IPlatformFile::GetPlatformPhysical().SetReadOnly(*packageFilename, false))
            {
               UE_LOG(LogOSEGenerateAllQuipsCommandlet, Error, TEXT("Error setting %s writable"), *packageFilename);
               return 1;
            }
         }
      }
   }

   FEditorFileUtils::PromptForCheckoutAndSave(packagesToSave, false, false, nullptr, true, false);
   return 0;
}


