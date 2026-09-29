// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/TATSystemMessageActivatableWidget.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "UI/Queue/TATUIQueue.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSystemMessageActivatableWidget)


UTATUIQueueAction* UTATSystemMessageActivatableWidget::CreateActivatableSystemMessageAction(const FText& systemMessage)
{
   return UTATActivatableWidgetQueueAction::Create(UTATProjectSettings::Get().SystemMessageActivatableWidget, [systemMessage](UCommonActivatableWidget* widget)
      {
         CastChecked<UTATSystemMessageActivatableWidget>(widget)->SetSystemMessage(systemMessage);
      });
}
