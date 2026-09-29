// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Executions/TATDamageThresholdExecution.h"

// tat
#include "Abilities/Attributes/TATPropHealthAttributeSet.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDamageThresholdExecution)

UTATDamageThresholdExecution::UTATDamageThresholdExecution()
{
   Attributes = { UAttributeBaseSet::GetHealthDamageAttribute(), UTATPropHealthAttributeSet::GetPropDamageAttribute() };
}


void UTATDamageThresholdExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& executionParams, OUT FGameplayEffectCustomExecutionOutput& outExecutionOutput) const
{
   const FGameplayEffectSpec& spec = executionParams.GetOwningSpec();
   const FGameplayEffectModifiedAttribute* modifiedAttribute = _FindModifiedAttribute(spec);
   if (modifiedAttribute == nullptr)
   {
      // this might be normal if there is no damage
      return;
   }

   const float magnitude = modifiedAttribute->TotalMagnitude;
   const int32 thresholdIndex = Thresholds.FindLastByPredicate([magnitude](const FTATThresholdExecutionEntry& entry) { return magnitude >= entry.MinMagnitude.GetValue(); });
   if (thresholdIndex == INDEX_NONE)
   {
      // again, potentially normal
      return;
   }

   const FTATThresholdExecutionEntry& threshold = Thresholds[thresholdIndex];

   if (UGameplayEffect* effectCdo = threshold.EffectToGrant.GetDefaultObject())
   {
      // Note: This was formerly doing initialize-linked-spec, but that inherited the set-by-caller tags
      //       which caused problems with DoT effects, since the execution implicitly uses it even if not
      //       intended. We may want to make that more explicit.
      FGameplayEffectSpec newSpec(effectCdo, spec.GetEffectContext());
      newSpec.SetLevel(thresholdIndex); //< This part is a little bit magic. Design didn't mind it, but could make it more explicit if needed
      executionParams.GetTargetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(newSpec);
   }
}

const FGameplayEffectModifiedAttribute* UTATDamageThresholdExecution::_FindModifiedAttribute(const FGameplayEffectSpec& spec) const
{
   // Find the first one that matches
   for (const FGameplayEffectModifiedAttribute& possibleAttr : spec.ModifiedAttributes)
   {
      if (Attributes.Contains(possibleAttr.Attribute))
      {
         return &possibleAttr;
      }
   }

   return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UTATDamageThresholdExecution::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   float previousThreshold = -1;
   for (int i = 0; i < Thresholds.Num(); ++i)
   {
      const FTATThresholdExecutionEntry& threshold = Thresholds[i];
      const float currentValue = threshold.MinMagnitude.GetValue();
      if (currentValue <= previousThreshold)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Damage Threshold execution %s is not strictly increasing: At index %d, %f <= (previous) %f"), *GetName(), i, currentValue, previousThreshold)));
      }

      previousThreshold = currentValue;
   }

   return context.GetNumErrors() + context.GetNumWarnings() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR
