// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreeTaskBase.h"

// TAT
#include "AI/Alertness/AlertnessEnums.h"
#include "TATStateTreeTaskSetAlertness.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskSetAlertnessData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
   
   UPROPERTY(EditAnywhere, Category = "In")
   EAlertnessLevel AlertnessLevel = EAlertnessLevel::Neutral;

   // If true, this task will continue running until any alertness transition
   // animations have finished playing. Otherwise it will finish immediately.
   UPROPERTY(EditAnywhere, Category = "In")
   bool RunUntilAnimationFinished = true;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskSetAlertness : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetAlertnessData;
	
   FTATStateTreeTaskSetAlertness() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
};
