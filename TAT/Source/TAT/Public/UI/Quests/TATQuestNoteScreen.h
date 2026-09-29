// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "UI/TATScreenWidget.h"

// ue
#include "CoreMinimal.h"

#include "TATQuestNoteScreen.generated.h"

class UTATUIQueueAction;

// TODO: rename to contract note screen?
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATQuestNoteScreen : public UTATScreenWidget
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintPure, Category="Quest|Flow")
   static UTATUIQueueAction* CreateQuestNoteAction(const FText& noteText);
   
   static UTATUIQueueAction* CreateCustomAction(const TSoftClassPtr<UTATQuestNoteScreen>& screen, const FText& noteText);

   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void SetNoteText(const FText& noteText);
};
