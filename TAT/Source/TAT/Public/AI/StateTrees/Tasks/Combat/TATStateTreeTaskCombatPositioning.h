// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "StateTreeTaskBase.h"
#include "AI/Combat/TATCombatPositioningComponent.h"
#include "TATStateTreeTaskCombatPositioning.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskCombatPositioningData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "Parameter")
   ECombatPosition CombatPosition { ECombatPosition::Close };
   
   ECombatPosition PreviousCombatPosition { ECombatPosition::Close };
};

USTRUCT()
struct TAT_API FTATStateTreeTaskCombatPositioning : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskCombatPositioningData;
	
   FTATStateTreeTaskCombatPositioning();
   bool HandleSetCombatPosition(const FStateTreeExecutionContext& context, bool shouldReset) const;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};
