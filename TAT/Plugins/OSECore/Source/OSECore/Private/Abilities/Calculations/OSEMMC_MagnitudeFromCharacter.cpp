// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "Abilities/Calculations/OSEMMC_MagnitudeFromCharacter.h"
#include "Character/OSECharacterBase.h"

#include "Abilities/OSEGameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMMC_MagnitudeFromCharacter)

float UOSEMMC_MagnitudeFromCharacter::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& spec) const
{
   AOSECharacterBase* character = Cast<AOSECharacterBase>(spec.GetEffectContext().GetEffectCauser());
   check(character);
   return GetMagnitudeFromCharacter(character);
}

