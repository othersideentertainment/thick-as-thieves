// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskLoop.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskLoopData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   int MaxLoopCount { 0 };
   
   UPROPERTY(EditAnywhere, meta = (RefType = "int32"))
   FStateTreePropertyRef LoopCount;
};


USTRUCT()
struct TAT_API FTATStateTreeTaskLoop : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskLoopData;
	
   FTATStateTreeTaskLoop() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
