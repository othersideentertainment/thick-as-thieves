// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Calculations/TATMMC_SwitchOnTargetTagPresence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMMC_SwitchOnTargetTagPresence)

float UTATMMC_SwitchOnTargetTagPresence::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const
{
   if (spec.CapturedTargetTags.GetActorTags().HasTag(TagToCheck))
   {
      return ValueWithTag;
   }
   else
   {
      return ValueWithoutTag;
   }
}
