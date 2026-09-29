// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Quests/TATQuestFlowUtils.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "GameFramework/TATMatchPersistentTypes.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestInfo.h"
#include "UI/Quests/TATContractJournalScreen.h"
#include "UI/Quests/TATQuestNoteScreen.h"
#include "UI/Quests/TATQuestCompleteScreen.h"
#include "UI/Queue/TATUIQueue.h"

// ue5
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestFlowUtils)


void UTATQuestFlowUtils::AddPostMatchContractFlow(UTATUIQueue* queue, const FMatchPersistentQuestResult& questResult)
{
   // no contract, do nothing
   const FGameplayTag questTag = questResult.QuestTag;
   if (!questTag.IsValid() || questResult.IsAccomplice)
   {
      return;
   }

   const FTATContractInfo* contract = UTATQuestDataSubsystem::Get(queue).FindContractInfo(questTag);
   if (contract == nullptr)
   {
      return;
   }

   const bool didComplete = questResult.IsObjectiveComplete;
   if(!didComplete)
   {
      AddContractFailedFlow(queue, *contract);
   }
   else if (contract->OutroFlow == ETATContractOutroFlow::Automatic)
   {
      AddContractCompleteFlow(queue, *contract);
   }
}

void UTATQuestFlowUtils::AddContractCompleteFlow(UTATUIQueue* queue, const FTATContractInfo& quest)
{
   AddContractPreCompleteFlow(queue, quest);
   AddContractPostCompleteFlow(queue, quest);
}

void UTATQuestFlowUtils::AddContractPreCompleteFlow(UTATUIQueue* queue, const FTATContractInfo& quest)
{
   if (!quest.OutroEncounterText.IsEmpty())
   {
      if(quest.OutroFlow == ETATContractOutroFlow::Fairy)
      {
         queue->AddAction(UTATQuestNoteScreen::CreateCustomAction(UTATProjectSettings::Get().FairyContractOutroScreen, quest.OutroEncounterText));
      }
      else
      {
         queue->AddAction(UTATQuestNoteScreen::CreateQuestNoteAction(quest.OutroEncounterText));
      }
   }
}

void UTATQuestFlowUtils::AddContractPostCompleteFlow(UTATUIQueue* queue, const FTATContractInfo& quest)
{
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   queue->AddAction(UTATToastQueueAction::Create(projectSettings.ContractCompletedToastMessage, projectSettings.ContractCompletedToastTag));

   if (!quest.PostOutroCustomScreenWidget.IsNull())
   {
      queue->AddAction(UTATScreenQueueAction::Create(quest.PostOutroCustomScreenWidget));
   }
}

void UTATQuestFlowUtils::AddContractFailedFlow(UTATUIQueue* queue, const FTATContractInfo& quest)
{
   if(!quest.ObjectiveFailedEncounterText.IsEmpty())
   {
      queue->AddAction(UTATQuestNoteScreen::CreateQuestNoteAction(quest.ObjectiveFailedEncounterText));
   }
}

void UTATQuestFlowUtils::AddMissionCompleteFlow(UTATUIQueue* queue, FGameplayTag questTag, bool didComplete)
{
   if (questTag.IsValid())
   {
      
      queue->AddAction(UTATQuestCompleteScreen::CreateMissionAction(questTag, didComplete));
   }
}
