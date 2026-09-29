// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TATUIQueueUtils.generated.h"

UCLASS()
class TAT_API UTATUIQueueUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
   // Creates an action to either display the old user widget or the new common UI version of the system message widget 
   // to make sure it is displayed on top of other widgets correctly and to avoid input issues between the two types of widgets.
   UFUNCTION(BlueprintCallable, BlueprintPure, Category="UIQueueUtils|System", meta = (WorldContext = "worldContextObject"))
   static UTATUIQueueAction* AddSystemMessageAction(const UObject* worldContextObject, const FText& systemMessage);
};
