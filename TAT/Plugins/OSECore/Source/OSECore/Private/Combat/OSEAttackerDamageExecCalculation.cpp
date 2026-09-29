// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/OSEAttackerDamageExecCalculation.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Combat/CombatSettings.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAttackerDamageExecCalculation)

// Declare the attributes to capture and define how we want to capture them from the Source and Target.
struct OSEAttackerDamageStatics
{
   DECLARE_ATTRIBUTE_CAPTUREDEF(AttackDamage);
   DECLARE_ATTRIBUTE_CAPTUREDEF(AttackDamageMultiplier);

   OSEAttackerDamageStatics()
   {
      // Snapshot happens at time of GESpec creation (bool param here is whether or not to snapshot the stat)
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, AttackDamage, Source, false);
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, AttackDamageMultiplier, Source, true);
   }
};

static const OSEAttackerDamageStatics& GetAttackerDamageStatics()
{
   static OSEAttackerDamageStatics sStatics;
   return sStatics;
}

UOSEAttackerDamageExecCalculation::UOSEAttackerDamageExecCalculation()
{
   RelevantAttributesToCapture.Add(GetAttackerDamageStatics().AttackDamageDef);
   RelevantAttributesToCapture.Add(GetAttackerDamageStatics().AttackDamageMultiplierDef);
}

void UOSEAttackerDamageExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& executionParams, OUT FGameplayEffectCustomExecutionOutput& outExecutionOutput) const
{
   UAbilitySystemComponent* sourceAbilitySystemComponent = executionParams.GetSourceAbilitySystemComponent();

   AActor* sourceActor = sourceAbilitySystemComponent ? sourceAbilitySystemComponent->GetAvatarActor() : nullptr;

   const FGameplayEffectSpec& spec = executionParams.GetOwningSpec();

   // Gather the tags from the source and target as that can affect which buffs should be used
   const FGameplayTagContainer* sourceTags = spec.CapturedSourceTags.GetAggregatedTags();

   FAggregatorEvaluateParameters evaluationParameters;
   evaluationParameters.SourceTags = sourceTags;

   // What is our base damage dealt for this attack?
   const UCombatSettings& settings = UCombatSettings::Get();
   float damageDealt = spec.GetSetByCallerMagnitude(settings.DamageEffectMagnitudeTag, false, 0.0f);

   // What's our (simple, TEMP?) damage multiplier?
   // TODO: This is where we'd grab all our stats -- strength, attack power, whatever, and use them to influence the damage we deal
   float damageMultiplier = 1.0f;
   if (executionParams.AttemptCalculateCapturedAttributeMagnitude(GetAttackerDamageStatics().AttackDamageMultiplierDef, evaluationParameters, damageMultiplier))
   {
      damageDealt *= damageMultiplier;
   }

   // this is our damage output value; could be zero
   outExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(GetAttackerDamageStatics().AttackDamageProperty, EGameplayModOp::Override, damageDealt));
}

