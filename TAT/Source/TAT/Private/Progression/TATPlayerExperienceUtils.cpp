// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Progression/TATPlayerExperienceUtils.h"

// tat
#include "Progression/TATProgressionSettings.h"
#include "Player/TATPlayerStatsTags.h"
#include "Progression/TATPlayerExperienceTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerExperienceUtils)

#define LOCTEXT_NAMESPACE "TATXP"

int32 UTATPlayerExperienceUtils::CalculateFinishedMatchXP(const UObject* worldContext, const FMatchPersistentData& matchData, const TArray<FTATFinishedMatchXPGained>& additionalXP, TArray<FTATFinishedMatchXPGained>& xpGainedArray)
{
   float totalAmountXP = 0.f;

   // We no longer want to show players the time in match statistic at the end game screen, so lump the xp together with the extraction category
   const UTATProgressionSettings& progressionSettings = UTATProgressionSettings::Get();
   const float MatchTimeInSeconds = matchData.MatchTimeInSeconds;
   const float MaximumTimeInGameXP = progressionSettings.MaximumTimeInGameXP;
   const float timeInMatchXP = FMath::Clamp(MatchTimeInSeconds / 60.f * progressionSettings.XPPerMinute, 0, MaximumTimeInGameXP);
   totalAmountXP += timeInMatchXP;

   const bool didWin = matchData.CompletionState == EMatchCompletionState::Escaped;
   if (didWin)
   {
      const float winXP = progressionSettings.WinXP;
      totalAmountXP += winXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(winXP + timeInMatchXP, TAG_XP_ReachExitInTime));
   }
   else
   {
      xpGainedArray.Add(FTATFinishedMatchXPGained(timeInMatchXP, TAG_XP_DidNotReachExitInTime));
   }

   const bool didCompleteMission = matchData.MissionResult.IsObjectiveComplete;
   if (didCompleteMission)
   {
      const float completeMissionXP = progressionSettings.MissionXP;
      totalAmountXP += completeMissionXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(completeMissionXP, TAG_XP_CompleteMission));
   }

   const bool didCompleteContracts = matchData.ContractResult.IsObjectiveComplete;
   if (didCompleteContracts)
   {
      const float completeContractsXP = progressionSettings.ContractsXP;
      totalAmountXP += completeContractsXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(completeContractsXP, TAG_XP_CompleteContracts));
   }

   const bool didOpenQueue = matchData.WasMatchmade;
   if (didOpenQueue)
   {
      const float openQueueXP = progressionSettings.OpenQueueXP;
      totalAmountXP += openQueueXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(openQueueXP, TAG_XP_OpenQueue));
   }

   // TODO: use LootingPlayerXPMultiplier when we know who looted what
   const int32 numMajorLoot = matchData.CountAllLootOfType(worldContext, ETATLootType::MajorLoot);
   if (numMajorLoot > 0)
   {
      const float majorLootXP = progressionSettings.MajorLootXP * numMajorLoot;
      totalAmountXP += majorLootXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(majorLootXP, TAG_XP_MajorLoot));
   }

   // TODO: use LootingPlayerXPMultiplier when we know who looted what
   const int32 numMinorLoot = matchData.CountAllLootOfType(worldContext, ETATLootType::MinorLoot);
   if (numMinorLoot > 0)
   {
      const float minorLootXP = progressionSettings.MinorLootXP * numMinorLoot;
      totalAmountXP += minorLootXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(minorLootXP, TAG_XP_MinorLoot));
   }
      
   for (const FTATFinishedMatchXPGained& newXPGained : additionalXP)
   {
      totalAmountXP += newXPGained.AmountXP;
      xpGainedArray.Add(newXPGained);
   }
      
   const bool wasCoop = matchData.HasCoopAllies;
   if (wasCoop)
   {
      const float previousXP = totalAmountXP;
      totalAmountXP *= progressionSettings.CoopXPMultiplier;
      const float coopXP = totalAmountXP - previousXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(coopXP, TAG_XP_Cooperative));
   }

   if (matchData.IsFTUEMatch)
   {
      const int xpNeededForNextLevel = UTATProgressionSettings::GetXPForNextLevel(1);
      // integer here, because I need to divide it
      const int additionalFtueXp = progressionSettings.FtueLevelUpXpIsOnTopOfRegularXp ?
         xpNeededForNextLevel :
         xpNeededForNextLevel - FMath::RoundToInt(totalAmountXP);
      totalAmountXP += additionalFtueXp;
      check(totalAmountXP >= xpNeededForNextLevel);

      auto divideBonusXp = [&](TConstArrayView<FGameplayTag> categories) {
         check(categories.Num() > 0);
         const int categoryCount = categories.Num();
         const int xpPerCategory = additionalFtueXp / categoryCount;
         const int remainder = additionalFtueXp % categoryCount;
         for (int i = 0; i < categoryCount; ++i)
         {
            const int categoryXp = xpPerCategory + (i < remainder ? 1 : 0);
            if (FTATFinishedMatchXPGained* found = xpGainedArray.FindByKey(categories[i]))
            {
               found->AmountXP += categoryXp;
            }
            else
            {
               xpGainedArray.Add(FTATFinishedMatchXPGained(categoryXp, categories[i]));
            }
         }
      };

      if (progressionSettings.FtueLevelUpXPCategories.Num())
      {
         divideBonusXp(progressionSettings.FtueLevelUpXPCategories);
      }
      else
      {
         // Have some fallback if no categories in data
         divideBonusXp({ TAG_XP_CompleteMission });
      }
   }

   float difficultyMultiplier = 1.0f;
   switch (matchData.MatchDifficulty)
   {
   case ETATDifficulty::Easy:
      difficultyMultiplier = progressionSettings.EasyDifficultyXPMultiplier;
      break;

   case ETATDifficulty::Normal:
      difficultyMultiplier = progressionSettings.NormalDifficultyXPMultiplier;
      break;

   case ETATDifficulty::Hard:
      difficultyMultiplier = progressionSettings.HardDifficultyXPMultiplier;
      break;

   default:
      ensureMsgf(false, TEXT("There is a matchData.MatchDifficulty that doesn't have a XP Multiplier associated."));
      break;
   }

   if (difficultyMultiplier > 1.0f)
   {
      const float previousXP = totalAmountXP;
      totalAmountXP *= difficultyMultiplier;
      const float difficultyXP = totalAmountXP - previousXP;
      xpGainedArray.Add(FTATFinishedMatchXPGained(difficultyXP, TAG_XP_Difficulty));
   }

   return FMath::RoundToInt(totalAmountXP);
}

#undef LOCTEXT_NAMESPACE
