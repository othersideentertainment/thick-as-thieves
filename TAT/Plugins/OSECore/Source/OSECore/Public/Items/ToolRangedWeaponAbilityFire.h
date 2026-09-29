// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Items/ToolRangedWeaponAbilityBase.h"

#include "ToolRangedWeaponAbilityFire.generated.h"

class UToolRangedWeaponComponent;

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UToolRangedWeaponAbilityFire : public UToolRangedWeaponAbilityBase
{
   GENERATED_BODY()

public:
   UToolRangedWeaponAbilityFire();

   UFUNCTION(BlueprintCallable, Category = "Weapon")
   void DeductProjectilesForActivation();
   
   // from UToolRangedWeaponAbilityBase
   virtual bool CanActivateAbility(const UToolRangedWeaponComponent& rangedWeaponComp) const override;
};
