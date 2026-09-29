// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Calculations/OSEMMC_ScaleByPeriod.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMMC_ScaleByPeriod)

float UOSEMMC_ScaleByPeriod::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const
{
   return spec.GetPeriod();
}
