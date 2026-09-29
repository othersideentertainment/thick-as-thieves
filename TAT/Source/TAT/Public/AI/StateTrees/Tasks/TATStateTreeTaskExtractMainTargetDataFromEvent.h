// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskExtractMainTargetDataFromEvent.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskExtractMainTargetDataFromEventData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/Engine.Actor"))
   FStateTreePropertyRef ResultActor;
};

USTRUCT(meta = (DisplayName = "Extract Target Data", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeTaskExtractMainTargetDataFromEvent : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskExtractMainTargetDataFromEventData;
	
   FTATStateTreeTaskExtractMainTargetDataFromEvent() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};

USTRUCT()
struct FTATStateTreeTaskExtractMainTargetDataFromTargetingGroupData : public FTATStateTreeTaskExtractMainTargetDataFromEventData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   FGameplayTag TargetingGroup;
};

USTRUCT(meta = (DisplayName = "Extract Target From Targeting Group", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeTaskExtractMainTargetDataFromTargetingGroup : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskExtractMainTargetDataFromTargetingGroupData;
	
   FTATStateTreeTaskExtractMainTargetDataFromTargetingGroup() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};

