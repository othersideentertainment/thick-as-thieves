// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Items/OSEGameplayAbility_EquipToolInCategories.h"

// ue4

#include "TATGameplayAbility_EquipWeapon.generated.h"

class UToolComponent;

UCLASS(ClassGroup = (Ability), Blueprintable)
class TAT_API UTATGameplayAbility_EquipWeapon : public UOSEGameplayAbility_EquipToolInCategories
{
   GENERATED_BODY()

public:
   UTATGameplayAbility_EquipWeapon();

protected:
   // from UOSEGameplayAbility_EquipToolInCategories
   virtual TSubclassOf<UToolComponent> _GetLastEquippedToolClass(const FGameplayAbilityActorInfo* actorInfo) const override;
};
