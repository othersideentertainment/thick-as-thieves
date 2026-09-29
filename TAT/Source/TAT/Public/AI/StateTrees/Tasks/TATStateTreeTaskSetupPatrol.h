// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskSetupPatrol.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskCompletePatrolData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category=Context)
   TObjectPtr<AAIController> AIController { nullptr };
};

USTRUCT()
struct FTATStateTreeTaskCompleteBrokenPathData : public FTATStateTreeTaskCompletePatrolData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, meta = (RefType = "bool"))
   FStateTreePropertyRef PatrolWasInterrupted;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/CoreUObject.Vector"))
   FStateTreePropertyRef PatrolBrokenLocation;
};

USTRUCT()
struct FTATStateTreeTaskSetupPatrolData : public FTATStateTreeTaskCompleteBrokenPathData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/TAT.PatrolPoint"))
   FStateTreePropertyRef PatrolData;

   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/CoreUObject.Vector"))
   FStateTreePropertyRef PatrolWorldLocation;
};


USTRUCT(Category="TAT|AI|Patrol", DisplayName="Setup Patrol Positions")
struct TAT_API FTATStateTreeTaskSetupPatrol : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetupPatrolData;
	
   FTATStateTreeTaskSetupPatrol() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT(Category="TAT|AI|Patrol", DisplayName="Complete Broken Segment")
struct TAT_API FTATStateTreeTaskCompleteBrokenSegment : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskCompleteBrokenPathData;
	
   FTATStateTreeTaskCompleteBrokenSegment() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT(Category="TAT|AI|Patrol", DisplayName="Complete Patrol Segment")
struct TAT_API FTATStateTreeTaskCompletePatrolSegment : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskCompletePatrolData;
	
   FTATStateTreeTaskCompletePatrolSegment() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
