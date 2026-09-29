// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Quests/TATContractJournalScreen.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATContractState.h"
#include "SaveGame/TATSaveGame.h"
#include "UI/Queue/TATUIQueue.h"

// ue5
#include "Components/ListView.h"
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATContractJournalScreen)

UTATUIQueueAction* UTATContractJournalScreen::CreateAction(const FGameplayTag& questTag)
{
   return UTATScreenQueueAction::Create(UTATProjectSettings::Get().QuestJournalScreen,
      [questTag](UTATScreenWidget* screen)
      {
         CastChecked<UTATContractJournalScreen>(screen)->_initialContractTag = questTag;
      });
}

void UTATContractJournalScreen::NativeConstruct()
{
   Super::NativeConstruct();

   const UTATSaveGame* save = UTATSaveGame::GetTATSaveGame(this);
   if (!save)
   {
      return;
   }

   const UTATQuestDataSubsystem& questDataSubsystem = UTATQuestDataSubsystem::Get(this);

   const TArray<FTATContractWithStatus>& quests = save->GetPlayerProgression().Contracts;
   _quests.Reserve(quests.Num());

   int initialQuestIndex = INDEX_NONE;
   for (const FTATContractWithStatus& quest : quests)
   {
      if (quest.State <= ETATContractState::Intro)
      {
         continue;
      }

      FTATQuestHandle questHandle;
      if(questDataSubsystem.FindQuestHandle(quest.ContractTag, questHandle))
      {
         if (quest.ContractTag == _initialContractTag)
         {
            initialQuestIndex = _quests.Num();
         }

         UTATContractStateUIProxy* proxy = NewObject<UTATContractStateUIProxy>(this, NAME_None, RF_Transient);
         proxy->Contract = questHandle;
         proxy->ContractState = quest.State;
         _quests.Add(proxy);
      }
   }

   ListView->SetListItems(_quests);

   if (initialQuestIndex >= 0)
   {
      ListView->NavigateToIndex(initialQuestIndex);
      ListView->SetSelectedIndex(initialQuestIndex);
   }
   else
   {
      ListView->SetSelectedIndex(0);
   }
}

void UTATContractJournalScreen::NativeDestruct()
{
   // TODO: maybe actually reuse some?
   _quests.Reset();

   Super::NativeDestruct();
}
