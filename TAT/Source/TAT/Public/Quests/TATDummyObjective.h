// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Quests/TATQuestObjective.h"
#include "Quests/TATQuestObjectiveTracker.h"

#include "TATDummyObjective.generated.h"

// Can delete if surface area grows enough to be annoying to maintain
USTRUCT(meta = (DisplayName = "Dummy Objective"))
struct TAT_API FTATDummyObjectiveInfo : public FTATQuestObjectiveInfo
{
   GENERATED_BODY()

public:
   virtual FTATObjectiveTrackerPayload CreateTracker() const override;
   virtual FString GetDebugDescription() const override { return TEXT("Dummy"); }
};

UCLASS()
class TAT_API UTATDummyObjectiveTracker : public UTATQuestObjectiveTracker
{
   GENERATED_BODY()

public:
   virtual void CheatComplete() override;
};
