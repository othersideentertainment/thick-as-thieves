// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreeConditionBase.h"

#include "TATStateTreeConditionStateIsActive.generated.h"

USTRUCT()
struct FTATStateTreeConditionStateIsActiveInstanceData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FStateTreeStateLink State;

   // If true, condition will succeed if the linked State is active in the state tree.
   // if false, condition will succeed if the linked State is NOT active in the state tree.
   UPROPERTY(EditDefaultsOnly)
   bool SucceedIfActive = true;
};

USTRUCT(meta = (DisplayName = "Check Linked State Is Active", Category = "TAT|AI|Events"))
struct TAT_API FTATStateTreeConditionStateIsActive : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionStateIsActiveInstanceData;

   FTATStateTreeConditionStateIsActive() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
