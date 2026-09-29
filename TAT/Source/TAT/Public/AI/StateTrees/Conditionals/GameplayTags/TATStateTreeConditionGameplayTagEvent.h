// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "StateTreeConditionBase.h"

#include "TATStateTreeConditionGameplayTagEvent.generated.h"

USTRUCT()
struct FTATStateTreeConditionGameplayTagEventData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   FGameplayTag TagToMatch;
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   bool RequireExisting { true };
};

USTRUCT(meta = (DisplayName = "Check Gameplay Tag Event Matches", Category = "TAT|AI|Events|Gameplay Tags"))
struct TAT_API FTATStateTreeConditionGameplayTagEvent : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionGameplayTagEventData;

   FTATStateTreeConditionGameplayTagEvent() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
