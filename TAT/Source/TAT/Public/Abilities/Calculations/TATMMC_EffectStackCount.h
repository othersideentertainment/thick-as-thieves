// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"

#include "TATMMC_EffectStackCount.generated.h"

// A modifier that gets the current stack count of the applied effect
// Useful for dividing out in order to treat stack counts as equal to a single application
UCLASS()
class TAT_API UTATMMC_EffectStackCount : public UGameplayModMagnitudeCalculation
{
   GENERATED_BODY()

public:
   virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const override;
};
