// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "StateTreeTaskBase.h"
#include "TATStateTreeTaskSetCombatTarget.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskSetCombatTargetData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
   
   UPROPERTY(EditAnywhere, Category=In)
   TObjectPtr<AActor> Target = nullptr;
};


USTRUCT()
struct TAT_API FTATStateTreeTaskSetCombatTarget : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetCombatTargetData;
	
   FTATStateTreeTaskSetCombatTarget();
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
