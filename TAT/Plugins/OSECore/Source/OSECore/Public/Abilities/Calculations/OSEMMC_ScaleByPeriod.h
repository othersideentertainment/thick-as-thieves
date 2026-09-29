// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "GameplayModMagnitudeCalculation.h"

#include "OSEMMC_ScaleByPeriod.generated.h"

// Scales the coefficient with the effect's period, so the magnitude can be specified in change-per-second
UCLASS()
class OSECORE_API UOSEMMC_ScaleByPeriod : public UGameplayModMagnitudeCalculation
{
   GENERATED_BODY()

public:
   virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const override;
};
