// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreeConditionBase.h"
#include "TATStateTreeConditionActors.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeConditionActor_HasGameplayTagData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   AActor* Target { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   FGameplayTag RequiredGameplayTag;

   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   bool Invert { false };   
};

USTRUCT(meta = (DisplayName = "Check Target Has Gameplay Tag", Category = "TAT|AI|Actors"))
struct TAT_API FTATStateTreeConditionActor_HasGameplayTag : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionActor_HasGameplayTagData;

   FTATStateTreeConditionActor_HasGameplayTag() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
