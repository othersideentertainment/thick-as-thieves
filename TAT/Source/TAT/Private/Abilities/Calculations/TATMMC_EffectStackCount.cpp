// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Calculations/TATMMC_EffectStackCount.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMMC_EffectStackCount)

float UTATMMC_EffectStackCount::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const
{
   return static_cast<float>(spec.GetStackCount());
}

