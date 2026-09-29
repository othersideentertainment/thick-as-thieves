// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEVoiceOverLineAssignAkEventCommandlet.h"

// ue
#include "AkAudioEvent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlOperation.h"
#include "Misc/Paths.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverLineAssignAkEventCommandlet)
DEFINE_LOG_CATEGORY_STATIC(LogOSEVoiceOverLineAssignAkEventCommandlet, Log, All);

FOSEVoiceOverIdentityStimData::FOSEVoiceOverIdentityStimData(const UOSEVoiceOverLine* voiceOverLine)
{
   // VOLs should follow a format:
   // VOL_XXX_YYYY_YYY
   // Where XXX = a generic identity (eg. "NPC" which corresponds to Enforcer / GenericCiv2, but not Player)
   // and YYYY_YYY = a stim (we allow underscores within for clarity)
   const FString assetName = voiceOverLine->GetName();
   const FString prefix = TEXT("VOL_");
   _ExtractIdentityAndStim(assetName, prefix, Identity, Stim);
   
   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("%s:"), *assetName);
   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("* Identity = %s"), *Identity);
   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("* Stim = %s"), *Stim);
}

FOSEVoiceOverIdentityStimData::FOSEVoiceOverIdentityStimData(const UAkAudioEvent* akAudioEvent)
{
   // VO AkAudioEvents should follow a format:
   // Play_VO_XXX_AA_YYYY_YYY_BB
   // Where XXX = a specific identity (eg. "Enforcer" / "GenericCiv2")
   // and YYYY_YYY = a stim (we allow underscores within for clarity)
   // AA = line variant and BB = take number. These are not needed for parsing, so we attempt to strip them out afterwards
   FString assetName = akAudioEvent->GetName();
   const FString prefix = TEXT("Play_VO_");
   FString outStim;
   _ExtractIdentityAndStim(assetName, prefix, Identity, outStim);

   // Attempt to parse out the indices surrounding the stim
   // Eg. Play_VO_Enforcer1_47_ThiefObserved_ChasingThief_01
   // We parsed out 47_ThiefObserved_ChasingThief_01 above, and now need to trim out the first/last elements
   TArray<FString> parsedStimArray;
   check(outStim.ParseIntoArray(parsedStimArray, TEXT("_")) >= 3);
   parsedStimArray.RemoveAt(0);
   parsedStimArray.Pop();
   
   // Rejoin the middle parts with underscores to form the final stim
   Stim = FString::Join(parsedStimArray, TEXT("_"));

   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("%s:"), *assetName);
   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("* Identity = %s"), *Identity);
   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("* Stim = %s"), *Stim);
}

void FOSEVoiceOverIdentityStimData::_ExtractIdentityAndStim(const FString& assetName, const FString& prefix, FString& outIdentity, FString& outStim)
{
   check(assetName.StartsWith(prefix));
   FString beforeVOLPrefix;
   FString afterVOLPrefix;
   assetName.Split(prefix, &beforeVOLPrefix, &afterVOLPrefix);

   afterVOLPrefix.Split(TEXT("_"), &outIdentity, &outStim);
}

int UOSEVoiceOverLineAssignAkEventCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   TArray<FString> tokens, switches;
   ParseCommandLine(*fullCommandLine, tokens, switches);
   const bool dryRun = switches.Contains(TEXT("dryrun"));
   const bool noSourceControl = switches.Contains(TEXT("nosourcecontrol"));

   // Find all AkEvent / VOLs in specified paths, scraping stim / identity data from their filenames
   TMap<UOSEVoiceOverLine*, FOSEVoiceOverIdentityStimData> voiceOverLines = _ScrapeVoiceOverLineData(_volFilterPackagePaths);
   TMap<UAkAudioEvent*, FOSEVoiceOverIdentityStimData> akEvents = _ScrapeAkEventData(_akAudioEventFilterPackagePaths);
   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Log, TEXT("Found %d ak events and %d VOLs"), akEvents.Num(), voiceOverLines.Num());

   TArray<UPackage*> packagesToSave;
   _RefreshVoiceOverLineAkEvents(voiceOverLines, akEvents, packagesToSave, dryRun);

   for (UPackage* package : packagesToSave)
   {
      UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("Changes %s to package: %s"), dryRun ? TEXT("would be made") : TEXT("made"), *package->GetName());
   }

   const bool needsSave = !dryRun && packagesToSave.Num() > 0;
   return needsSave ? _SavePackages(packagesToSave, noSourceControl) : 0;
}

