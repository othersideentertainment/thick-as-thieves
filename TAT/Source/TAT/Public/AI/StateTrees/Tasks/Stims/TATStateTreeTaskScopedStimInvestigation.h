// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "AI/Perception/StimInfo.h"

#include "TATStateTreeTaskScopedStimInvestigation.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskScopedStimInvestigationData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   int StimID { INDEX_NONE };
};

USTRUCT(meta = (DisplayName = "Scoped Stim Investigation", Category = "TAT|AI|Stims"))
struct TAT_API FTATStateTreeTaskScopedStimInvestigation : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   
   using FInstanceDataType = FTATStateTreeTaskScopedStimInvestigationData;
   
   FTATStateTreeTaskScopedStimInvestigation() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   static bool TrySetInvestigationState(const UObject* owner, const FInstanceDataType& instanceData, EStimInvestigationState investigationState);
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
