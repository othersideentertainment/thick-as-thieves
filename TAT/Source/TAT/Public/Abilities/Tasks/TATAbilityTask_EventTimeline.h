// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/Tasks/AbilityTask_EventTimeLine.h"

#include "TATAbilityTask_EventTimeline.generated.h"

UCLASS(Blueprintable, abstract)
class TAT_API UEventTimelineAction_BlueprintBase : public UEventTimelineAction
{
	GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const override;

   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "EvaluateAction"))
   void BP_EvaluateAction(AActor* actor, AActor* otherActor) const;
	
};
