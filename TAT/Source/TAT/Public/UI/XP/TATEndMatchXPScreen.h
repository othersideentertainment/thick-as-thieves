// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATScreenWidget.h"
#include "Progression/TATPlayerExperience.h"
#include "GameFramework/TATMatchPersistentTypes.h"

#include "TATEndMatchXPScreen.generated.h"

class UTATUIQueueAction;

// A UI screen showing the XP gained at the end of the match
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATEndMatchXPScreen : public UTATScreenWidget
{
   GENERATED_BODY()

public:
   static UTATUIQueueAction* CreateAction(const FMatchPersistentXPGainedData& persistentXPGainedData);

protected:
   UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
   int32 CurrentLevel;

   UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
   float CurrentXP;

   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   TArray<FTATFinishedMatchXPGainedInfo> EndMatchXPGained;
};
