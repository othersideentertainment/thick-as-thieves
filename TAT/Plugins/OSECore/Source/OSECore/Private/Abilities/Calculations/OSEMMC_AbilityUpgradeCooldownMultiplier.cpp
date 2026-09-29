// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Calculations/OSEMMC_AbilityUpgradeCooldownMultiplier.h"

#include "Abilities/OSEGameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMMC_AbilityUpgradeCooldownMultiplier)

float UOSEMMC_AbilityUpgradeCooldownMultiplier::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const
{
   // It is not ideal that this uses the ability instance rather than the CDO, but only the source ASC is available (not any other actor info), which is only
   // probably the same as for the ability. But an alternate implementation would be to use the fields on the CDO directly in concert with the ASC. That moves
   // more logic here.
   const UOSEGameplayAbility* ability = Cast<UOSEGameplayAbility>(spec.GetContext().GetAbilityInstance_NotReplicated());
   if (!ability) return 1.f;

   float cooldownMultiplier;
   if (ability->TryGetCooldownMultiplier(cooldownMultiplier))
   {
      return cooldownMultiplier;
   }
   else
   {
      return 1.f;
   }

}

