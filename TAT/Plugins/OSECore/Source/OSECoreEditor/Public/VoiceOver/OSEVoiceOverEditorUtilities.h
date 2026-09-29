// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

class UOSEVoiceOverLine;

class FOSEVoiceOverEditorUtilities
{
public:
   FOSEVoiceOverEditorUtilities() = delete;

   // Populates a voice line using it's voice verb tag and file naminig conventions and returns true if anything was changed.
   static bool PopulateVoiceLine(UOSEVoiceOverLine* line);

   // Uses the VoiceVerb tags to ensure they all have a voice line asset created, returns list of changed assets.
   static TArray<UOSEVoiceOverLine*> GenerateVoiceLines();
};
