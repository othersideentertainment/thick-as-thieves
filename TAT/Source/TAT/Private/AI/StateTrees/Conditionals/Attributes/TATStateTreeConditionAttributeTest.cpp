// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/Attributes/TATStateTreeConditionAttributeTest.h"

// ue
#include "StateTreeExecutionContext.h"

// ose
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Math/OSEMathFunctionLibrary.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionAttributeTest)

#define LOCTEXT_NAMESPACE "TATStateTreeConditionAttributeTest"

bool FTATStateTreeConditionAttributeTest::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);
   if (instanceData.ActualValueAttributeSet == nullptr)
      return false;

   if (instanceData.MaxValueAttributeSet == nullptr)
      return false;

   const AAIController* aiController = instanceData.Controller;
   if (aiController == nullptr)
      return false;

   if (const APawn* pawn = aiController->GetPawn())
   {
      if (const UAbilitySystemComponent* abilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(pawn))
      {
         const float attributeValue = abilitySystemComponent->GetNumericAttribute(instanceData.ActualValueAttributeSet);
         const float maxAttributeValue = abilitySystemComponent->GetNumericAttribute(instanceData.MaxValueAttributeSet);
         const float normalizedValue = FMath::GetMappedRangeValueClamped(FVector2f(0.f, maxAttributeValue), FVector2f(0.f, 1.f), attributeValue);
         return UOSEMathFunctionLibrary::CompareFloats(normalizedValue, instanceData.NormalizedComparisonValue, instanceData.ComparisonMethod);
      }
   }
   return false;
}


#if WITH_EDITOR
FText FTATStateTreeConditionAttributeTest::GetDescription(const FGuid& id,
                                                          const FStateTreeDataView instanceDataView,
                                                          const IStateTreeBindingLookup& bindingLookup,
                                                          const EStateTreeNodeFormatting formatting) const
{
   const FInstanceDataType* instanceData = instanceDataView.GetPtr<FInstanceDataType>();
   check(instanceData);
   const FText format = LOCTEXT("TATStateTreeConditionAttributeTest_Description", "{Attribute} {Comparision} {Value}");
   return FText::FormatNamed(format,
      TEXT("Attribute"), FText::FromString(*instanceData->ActualValueAttributeSet.GetName()),
      TEXT("Comparision"), FText::FromString(*UOSEMathFunctionLibrary::GetComparisonMethodDescription(instanceData->ComparisonMethod)),
      TEXT("Value"), instanceData->NormalizedComparisonValue);
}
#endif
#undef LOCTEXT_NAMESPACE
