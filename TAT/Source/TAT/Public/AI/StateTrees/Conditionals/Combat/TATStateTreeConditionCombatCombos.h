// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

#include "TATStateTreeConditionCombatCombos.generated.h"

USTRUCT()
struct FTATStateTreeConditionCombatCombosData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   FGameplayTag RequiredComboTag;

   UPROPERTY(EditDefaultsOnly, Category="Context")
   AAIController* Owner { nullptr };
};

USTRUCT(meta = (DisplayName = "Check Next Combat Combo", Category = "TAT|AI|Combat"))
struct TAT_API FTATStateTreeConditionCombatCombos : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionCombatCombosData;

   FTATStateTreeConditionCombatCombos() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
