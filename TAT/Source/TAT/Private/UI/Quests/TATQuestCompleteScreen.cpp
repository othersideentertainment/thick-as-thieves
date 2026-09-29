// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Quests/TATQuestCompleteScreen.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "UI/Queue/TATUIQueue.h"

// ue5
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestCompleteScreen)

UTATUIQueueAction* UTATQuestCompleteScreen::CreateMissionAction(const FGameplayTag& questTag, bool didComplete)
{
   return UTATScreenQueueAction::Create(UTATProjectSettings::Get().MissionCompleteScreen,
      [questTag, didComplete](UTATScreenWidget* screen)
      {
         CastChecked<UTATQuestCompleteScreen>(screen)->SetQuest(questTag, didComplete);
      });
}

UTATUIQueueAction* UTATQuestCompleteScreen::CreateContractAction(const FGameplayTag& questTag, bool didComplete)
{
   return UTATScreenQueueAction::Create(UTATProjectSettings::Get().ContractCompleteScreen,
      [questTag, didComplete](UTATScreenWidget* screen)
      {
         CastChecked<UTATQuestCompleteScreen>(screen)->SetQuest(questTag, didComplete);
      });
}
