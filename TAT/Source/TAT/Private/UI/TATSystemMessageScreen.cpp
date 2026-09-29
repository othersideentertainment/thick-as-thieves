// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/TATSystemMessageScreen.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "UI/Queue/TATUIQueue.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSystemMessageScreen)

UTATUIQueueAction* UTATSystemMessageScreen::CreateSystemMessageAction(const FText& systemMessage)
{
   return UTATScreenQueueAction::Create(UTATProjectSettings::Get().SystemMessageScreen,
      [systemMessage](UTATScreenWidget* screen)
      {
         CastChecked<UTATSystemMessageScreen>(screen)->SetSystemMessage(systemMessage);
      });
}
