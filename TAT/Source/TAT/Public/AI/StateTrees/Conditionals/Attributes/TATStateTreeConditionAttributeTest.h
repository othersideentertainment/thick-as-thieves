// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

// ose
#include "AttributeSet.h"
#include "Math/OSEMathFunctionLibrary.h"
#include "TATStateTreeConditionAttributeTest.generated.h"

USTRUCT()
struct FTATStateTreeConditionAttributeTestData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* Controller { nullptr };

   UPROPERTY(EditAnywhere)
   FGameplayAttribute ActualValueAttributeSet { nullptr };
   UPROPERTY(EditAnywhere)
   FGameplayAttribute MaxValueAttributeSet { nullptr };
   
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float NormalizedComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Test Attribute", Category = "TAT|AI|Attributes"))
struct TAT_API FTATStateTreeConditionAttributeTest : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionAttributeTestData;

   FTATStateTreeConditionAttributeTest() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
   
#if WITH_EDITOR
   virtual FText GetDescription(const FGuid& id, FStateTreeDataView instanceDataView, const IStateTreeBindingLookup& bindingLookup, EStateTreeNodeFormatting formatting) const override;
   virtual FColor GetIconColor() const override { return UE::StateTree::Colors::Green; }
#endif
};
