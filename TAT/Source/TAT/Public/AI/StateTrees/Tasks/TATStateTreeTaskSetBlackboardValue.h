// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ue
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskSetBlackboardValue.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskSetBlackboardValueData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category=Parameter)
   FVector Location = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, Category=Parameter)
   FName BlackboardKeyName;   
};

// Note this ONLY allows the setting of vectors in its current iteration.
USTRUCT()
struct TAT_API FTATStateTreeTaskSetBlackboardValue : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetBlackboardValueData;
	
   FTATStateTreeTaskSetBlackboardValue() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT()
struct FTATStateTreeTaskSetBlackboardValueActorData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category=Parameter)
   AActor* Target = nullptr;

   UPROPERTY(EditAnywhere, Category=Parameter)
   FName BlackboardKeyName;   
};

USTRUCT()
struct TAT_API FTATStateTreeTaskSetBlackboardActorValue : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetBlackboardValueActorData;
	
   FTATStateTreeTaskSetBlackboardActorValue() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
