// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Quests/TATQuestNoteScreen.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "UI/Queue/TATUIQueue.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestNoteScreen)

UTATUIQueueAction* UTATQuestNoteScreen::CreateQuestNoteAction(const FText& noteText)
{
   return CreateCustomAction(UTATProjectSettings::Get().QuestNoteScreen, noteText);
}

UTATUIQueueAction* UTATQuestNoteScreen::CreateCustomAction(const TSoftClassPtr<UTATQuestNoteScreen>& screen, const FText& noteText)
{
   return UTATScreenQueueAction::Create(screen,
      [noteText](UTATScreenWidget* screen)
      {
         CastChecked<UTATQuestNoteScreen>(screen)->SetNoteText(noteText);
      });
}
