// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat
#include "UI/TATScreenWidget.h"

// ue
#include "CoreMinimal.h"

#include "TATQuestCompleteScreen.generated.h"

class UTATUIQueueAction;
struct FGameplayTag;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATQuestCompleteScreen : public UTATScreenWidget
{
   GENERATED_BODY()
   
public:
   static UTATUIQueueAction* CreateMissionAction(const FGameplayTag& questTag, bool didComplete);
   static UTATUIQueueAction* CreateContractAction(const FGameplayTag& questTag, bool didComplete);

   UFUNCTION(BlueprintImplementableEvent)
   void SetQuest(const FGameplayTag& questTag, bool didComplete);
};
