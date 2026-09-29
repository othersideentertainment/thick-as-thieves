// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolRangedWeaponAbilityFire.h"

// ose
#include "Items/ToolRangedWeaponComponent.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolRangedWeaponAbilityFire)

UToolRangedWeaponAbilityFire::UToolRangedWeaponAbilityFire()
   : Super()
{
}

void UToolRangedWeaponAbilityFire::DeductProjectilesForActivation()
{
   if (UToolRangedWeaponComponent* rangedWeaponComp = GetRangedWeaponComponent())
   {
      rangedWeaponComp->DeductProjectilesForActivation();
   }
}

bool UToolRangedWeaponAbilityFire::CanActivateAbility(const UToolRangedWeaponComponent& rangedWeaponComp) const
{
   return rangedWeaponComp.HasRequiredProjectilesForActivation();
}

