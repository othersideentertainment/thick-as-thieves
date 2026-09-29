// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATRangedWeaponToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRangedWeaponToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATRangedWeaponToolComponent, Log, All)

UTATRangedWeaponToolComponent::UTATRangedWeaponToolComponent()
{
   // Ranged weapons by default don't need ammo
   AmmoType = ETATToolAmmoType::Infinite;

   // initial value
   ProjectileDamage.DamageAmount.SetValue(30);
}



