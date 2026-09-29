// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolRangedWeaponAbilityReload.h"

// ose
#include "Items/ToolRangedWeaponComponent.h"

// ue4
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolRangedWeaponAbilityReload)

UToolRangedWeaponAbilityReload::UToolRangedWeaponAbilityReload()
   : Super()
{
}

void UToolRangedWeaponAbilityReload::DoReload()
{
   if (UToolRangedWeaponComponent* rangedWeaponComp = GetRangedWeaponComponent())
   {
      rangedWeaponComp->DoReload();
   }
}

bool UToolRangedWeaponAbilityReload::CanActivateAbility(const UToolRangedWeaponComponent& rangedWeaponComp) const
{
   return rangedWeaponComp.CanReload();
}

