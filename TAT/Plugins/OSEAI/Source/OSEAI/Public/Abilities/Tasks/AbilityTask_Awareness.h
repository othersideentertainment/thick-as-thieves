// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// tat
#include "AI/Alertness/AlertnessEnums.h"

// ose
#include "Abilities/Tasks/AbilityTask_EventTimeLine.h"

#include "AbilityTask_Awareness.generated.h"

UCLASS(BlueprintType)
class OSEAI_API UEventTimelineActionRaiseAlertnessLevelToAtLeast : public UEventTimelineAction
{
   GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const override;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EAlertnessLevel AlertnessLevel = EAlertnessLevel::Neutral;
};

UCLASS(BlueprintType)
class OSEAI_API UEventTimelineActionLowerAlertnessLevelToAtMost : public UEventTimelineAction
{
   GENERATED_BODY()

public:
   virtual void EvaluateAction(AActor* actor, AActor* otherActor) const override;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   EAlertnessLevel AlertnessLevel = EAlertnessLevel::Neutral;
};
