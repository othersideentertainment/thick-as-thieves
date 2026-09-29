// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "Conditions/OSECondition.h"
#include "Character/OSECharacterBase.h"
#include "Math/OSEMathFunctionLibrary.h"

// ue5
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSECondition)



bool UOSECondition::Evaluate(const FOSEConditionContext& context) const
{
   // != on bools is the same as exclusive or, which is the same as inverting based on Inverted.
   return Inverted != _EvaluateInternal(context);
}

bool FOSEConditionSet::Satisfied(const FOSEConditionContext& context) const
{
   for (const UOSECondition* condition : Conditions)
   {
      if (!condition || !condition->Evaluate(context))
      {
         return false;
      }
   }

   return true;
}

bool UOSEHasAllTagsCondition::_EvaluateInternal(const FOSEConditionContext& context) const
{
  if (!context.GetCharacter())
    return false;

  const UAbilitySystemComponent* asc = context.GetCharacter()->GetAbilitySystemComponent();

  if (!asc)
     return false;
  
  return asc->HasAllMatchingGameplayTags(Tags);
}

bool UOSEHasAnyTagsCondition::_EvaluateInternal(const FOSEConditionContext& context) const
{
   if (!context.GetCharacter())
      return false;

   const UAbilitySystemComponent* asc = context.GetCharacter()->GetAbilitySystemComponent();

   if (!asc)
      return false;

   return asc->HasAnyMatchingGameplayTags(Tags);
}

bool UOSEAttributeCondition::_EvaluateInternal(const FOSEConditionContext& context) const
{
   if (!context.GetCharacter())
      return false;

   const UAbilitySystemComponent* asc = context.GetCharacter()->GetAbilitySystemComponent();

   if (!asc)
      return false;

   if (!Attribute.IsValid())
      return false;

   if (!MaxAttribute.IsValid())
      return false;

   float attributeValue = asc->GetNumericAttribute(Attribute);
   float attributeMax = asc->GetNumericAttribute(MaxAttribute);
   float percentage = 100.0f * (attributeValue / attributeMax);

   return UOSEMathFunctionLibrary::CompareFloats(percentage, Percentage, Comparison);
}


