// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Abilities/Items/OSEGameplayAbility_EquipToolBase.h"

#include "TATGameplayAbility_EquipToolByIndex.generated.h"

// Equip a specific tool in our toolset by index
UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class TAT_API UTATGameplayAbility_EquipToolByIndex : public UOSEGameplayAbility_EquipToolBase
{
   GENERATED_BODY()
public:

   UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0", UIMin = "0"))
   int32 ToolIndexToEquip = 0;

protected:
   virtual TSubclassOf<UToolComponent> _GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const override;
};

