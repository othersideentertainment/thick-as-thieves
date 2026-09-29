// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Queue/TATUIQueueUtils.h"

// tat
#include "TATGameInstance.h"
#include "UI/TATLayoutSubsystem.h"
#include "UI/TATLayoutWidget.h"
#include "UI/TATSystemMessageActivatableWidget.h"
#include "UI/TATSystemMessageScreen.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUIQueueUtils)

UTATUIQueueAction* UTATUIQueueUtils::AddSystemMessageAction(const UObject* worldContextObject, const FText& systemMessage)
{
   UWorld* world = worldContextObject->GetWorld();

   //Only allow a message if the world is not being destroyed.
   if (world && !world->bIsTearingDown)
   {
      const UTATGameInstance& gameInstance = UTATGameInstance::Get(worldContextObject);

      // If this message is going to be displayed while there are activatable widgets on screen,
      // we need to use the activatable version so it is shown on the correct layer in CommonUI.
      UTATLayoutSubsystem* layoutSubsystem = gameInstance.GetSubsystem<UTATLayoutSubsystem>();
      if (layoutSubsystem != nullptr)
      {
         if (UTATLayoutWidget* layoutWidget = layoutSubsystem->GetPlayerLayout(gameInstance.GetLocalPlayerByIndex(0)))
         {
            if (layoutWidget->IsAnyWidgetActive())
            {
               return UTATSystemMessageActivatableWidget::CreateActivatableSystemMessageAction(systemMessage);
            }
         }
      }

      // If there are no activatable widgets on screen then use the TATScreenWidget version 
      // to avoid input issues between the two types of UI widgets.
      return UTATSystemMessageScreen::CreateSystemMessageAction(systemMessage);
   }

   return nullptr;
}
