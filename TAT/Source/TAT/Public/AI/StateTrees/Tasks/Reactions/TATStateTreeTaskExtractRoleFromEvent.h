// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue 
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskExtractRoleFromEvent.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskRoleFromEventData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/GameplayTags.GameplayTag"))
   FStateTreePropertyRef ResultRegisteredRoleTag;
};

USTRUCT(meta = (DisplayName = "Extract Reaction Role", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeTaskExtractRoleFromEvent : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskRoleFromEventData;
	
   FTATStateTreeTaskExtractRoleFromEvent() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};
