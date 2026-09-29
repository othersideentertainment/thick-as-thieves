// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "Abilities/Calculations/TATMMC_CharacterHealthRegen.h"
#include "Player/TATCharacter.h"
#include "Character/TATCharacterAIBase.h"

// ose
#include "Abilities/Attributes/AttributeBaseSet.h"

// ue
#include "GameplayEffectExecutionCalculation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMMC_CharacterHealthRegen)

// Declare the attributes to capture and define how we want to capture them from the Source and Target.
struct OSEHealthDataStatics
{
   DECLARE_ATTRIBUTE_CAPTUREDEF(HealthRegenRate);
   DECLARE_ATTRIBUTE_CAPTUREDEF(Health);
   DECLARE_ATTRIBUTE_CAPTUREDEF(HealthMax);

   OSEHealthDataStatics()
   {
      // Snapshot happens at time of GESpec creation (bool param here is whether or not to snapshot the stat)
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, HealthRegenRate, Source, true);
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, Health, Source, false);
      DEFINE_ATTRIBUTE_CAPTUREDEF(UAttributeBaseSet, HealthMax, Source, true);
   }
};

static const OSEHealthDataStatics& GetHealthDataStatics()
{
   static OSEHealthDataStatics sStatics;
   return sStatics;
}

UTATMMC_CharacterHealthRegen::UTATMMC_CharacterHealthRegen()
{
   RelevantAttributesToCapture.Add(GetHealthDataStatics().HealthRegenRateDef);
   RelevantAttributesToCapture.Add(GetHealthDataStatics().HealthDef);
   RelevantAttributesToCapture.Add(GetHealthDataStatics().HealthMaxDef);
}

float UTATMMC_CharacterHealthRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
   float healthRegenRate = 0.0f;
   FAggregatorEvaluateParameters evaluateParameters;
   GetCapturedAttributeMagnitude(GetHealthDataStatics().HealthRegenRateDef, Spec, evaluateParameters, healthRegenRate);
   float healthRegenAmount = healthRegenRate * Spec.Period;

   if (const ATATCharacter* tatCharacter = Cast<ATATCharacter>(Spec.GetEffectContext().GetEffectCauser()))
   {
      if (tatCharacter->GetIsInCombat())
      {
         healthRegenAmount = 0.0f;
      }
      else
      {
         const TArray<int32>& healthBarBlocks = tatCharacter->GetHealthBarBlocks();

         if (healthBarBlocks.Num() > 0)
         {
            float currentHealth = 0.0f;
            float maxHealth = 0.0f;
            GetCapturedAttributeMagnitude(GetHealthDataStatics().HealthDef, Spec, evaluateParameters, currentHealth);
            GetCapturedAttributeMagnitude(GetHealthDataStatics().HealthMaxDef, Spec, evaluateParameters, maxHealth);

            float maxRegenerableHealth = GetMaxRegenerableHealth(healthBarBlocks, currentHealth, maxHealth);

            // If we're at or close to the max regenerable health, set the regen amount to the difference, even if that difference is 0!
            if (currentHealth + healthRegenAmount > maxRegenerableHealth)
            {
               healthRegenAmount = maxRegenerableHealth - currentHealth;
            }
         }
      }
   }

   else if (const ATATCharacterAIBase* tatAICharacter = Cast<ATATCharacterAIBase>(Spec.GetEffectContext().GetEffectCauser()))
   {
      if (tatAICharacter->GetAlertnessLevel() == EAlertnessLevel::Combat)
      {
         healthRegenAmount = 0.0f;
      }
   }

   return healthRegenAmount;
}

// static
float UTATMMC_CharacterHealthRegen::GetMaxRegenerableHealth(const TArray<int32>& healthBarBlocks, const float currentHealth, const float maxHealth)
{
   // Figure out what is the max health we can regenerate to based on the health blocks
   float maxRegenerableHealth = maxHealth;
   for (int32 i = healthBarBlocks.Num() - 1; i >= 0; --i)
   {
      // Health Bar Blocks are integers from 1-100 that represent the percentage of max health they take up
      float healthBlockAmount = maxHealth * (healthBarBlocks[i] / 100.0f);
      
      // If we have more health than the current health block, we can continue to regen so stop here
      if (currentHealth > maxRegenerableHealth - healthBlockAmount)
      {
         break;
      }

      // Otherwise subtract the amount of health calculated in this block to the current max regenerable health
      maxRegenerableHealth -= healthBlockAmount;
   }

   return maxRegenerableHealth;
}
