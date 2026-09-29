// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "StateTreeConditionBase.h"

#include "TATStateTreeConditionGameplayTagIsValid.generated.h"

USTRUCT()
struct FTATStateTreeConditionGameplayTagIsValidData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   FGameplayTag TagToCheck;
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   bool SucceedIfValid = true;
};

USTRUCT(meta = (DisplayName = "Check Gameplay Tag Is Valid", Category = "Gameplay Tags"))
struct TAT_API FTATStateTreeConditionGameplayTagIsValid : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionGameplayTagIsValidData;

   FTATStateTreeConditionGameplayTagIsValid() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
