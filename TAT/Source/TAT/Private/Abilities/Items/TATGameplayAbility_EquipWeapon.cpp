// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/TATGameplayAbility_EquipWeapon.h"

// ose
#include "Player/TATCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_EquipWeapon)

// ue4

UTATGameplayAbility_EquipWeapon::UTATGameplayAbility_EquipWeapon()
{
}

TSubclassOf<UToolComponent> UTATGameplayAbility_EquipWeapon::_GetLastEquippedToolClass(const FGameplayAbilityActorInfo* actorInfo) const
{
   if (ATATCharacter* tatCharacter = Cast<ATATCharacter>(_GetCharacter(actorInfo)))
   {
      return tatCharacter->GetRecentlyEquippedWeaponClass();
   }
   return nullptr;
}

