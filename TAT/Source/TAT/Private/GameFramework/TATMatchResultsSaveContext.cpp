// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/TATMatchResultsSaveContext.h"

// tat
#include "GameFramework/TATMatchPersistentTypes.h"
#include "SaveGame/TATSaveGame.h"
#include "Quests/TATContractProgression.h"
#include "Quests/Rewards/TATQuestRewardContext.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "Quests/TATContractState.h"
#include "GameFramework/TATMatchPersistenceGameInstanceSubsystem.h"
#include "Progression/TATProgressionSettings.h"
#include "Progression/TATPlayerExperienceTags.h"
#include "Player/TATPlayerController.h"

void TATMatchResultsSaveContext::Apply(const FMatchPersistentData& results, UTATSaveGame* saveGame)
{
   check(saveGame);

   const FTATCharacterSaveId saveId = results.CharacterSaveId;
   check(saveId.IsValid());

   // Just give it all as money
   saveGame->UpdateMoney(results.GetTotalValue());

   {
      FTATSavedLootAddRequest addLootRequest;
      addLootRequest.PopulateFrom(results.CarriedLoot);
      addLootRequest.PopulateFrom(results.AllyCarriedLoot);
      addLootRequest.PopulateFrom(results.StashedLoot);
      saveGame->AddLoot(saveId, addLootRequest);
   }

   // Should I just make these out-of-line static helpers?
   auto applyContractResult = [saveGame, saveId](const FMatchPersistentQuestResult& questResult)
   {
      if (questResult.IsObjectiveComplete && questResult.QuestTag.IsValid())
      {
         if (const FTATContractInfo* contract = UTATQuestDataSubsystem::Get(saveGame).FindContractInfo(questResult.QuestTag))
         {
            // Grant rewards even if players are accomplices
            contract->GrantRewards(FTATQuestRewardContext{ saveGame, saveId });

            // Progress the contract if non-accomplice, or they are actively on the contract in the save game
            // The latter fallback could happen for a late join, since that is assigned at match start
            if (!questResult.IsAccomplice || saveGame->GetContractState(questResult.QuestTag) == ETATContractState::Objective)
            {
               if (contract->OutroFlow == ETATContractOutroFlow::Automatic)
               {
                  TATContractProgression::CompleteContract(saveGame, saveId, *contract);
               }
               else
               {
                  saveGame->SetContractState(questResult.QuestTag, ETATContractState::Outro);
               }
            }
         }
      }
   };
   auto applyMissionResult = [saveGame, saveId](const FMatchPersistentQuestResult& questResult)
   {
      // Just grant the rewards, completed-ness not stored for missions
      if (questResult.IsCompleteForMetagame() && questResult.QuestTag.IsValid())
      {
         if (const FTATMissionInfo* quest = UTATQuestDataSubsystem::Get(saveGame).FindMissionInfo(questResult.QuestTag))
         {
            const FTATQuestRewardContext context {
               .SaveGame = saveGame,
               .Character = saveId
            };
            quest->GrantRewards(context);
            quest->GrantBonusRewards(context, questResult.UnlockedBonusRewardsBitmask);
         }
      }
   };
   applyContractResult(results.ContractResult);
   applyMissionResult(results.MissionResult);
   

   // the match count itself isn't high priority, but the rest is
   saveGame->UpdateCharacterProgression(saveId, EOSESavePriority::HighPriority, [&results](FTATCharacterProgression& progression) {
      progression.MatchCount += 1;
      progression.UpdatePerformanceRankingForMatch(results);
   });
}

FMatchPersistentXPGainedData TATMatchResultsSaveContext::ApplyXP(const FMatchPersistentData& results, UTATSaveGame* saveGame)
{
   check(saveGame);

   FMatchPersistentXPGainedData xpGainedData;
   xpGainedData.LevelBeforeXPGain = saveGame->GetXP().Level;
   xpGainedData.LevelXPBeforeXPGain = saveGame->GetXP().CurrentLevelXP;
   xpGainedData.XPGainedArray = results.XPGainedArray;

   saveGame->AddXP(results.XPGained);

   return xpGainedData;
}
