// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "TATStateTreeTaskPersistPatrolData.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskPersistPatrolDataType
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskPersistPatrolData : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskPersistPatrolDataType;
	
   FTATStateTreeTaskPersistPatrolData();
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
