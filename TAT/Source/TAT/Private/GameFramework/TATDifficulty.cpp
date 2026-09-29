// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATDifficulty.h"

// tat
#include "Developer/TATEditorSettings.h"
#include "GameFramework/TATWorldSettings.h"
#include "Settings/TATMatchSettings.h"

// ose
#include "OSECoreCheats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDifficulty)

UE_DEFINE_GAMEPLAY_TAG(TAG_MatchSetting_Difficulty, "MatchSetting.Difficulty")
UE_DEFINE_GAMEPLAY_TAG(TAG_Difficulty_Easy, "Difficulty.Novice")
UE_DEFINE_GAMEPLAY_TAG(TAG_Difficulty_Normal, "Difficulty.Thief")
UE_DEFINE_GAMEPLAY_TAG(TAG_Difficulty_Hard, "Difficulty.MasterThief")

FGameplayTag TATDifficulty::GetDifficultyTag(ETATDifficulty difficulty)
{
   switch (difficulty)
   {
      case ETATDifficulty::Easy:
         return TAG_Difficulty_Easy;
      case ETATDifficulty::Normal:
         return TAG_Difficulty_Normal;
      case ETATDifficulty::Hard:
         return TAG_Difficulty_Hard;
   }

   return FGameplayTag();
}

ETATDifficulty TATDifficulty::GetDifficultyForMatch(UWorld* world)
{
#if OSE_CHEATS_ENABLED
   if (UTATEditorSettings::Get().ShouldOverrideDifficulty)
   {
      return UTATEditorSettings::Get().DifficultyOverride;
   }
#endif
   
   if (const UTATMatchSettings* matchSettings = UTATMatchSettings::GetTATMatchSettings<UTATMatchSettings>(world))
   {
      return matchSettings->Difficulty;
   }

   return ETATDifficulty::Easy;
}

void TATDifficulty::InitDifficulty(UWorld* world)
{
   const ATATWorldSettings* worldSettings = ATATWorldSettings::GetTATWorldSettings(world);
   // Force tutorial to Easy/Novice in tutorial levels
   // * Doing it as a mutation so that it doesn't have be checked continuously in GetDifficultyForMatch
   // * Keeping it narrow/specific rather than configurable because the FTUE has a number of simplifying
   //   constraints that make is easier to be confident about the correctness which may not hold in a more
   //   general case.
   // If we need a just-in-time general-purpose difficulty override, we could deflect reads from match settings
   // to some other location (or construct it in such a way that it guarantees ordering)
   if (worldSettings && worldSettings->MapType == ETATMapType::Tutorial)
   {
      if (UTATMatchSettings* matchSettings = UTATMatchSettings::GetMutableMatchSettings<UTATMatchSettings>(world))
      {
         matchSettings->Difficulty = ETATDifficulty::Easy;
      }
   }
}
