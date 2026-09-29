// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"

#include "TATMMC_CharacterHealthRegen.generated.h"

/**
 * Calculates the amount of health to regen for the character based off of the Attribute.
 * If the Character is a TATCharacter, also ensures that health won't regenerate over the specified health block limits.
 */
UCLASS()
class TAT_API UTATMMC_CharacterHealthRegen : public UGameplayModMagnitudeCalculation
{
   GENERATED_BODY()

   UTATMMC_CharacterHealthRegen();
   
   virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

   static float GetMaxRegenerableHealth(const TArray<int32>& healthBarBlocks, const float currentHealth, const float maxHealth);
};

