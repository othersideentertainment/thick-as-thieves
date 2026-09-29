// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue 
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskExtractStimDataFromEvent.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskStimDataInstanceData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, meta=(RefType="/Script/OSEAI.StimInfo"))
   FStateTreePropertyRef ResultStim;

   UPROPERTY(EditAnywhere, meta=(RefType="/Script/GameplayTags.GameplayTagContainer", Optional))
   FStateTreePropertyRef ResultBehaviors;

   // DG: BP float is actually a double..
   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "double", Optional))
   FStateTreePropertyRef OutOuterSearchRadius;
   
   // DG: BP float is actually a double..
   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "double", Optional))
   FStateTreePropertyRef OutInnerSearchRadius;

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "bool", Optional))
   FStateTreePropertyRef StimLocationCanUpdate;
};

USTRUCT(meta = (DisplayName = "Extract Stim Data", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeTaskExtractStimDataFromEvent : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskStimDataInstanceData;
	
   FTATStateTreeTaskExtractStimDataFromEvent() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};
