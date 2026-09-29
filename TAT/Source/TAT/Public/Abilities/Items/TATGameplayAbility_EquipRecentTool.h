// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Items/OSEGameplayAbility_EquipToolBase.h"

// ue4

#include "TATGameplayAbility_EquipRecentTool.generated.h"

UCLASS(ClassGroup = (Ability), Blueprintable)
class TAT_API UTATGameplayAbility_EquipRecentTool : public UOSEGameplayAbility_EquipToolBase
{
   GENERATED_BODY()

public:
   UTATGameplayAbility_EquipRecentTool();

protected:
   virtual TSubclassOf<UToolComponent> _GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const override;
};
