// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATContractProgression.h"

// tat
#include "Quests/TATQuestInfo.h"
#include "Quests/TATContractState.h"
#include "Quests/Rewards/TATQuestRewardContext.h"
#include "SaveGame/TATSaveGame.h"

void TATContractProgression::CompleteContract(UTATSaveGame* saveGame, FTATCharacterSaveId saveId, const FTATContractInfo& quest)
{
   check(IsValid(saveGame));

   saveGame->SetContractState(quest.QuestTag, ETATContractState::Complete);

   if(quest.NextContractInChain.IsValid())
   {
      saveGame->SetContractState(quest.NextContractInChain, ETATContractState::Intro);
   }
}
