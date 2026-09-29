// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"

#include "OSEMMC_AbilityUpgradeCooldownMultiplier.generated.h"

// A modifier that reads a cooldown multiplier from the source ability (for use by cooldowns affected by upgrades)
UCLASS()
class OSECORE_API UOSEMMC_AbilityUpgradeCooldownMultiplier : public UGameplayModMagnitudeCalculation
{
   GENERATED_BODY()

public:
   virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const override;
   
};
