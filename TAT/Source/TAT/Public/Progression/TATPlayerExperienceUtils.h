// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Progression/TATPlayerExperience.h"
#include "SaveGame/TATSaveGame.h"

#include "TATPlayerExperienceUtils.generated.h"

UCLASS()
class UTATPlayerExperienceUtils : public UObject
{
   GENERATED_BODY()

public:
   static int32 CalculateFinishedMatchXP(const UObject* worldContext, const FMatchPersistentData& matchData, const TArray<FTATFinishedMatchXPGained>& additionalXP, TArray<FTATFinishedMatchXPGained>& xpGainedArray);
};
