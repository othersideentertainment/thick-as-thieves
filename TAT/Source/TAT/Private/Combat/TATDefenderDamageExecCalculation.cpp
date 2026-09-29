// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/TATDefenderDamageExecCalculation.h"

// tat
#include "AI/TATAIController.h"
#include "Abilities/TATAttributeSet.h"
#include "Abilities/Attributes/TATPropHealthAttributeSet.h"
#include "Combat/TATCombatDamageBonusTargetInterface.h"
#include "Player/TATCharacter.h"

// ose
#include "OSECommon.h"
#include "Abilities/Attributes/AttributeBaseSet.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDefenderDamageExecCalculation)

DEFINE_LOG_CATEGORY_STATIC(LogTATDefenderDamageExecCalculation, Log, All);

struct TATDefenderDamageStatics
{
   DECLARE_ATTRIBUTE_CAPTUREDEF(SneakAttackDamageMultiplier);
   DECLARE_ATTRIBUTE_CAPTUREDEF(CounterAttackDamageMultiplier);
   DECLARE_ATTRIBUTE_CAPTUREDEF(PropDamageReductionMultiplier);

   TATDefenderDamageStatics()
   {
      DEFINE_ATTRIBUTE_CAPTUREDEF(UTATAttributeSet, SneakAttackDamageMultiplier, Source, true);
      DEFINE_ATTRIBUTE_CAPTUREDEF(UTATAttributeSet, CounterAttackDamageMultiplier, Source, true);
      DEFINE_ATTRIBUTE_CAPTUREDEF(UTATPropHealthAttributeSet, PropDamageReductionMultiplier, Target, false);
   }
};

static const TATDefenderDamageStatics& GetDefenderDamageStatics()
{
   static TATDefenderDamageStatics sStatics;
   return sStatics;
}

UTATDefenderDamageExecCalculation::UTATDefenderDamageExecCalculation()
{
   RelevantAttributesToCapture.Add(GetDefenderDamageStatics().SneakAttackDamageMultiplierDef);
   RelevantAttributesToCapture.Add(GetDefenderDamageStatics().CounterAttackDamageMultiplierDef);
   RelevantAttributesToCapture.Add(GetDefenderDamageStatics().PropDamageReductionMultiplierDef);
#if WITH_EDITORONLY_DATA
   InvalidScopedModifierAttributes.Add(GetDefenderDamageStatics().SneakAttackDamageMultiplierDef);
   InvalidScopedModifierAttributes.Add(GetDefenderDamageStatics().CounterAttackDamageMultiplierDef);
   InvalidScopedModifierAttributes.Add(GetDefenderDamageStatics().PropDamageReductionMultiplierDef);
#endif
}

float UTATDefenderDamageExecCalculation::_ComputeDamageMagnitude(const FGameplayEffectCustomExecutionParameters& executionParams) const
{
   const float baseDamage = Super::_ComputeDamageMagnitude(executionParams);

   UAbilitySystemComponent* sourceAbilitySystemComponent = executionParams.GetSourceAbilitySystemComponent();
   UAbilitySystemComponent* targetAbilitySystemComponent = executionParams.GetTargetAbilitySystemComponent();

   AActor* sourceActor = sourceAbilitySystemComponent ? sourceAbilitySystemComponent->GetAvatarActor() : nullptr;
   AActor* targetActor = targetAbilitySystemComponent ? targetAbilitySystemComponent->GetAvatarActor() : nullptr;
   ATATCharacter* sourceCharacter = Cast<ATATCharacter>(sourceActor);
   ITATCombatDamageBonusTargetInterface* targetCombatBonusDamageInterface = Cast<ITATCombatDamageBonusTargetInterface>(targetActor);

   const FGameplayEffectSpec& spec = executionParams.GetOwningSpec();

   // Gather the tags from the source and target as that can affect which buffs should be used
   const FGameplayTagContainer* sourceTags = spec.CapturedSourceTags.GetAggregatedTags();
   const FGameplayTagContainer* targetTags = spec.CapturedTargetTags.GetAggregatedTags();

   FAggregatorEvaluateParameters evaluationParameters;
   evaluationParameters.SourceTags = sourceTags;
   evaluationParameters.TargetTags = targetTags;

   float damageMultiplier = 1.0f;

   float propDamageMultiplier = 1.0f;
   if (executionParams.AttemptCalculateCapturedAttributeMagnitude(GetDefenderDamageStatics().PropDamageReductionMultiplierDef, evaluationParameters, propDamageMultiplier))
   {
      damageMultiplier *= propDamageMultiplier;
   }

   // Check that we have a bonus damage interface
   // Also check that this is not a self-damage event (such as a periodic DoT)
   if (targetCombatBonusDamageInterface && (sourceActor != targetActor) && (spec.GetPeriod() == FGameplayEffectConstants::NO_PERIOD))
   {
      const FGameplayEffectContextHandle& contextHandle = spec.GetEffectContext();
      const FHitResult* hitResult = contextHandle.IsValid() ? contextHandle.GetHitResult() : nullptr;

      // Use interface to determine when sneak attacks are available
      if (targetCombatBonusDamageInterface->CanBeSneakAttackedByActor(sourceActor))
      {
         // Get our sneak attack damage multiplier from our attributes
         float sneakAttackDamageMultiplier = 1.0f;
         if (executionParams.AttemptCalculateCapturedAttributeMagnitude(GetDefenderDamageStatics().SneakAttackDamageMultiplierDef, evaluationParameters, sneakAttackDamageMultiplier))
         {
            damageMultiplier *= sneakAttackDamageMultiplier;
         }

         if (sourceCharacter)
         {
            if (hitResult)
            {
               sourceCharacter->AuthorityOnSneakAttack(targetActor, *hitResult);
            }
            else
            {
               UE_LOG(LogTATDefenderDamageExecCalculation, Error, TEXT("No hit result on sneak attack?"));
            }

            targetCombatBonusDamageInterface->OnSneakAttackedByActor(sourceActor);
         }
      }

      // Use interface to determine when counter attacks are available
      if (targetCombatBonusDamageInterface->CanBeCounterAttackedByActor(sourceActor))
      {
         // Get our counter attack damage multiplier from our attributes
         float counterAttackDamageMultiplier = 1.0f;
         if (executionParams.AttemptCalculateCapturedAttributeMagnitude(GetDefenderDamageStatics().CounterAttackDamageMultiplierDef, evaluationParameters, counterAttackDamageMultiplier))
         {
            damageMultiplier *= counterAttackDamageMultiplier;
         }

         if (sourceCharacter)
         {
            if (hitResult)
            {
               sourceCharacter->AuthorityOnCounterAttack(targetActor, *hitResult);
            }
            else
            {
               UE_LOG(LogTATDefenderDamageExecCalculation, Error, TEXT("No hit result on counter attack?"));
            }
            
            targetCombatBonusDamageInterface->OnCounterAttackedByActor(sourceActor);
         }
      }
   }
   return baseDamage * damageMultiplier;
}

void UTATDefenderDamageExecCalculation::_HandleComputedDamage(float damage, const FGameplayEffectCustomExecutionParameters& executionParams, FGameplayEffectCustomExecutionOutput& outExecutionOutput) const
{
   if (damage > 0)
   {
      outExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UTATPropHealthAttributeSet::GetPropDamageAttribute(), EGameplayModOp::Additive, damage));
   }
}
