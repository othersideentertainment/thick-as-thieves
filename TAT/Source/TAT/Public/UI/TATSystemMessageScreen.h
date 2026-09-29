// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "UI/TATScreenWidget.h"

// ue
#include "CoreMinimal.h"

#include "TATSystemMessageScreen.generated.h"

class UTATUIQueueAction;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATSystemMessageScreen : public UTATScreenWidget
{
   GENERATED_BODY()
public:
   static UTATUIQueueAction* CreateSystemMessageAction(const FText& systemMessage);
   
   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void SetSystemMessage(const FText& systemMessage);
};
