// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue 
#include "ScalableFloat.h"
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskExtractValueFromScalableFloat.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskScalableFloatData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = "In")
   FScalableFloat ScalableFloat;

   UPROPERTY(EditAnywhere, Category = "In")
   float Level = 0;

   // Needs to be of type "double" because blueprint "float" properties are actually doubles..
   // EdGraphSchema_K2.cpp:4041 (PC_Real instead of PC_Float)
   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "double"))
   FStateTreePropertyRef OutFloatValue;
};

USTRUCT(meta = (DisplayName = "Extract Scalable Float", Category = "TAT|Data|Extractions"))
struct TAT_API FTATStateTreeTaskExtractValueFromScalableFloat : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskScalableFloatData;
	
   FTATStateTreeTaskExtractValueFromScalableFloat() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

};
