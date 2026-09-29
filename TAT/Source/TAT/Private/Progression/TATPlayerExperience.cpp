// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Progression/TATPlayerExperience.h"

// TAT
#include "Progression/TATProgressionSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerExperience)

FTATFinishedMatchXPGained::FTATFinishedMatchXPGained(int32 amountXP, FGameplayTag categoryTag)
   : AmountXP(amountXP)
   , CategoryTag(categoryTag)
{
}

FTATFinishedMatchXPGainedInfo::FTATFinishedMatchXPGainedInfo(int32 amountXP, FText categoryText, FGameplayTag categoryTag)
   : AmountXP(amountXP)
   , CategoryText(categoryText)
   , CategoryTag(categoryTag)
{
}

FTATPlayerExperience::FTATPlayerExperience(const int32 newXP)
   : XP(newXP)
{
   Update();
}

void FTATPlayerExperience::SetXP(const int32 totalXP)
{
   const UTATProgressionSettings& settings = UTATProgressionSettings::Get();
   const int32 maxXP = UTATProgressionSettings::GetTotalXPRequiredForLevel(settings.MaximumLevel);
   XP = FMath::Clamp(totalXP, 0, maxXP);
   Update();
}

void FTATPlayerExperience::AddXP(const int32 amountXP)
{
   SetXP(XP + amountXP);
}

void FTATPlayerExperience::FixLevelVersions()
{
   const int currentLevel = Level;
   const int minXP = UTATProgressionSettings::GetTotalXPRequiredForLevel(currentLevel);
   XP = FMath::Max(XP, minXP);
}

void FTATPlayerExperience::Update()
{
   int currentLevel = 0;
   int minXP = 0;
   const UTATProgressionSettings& progressionSettings = UTATProgressionSettings::Get();
   while (currentLevel <= progressionSettings.MaximumLevel)
   {
      minXP += UTATProgressionSettings::GetXPForNextLevel(currentLevel);
      if (minXP <= XP)
      {
         ++currentLevel;
      }
      else
      {
         break;
      }
   }
   Level = currentLevel;
   const int previousLevelXP = UTATProgressionSettings::GetTotalXPRequiredForLevel(currentLevel);
   CurrentLevelXP = XP - previousLevelXP;
}
