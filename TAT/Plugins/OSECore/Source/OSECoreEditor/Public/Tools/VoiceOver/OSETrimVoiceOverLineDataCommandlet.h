// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"

// ose
#include "VoiceOver/OSEVoiceOverLine.h"

#include "OSETrimVoiceOverLineDataCommandlet.generated.h"

class UAkAudioEvent;
class UOSEVoiceOverLine;
class UPackage;

// Batch operation on VOLs to remove lines from VOLs based on a variety of criteria (empty lines, TTS lines, etc). Call with -dryrun to preview results
UCLASS(Config=Game)
class OSECOREEDITOR_API UOSETrimVoiceOverLineDataCommandlet : public UOSECommandletBase
{
   GENERATED_BODY()

   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSETrimUnassignedVoiceOverLineDataCommandlet"); };

private:
   void _TrimLines(TArray<UOSEVoiceOverLine*> voiceOverLines, TArray<UPackage*>& modifiedPackages, const TArray<FString>& switches);
   bool _ShouldTrimLine(const FOSEVoiceOverLineIdentityData& identityData, int32 lineIndex, const TArray<FString>& switches) const;

private:
   // Paths under which to search for VOLs to update
   UPROPERTY(Config)
   TArray<FString> _volFilterPackagePaths;
};
