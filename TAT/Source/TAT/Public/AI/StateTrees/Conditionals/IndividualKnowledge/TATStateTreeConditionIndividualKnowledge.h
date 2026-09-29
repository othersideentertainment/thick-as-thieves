// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

#include "TATStateTreeConditionIndividualKnowledge.generated.h"

USTRUCT()
struct FTATStateTreeConditionIndividualKnowledgeData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* Controller { nullptr };

   UPROPERTY(EditAnywhere)
   AActor* Target { nullptr };   
};

USTRUCT(meta = (DisplayName = "Has Individual Knowledge", Category = "TAT|AI|Individual Knowledge"))
struct TAT_API FTATStateTreeConditionIndividualKnowledge : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionIndividualKnowledgeData;

   FTATStateTreeConditionIndividualKnowledge() = default;
   
   UPROPERTY(EditAnywhere)
   bool Invert { false };
   
   UPROPERTY(EditAnywhere)
   FGameplayTag KnowledgeTagToCheck;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;

};
