// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"
#include "TATStateTreeConditionTargetTimeSet.generated.h"

USTRUCT()
struct FTATStateTreeConditionTargetTimeSetData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* Controller { nullptr };
   
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag TargetingGroup;
   
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float ComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Test Time Set", Category = "TAT|AI|Targets"))
struct TAT_API FTATStateTreeConditionTargetTimeSet : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionTargetTimeSetData;

   FTATStateTreeConditionTargetTimeSet() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
#if WITH_EDITOR
   virtual FText GetDescription(const FGuid& id, FStateTreeDataView instanceDataView, const IStateTreeBindingLookup& bindingLookup, EStateTreeNodeFormatting formatting) const override;
   virtual FColor GetIconColor() const override { return UE::StateTree::Colors::Green; }
#endif
   
};
