// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskSetValue.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskSetValueBoolData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   bool ValueToUse { false };

   UPROPERTY(EditAnywhere, meta = (RefType = "bool"))
   FStateTreePropertyRef ValueToSet;
};


USTRUCT(meta = (DisplayName = "Set Value (bool)", Category = "TAT|Set Values"))
struct TAT_API FTATStateTreeTaskSetValueBool : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskSetValueBoolData;
	
   FTATStateTreeTaskSetValueBool() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
