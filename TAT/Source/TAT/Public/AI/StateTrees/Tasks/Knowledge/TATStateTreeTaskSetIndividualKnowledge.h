// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskSetIndividualKnowledge.generated.h"


class AAIController;

USTRUCT()
struct FTATStateTreeTaskSetIndividualKnowledgeData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
   
   UPROPERTY(EditAnywhere, Category=In)
   AActor* Target = nullptr;

   UPROPERTY(EditAnywhere, Category=In)
   bool ShouldAdd { true };
   
   UPROPERTY(EditAnywhere, Category=In)
   FGameplayTag Tag;

   // If zero, will never expire.
   UPROPERTY(EditAnywhere, Category=In)
   float TagShouldExpireIn { 0.f };
};

USTRUCT()
struct TAT_API FTATStateTreeTaskSetIndividualKnowledge : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetIndividualKnowledgeData;
	
   FTATStateTreeTaskSetIndividualKnowledge() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
