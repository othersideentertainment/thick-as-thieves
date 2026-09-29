// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSECommandletBase.h"
#include "VoiceOver/OSEVoiceOverLine.h"

// ue
#include "GameplayTagContainer.h"

#include "OSEVoiceOverLineAssignAkEventCommandlet.generated.h"

class UAkAudioEvent;
class UPackage;

// Used for parsing out the Identity / Stim data from the filenames of VOLs / AkAudioEvents
struct FOSEVoiceOverIdentityStimData
{
   FOSEVoiceOverIdentityStimData(const UOSEVoiceOverLine* voiceOverLine);
   FOSEVoiceOverIdentityStimData(const UAkAudioEvent* akAudioEvent);
public:
   // We match specifically on stim, because VOLs are structured as 1 stim -> N identities (and the AkEvents define those identities)
   FORCEINLINE bool operator==(const FOSEVoiceOverIdentityStimData& other) const { return other.Stim == Stim; }

   FString Identity;
   FString Stim;

private:
   // Takes a prefix to ignore, and parses out the Identity / Stim from an asset name
   // NOTE: Our AkAudioEvents have the stim surrounded by index labels, so those must be stripped out by the caller
   void _ExtractIdentityAndStim(const FString& assetName, const FString& prefix, FString& outIdentity, FString& outStim);
};

// Commandlet for running batch data operations on our VOLs (and corresponding AkAudioEvents)
UCLASS(Config=Game)
class OSECOREEDITOR_API UOSEVoiceOverLineAssignAkEventCommandlet : public UOSECommandletBase
{
   GENERATED_BODY()

   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEVoiceOverLineAssignAkEventCommandlet"); };

private:

   // Assigns AkEvents to the corresponding FOSEVoiceOverLineIdentityData of the associated stim's VOL (as defined in the scraped data for each collection).
   // Returns a list of modified UOSEVoiceOverLine packages
   void _RefreshVoiceOverLineAkEvents(TMap<UOSEVoiceOverLine*, FOSEVoiceOverIdentityStimData> voiceOverLines, TMap<UAkAudioEvent*, FOSEVoiceOverIdentityStimData>& akEvents, TArray<UPackage*>& modifiedPackages, bool dryRun);

   int _SavePackages(const TArray<UPackage*>& packagesToSave, bool noSourceControl);
   FARFilter _GenerateAssetRegistryFilter(const TArray<FString>& assetSearchPaths, const FTopLevelAssetPath& classPathName) const;
   TMap<UAkAudioEvent*, FOSEVoiceOverIdentityStimData> _ScrapeAkEventData(const TArray<FString>& assetSearchPaths) const;
   TMap<UOSEVoiceOverLine*, FOSEVoiceOverIdentityStimData> _ScrapeVoiceOverLineData(const TArray<FString>& assetSearchPaths) const;

   bool _VoiceOverDataContainsAkEvent(const FOSEVoiceOverLineIdentityData& identityData, const UAkAudioEvent* akAudioEvent) const;

   // Paths under which to search for AkAudioEvents to hook up to corresponding VOLs
   UPROPERTY(Config)
   TArray<FString> _akAudioEventFilterPackagePaths;

   // Paths under which to search for VOLs to update
   UPROPERTY(Config)
   TArray<FString> _volFilterPackagePaths;

   // Used for selecting identity tag when updating a VOL's FOSEVoiceOverLineIdentityData
   UPROPERTY(Config)
   TMap<FString, FGameplayTag> _identityStringTagMap;
};
