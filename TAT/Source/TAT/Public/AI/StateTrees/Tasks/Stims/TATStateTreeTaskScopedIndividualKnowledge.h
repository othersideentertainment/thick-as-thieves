// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskScopedIndividualKnowledge.generated.h"


class AAIController;

USTRUCT()
struct FTATStateTreeTaskScopedIndividualKnowledgeData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController { nullptr };

   UPROPERTY(EditAnywhere, Category = "In")
   TObjectPtr<AActor> Target { nullptr };
   
   UPROPERTY(EditAnywhere, Category="IndividualKnowledge")
   FGameplayTag KnowledgeTag;
};

USTRUCT(meta = (DisplayName = "Scoped Individual Knowledge", Category = "TAT|AI|Individual Knowledge"))
struct TAT_API FTATStateTreeTaskScopedIndividualKnowledge : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskScopedIndividualKnowledgeData;
   FTATStateTreeTaskScopedIndividualKnowledge() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

   bool SetIndividualKnowledge(const FStateTreeExecutionContext& context, bool shouldAdd) const;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