void UOSEVoiceOverLineAssignAkEventCommandlet::_RefreshVoiceOverLineAkEvents(TMap<UOSEVoiceOverLine*, FOSEVoiceOverIdentityStimData> voiceOverLines, TMap<UAkAudioEvent*, FOSEVoiceOverIdentityStimData>& akEvents, TArray<UPackage*>& modifiedPackages, bool dryRun)
{
   for (auto& voiceOverIt : voiceOverLines)
   {
      UOSEVoiceOverLine* voiceOverLine = voiceOverIt.Key;
      const FOSEVoiceOverIdentityStimData& voiceOverLineScrapedNameData = voiceOverIt.Value;
      check(voiceOverLine);

      bool voiceLineModified = false;
      // Find AkEvents with corresponding stims
      for (auto& akEventIt : akEvents)
      {
         UAkAudioEvent* akEvent = akEventIt.Key;
         const FOSEVoiceOverIdentityStimData& akEventScrapedNameData = akEventIt.Value;
         check(akEvent);

         if (akEventScrapedNameData == voiceOverLineScrapedNameData)
         {
            UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("Matched %s => %s"), *akEvent->GetName(), *voiceOverLine->GetName());
            if (const FGameplayTag* identityTag = _identityStringTagMap.Find(akEventScrapedNameData.Identity))
            {
               FOSEVoiceOverLineIdentityData* identityData = voiceOverLine->Identities.Find(*identityTag);
               if (!identityData)
               {
                  // Create new identity entry if not present
                  UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("Adding new FOSEVoiceOverLineIdentityData for identity tag: %s"), *identityTag->ToString());
                  identityData = &voiceOverLine->Identities.Emplace(*identityTag);
               }

               // Add AkEvent line to identity entry if needed
               const bool needsLineForEvent = !_VoiceOverDataContainsAkEvent(*identityData, akEvent);
               if (needsLineForEvent)
               {
                  UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Log, TEXT("Updating %s with new line for AkAudioEvent: %s"), *voiceOverLine->GetName(), *akEvent->GetName());
                  FOSEVoiceOverLineData lineData;
                  lineData.AudioEvent = akEvent;
                  identityData->Lines.Add(lineData);
                  voiceLineModified = true;
               }
               else
               {
                  UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Verbose, TEXT("%s already has line for AkAudioEvent: %s"), *voiceOverLine->GetName(), *akEvent->GetName());
               }
            }
         }
      }

      if (voiceLineModified)
      {
         modifiedPackages.AddUnique(voiceOverLine->GetPackage());
      }
   }

   UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Log, TEXT("%d VOLs %s with new AkAudioEvents"), modifiedPackages.Num(), dryRun ? TEXT("would be updated") : TEXT("updated"));
}

int UOSEVoiceOverLineAssignAkEventCommandlet::_SavePackages(const TArray<UPackage*>& packagesToSave, bool noSourceControl)
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
               UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Error, TEXT("Error setting %s writable"), *packageFilename);
            }
         }
         else
         {
            UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Error, TEXT("Could not find package: %s"), *packageFilename);
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
         UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Error, TEXT("Failed to checkout packages - no source control module configured! Setup source control in the editor, \
or pass -nosourcecontrol to modify packages without source control"));
         return 1;
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
      UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Error, TEXT("Failed to checkout and save package %s!"), *failedPackage->GetName());
   }
   return 0;
}

FARFilter UOSEVoiceOverLineAssignAkEventCommandlet::_GenerateAssetRegistryFilter(const TArray<FString>& assetSearchPaths, const FTopLevelAssetPath& classPathName) const
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
         UE_LOG(LogOSEVoiceOverLineAssignAkEventCommandlet, Error, TEXT("Failed to lookup FName for package path: %s"), *assetSearchPath);
      }
   }
   return arFilter;
}

TMap<UAkAudioEvent*, FOSEVoiceOverIdentityStimData> UOSEVoiceOverLineAssignAkEventCommandlet::_ScrapeAkEventData(const TArray<FString>& assetSearchPaths) const
{
   TMap<UAkAudioEvent*, FOSEVoiceOverIdentityStimData> akAudioEvents;

   FARFilter arFilter = _GenerateAssetRegistryFilter(assetSearchPaths, UAkAudioEvent::StaticClass()->GetClassPathName());
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
   TArray<FAssetData> assetDataList;
   assetRegistryModule.Get().GetAssets(arFilter, assetDataList);

   for (const FAssetData& assetData : assetDataList)
   {
      // Ensure we're only capturing the VO events (and none of the Stop ones)
      if (assetData.AssetName.ToString().StartsWith(TEXT("Play_VO")))
      {
         UAkAudioEvent* akAudioEvent = CastChecked<UAkAudioEvent>(assetData.GetAsset());
         FOSEVoiceOverIdentityStimData scrapedNameData(akAudioEvent);
         akAudioEvents.Emplace(akAudioEvent, scrapedNameData);
      }
   }
   return akAudioEvents;
}

TMap<UOSEVoiceOverLine*, FOSEVoiceOverIdentityStimData> UOSEVoiceOverLineAssignAkEventCommandlet::_ScrapeVoiceOverLineData(const TArray<FString>& assetPaths) const
{
   TMap<UOSEVoiceOverLine*, FOSEVoiceOverIdentityStimData> voiceOverLines;

   FARFilter arFilter = _GenerateAssetRegistryFilter(assetPaths, UOSEVoiceOverLine::StaticClass()->GetClassPathName());
   FAssetRegistryModule& assetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
   TArray<FAssetData> assetDataList;
   assetRegistryModule.Get().GetAssets(arFilter, assetDataList);

   for (const FAssetData& assetData : assetDataList)
   {
      UOSEVoiceOverLine* voiceOverLine = CastChecked<UOSEVoiceOverLine>(assetData.GetAsset());
      FOSEVoiceOverIdentityStimData scrapedNameData(voiceOverLine);
      voiceOverLines.Emplace(voiceOverLine, scrapedNameData);
   }

   return voiceOverLines;
}

bool UOSEVoiceOverLineAssignAkEventCommandlet::_VoiceOverDataContainsAkEvent(const FOSEVoiceOverLineIdentityData& identityData, const UAkAudioEvent* akAudioEvent) const
{
   for (const FOSEVoiceOverLineData& lineData : identityData.Lines)
   {
      if (lineData.AudioEvent == akAudioEvent)
      {
         return true;
      }
   }
   return false;
}
