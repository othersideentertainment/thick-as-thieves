// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATActivatableWidget.h"

// ue
#include "CoreMinimal.h"

#include "TATSystemMessageActivatableWidget.generated.h"

class UTATUIQueueAction;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATSystemMessageActivatableWidget : public UTATActivatableWidget
{
   GENERATED_BODY()
public:
   static UTATUIQueueAction* CreateActivatableSystemMessageAction(const FText& systemMessage);

   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void SetSystemMessage(const FText& systemMessage);
};
