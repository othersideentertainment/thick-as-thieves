// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/OSEDefenderDamageExecCalculation.h"

// ose
#include "OSECommon.h"
#include "OSEProjectSettings.h"
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "Combat/CombatSettings.h"
#include "Player/OSEPlayerState.h"
#include "Player/OSEPlayerStats.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDefenderDamageExecCalculation)

UE_DEFINE_GAMEPLAY_TAG(TAG_Effect_ExecutionParam_Damage, "Effect.ExecutionParam.Damage");

// Declare the attributes to capture and define how we want to capture them from the Source and Target.
struct OSEDefenderDamageStatics
{
   DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReductionMultiplier);
   DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

   OSEDefenderDamageStatics()
   {
      // Snapshot happens at time of GESpec creation (bool param here is whether or not to snapshot the stat)
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, DamageReductionMultiplier, Target, true);
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, Health, Target, false);
   }
};

static const OSEDefenderDamageStatics& GetDefenderDamageStatics()
{
   static OSEDefenderDamageStatics sStatics;
   return sStatics;
}

UOSEDefenderDamageExecCalculation::UOSEDefenderDamageExecCalculation()
{
   RelevantAttributesToCapture.Add(GetDefenderDamageStatics().DamageReductionMultiplierDef);
   RelevantAttributesToCapture.Add(GetDefenderDamageStatics().HealthDef);
#if WITH_EDITORONLY_DATA
   InvalidScopedModifierAttributes.Add(GetDefenderDamageStatics().HealthDef);
   ValidTransientAggregatorIdentifiers.AddTag(TAG_Effect_ExecutionParam_Damage);
#endif
}

float UOSEDefenderDamageExecCalculation::_ComputeDamageMagnitude(const FGameplayEffectCustomExecutionParameters& executionParams) const
{
   const FGameplayEffectSpec& spec = executionParams.GetOwningSpec();

   // Gather the tags from the source and target as that can affect which buffs should be used
   const FGameplayTagContainer* sourceTags = spec.CapturedSourceTags.GetAggregatedTags();
   const FGameplayTagContainer* targetTags = spec.CapturedTargetTags.GetAggregatedTags();

   FAggregatorEvaluateParameters evaluationParameters;
   evaluationParameters.SourceTags = sourceTags;
   evaluationParameters.TargetTags = targetTags;

   // How much damage were we dealt?
   float damageReceived = 0.0f;

   // Capture optional damage value set on the damage GE as a CalculationModifier under the ExecutionCalculation
   executionParams.AttemptCalculateTransientAggregatorMagnitude(TAG_Effect_ExecutionParam_Damage, evaluationParameters, damageReceived);

   // Add SetByCaller damage if it exists
   const UCombatSettings& combatSettings = UCombatSettings::Get();
   damageReceived += spec.GetSetByCallerMagnitude(combatSettings.DamageEffectMagnitudeTag, false, 0.0f);

   // How much do we reduce the damage dealt by?
   float damageReductionMultiplier = 0.0f;
   if (executionParams.AttemptCalculateCapturedAttributeMagnitude(GetDefenderDamageStatics().DamageReductionMultiplierDef, evaluationParameters, damageReductionMultiplier))
   {
      damageReceived *= damageReductionMultiplier;
   }

   return damageReceived;
}

void UOSEDefenderDamageExecCalculation::_HandleComputedDamage(float damage, const FGameplayEffectCustomExecutionParameters& executionParams, FGameplayEffectCustomExecutionOutput& outExecutionOutput) const
{

}

void UOSEDefenderDamageExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& executionParams, OUT FGameplayEffectCustomExecutionOutput& outExecutionOutput) const
{
   const float damageReceived = _ComputeDamageMagnitude(executionParams);

   // apply final damage, if there is damage to apply
   if (damageReceived > 0.f)
   {
      outExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UAttributeBaseSet::GetHealthDamageAttribute(), EGameplayModOp::Additive, damageReceived));

      _HandleComputedDamage(damageReceived, executionParams, outExecutionOutput);

      // as seen mentioned on UDN, since we're on the server here, and all our damage calculations come through here, and
      // this is the final stop before applying damage to the defender, we can run code here that cares about damage values and sources
      UAbilitySystemComponent* sourceAbilitySystemComponent = executionParams.GetSourceAbilitySystemComponent();
      UAbilitySystemComponent* targetAbilitySystemComponent = executionParams.GetTargetAbilitySystemComponent();
      AActor* sourceActor = sourceAbilitySystemComponent ? sourceAbilitySystemComponent->GetAvatarActor() : nullptr;
      AActor* targetActor = targetAbilitySystemComponent ? targetAbilitySystemComponent->GetAvatarActor() : nullptr;

      // the target sure needs an ASC because that's who is taking damage, but I suppose the source doesn't.  we can try and pull a
      // source from the effect causer if it doesn't have an asc...
      if (!sourceActor)
      {
         sourceActor = executionParams.GetOwningSpec().GetContext().GetEffectCauser();
      }

      // source == target is like us taking damage from traps or environmental hazards, only count that as incoming damage for now
      if (sourceActor != targetActor)
      {
         if (AOSEPlayerState* sourcePS = UOSECommon::GetPlayerState<AOSEPlayerState>(sourceActor))
         {
            sourcePS->AuthorityAddOutgoingDamageLogEntry(targetActor, damageReceived);
         }
      }
      if (AOSEPlayerState* targetPS = UOSECommon::GetPlayerState<AOSEPlayerState>(targetActor))
      {
         targetPS->AuthorityAddIncomingDamageLogEntry(sourceActor, damageReceived);
      }
   }
}

