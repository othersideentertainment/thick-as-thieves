// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "TATStateTreeTaskRetrieveKnowledgeData.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskRetrieveShareTargetData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/Engine.Actor"))
   FStateTreePropertyRef OutShareTargetActor;

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/CoreUObject.Vector"))
   FStateTreePropertyRef OutShareTargetLocation;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskRetrieveShareTarget : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskRetrieveShareTargetData;
	
   FTATStateTreeTaskRetrieveShareTarget() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
