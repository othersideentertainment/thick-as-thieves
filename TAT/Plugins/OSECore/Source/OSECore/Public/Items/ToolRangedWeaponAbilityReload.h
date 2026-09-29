// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Items/ToolRangedWeaponAbilityBase.h"

#include "ToolRangedWeaponAbilityReload.generated.h"

class UToolRangedWeaponComponent;

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UToolRangedWeaponAbilityReload : public UToolRangedWeaponAbilityBase
{
   GENERATED_BODY()

public:
   UToolRangedWeaponAbilityReload();
   
   UFUNCTION(BlueprintCallable, Category = "Weapon")
   void DoReload();

   // from UToolRangedWeaponAbilityBase
   virtual bool CanActivateAbility(const UToolRangedWeaponComponent& rangedWeaponComp) const override;
};
