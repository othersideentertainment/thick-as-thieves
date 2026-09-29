// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Items/OSEGameplayAbility_EquipToolBase.h"
#include "Abilities/Items/OSEGameplayAbility_EquipToolInterface.h"

// ue4

#include "OSEGameplayAbility_EquipToolByClass.generated.h"

class UToolComponent;

UCLASS(ClassGroup = (Ability), Blueprintable, EditInlineNew)
class OSECORE_API UOSEGameplayAbility_EquipToolByClass
   : public UOSEGameplayAbility_EquipToolBase
   , public IOSEGameplayAbility_EquipToolInterface
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE");
   TSubclassOf<UToolComponent> ToolToEquip = nullptr;

   // from IOSEGameplayAbility_EquipToolInterface
   virtual TSubclassOf<UToolComponent> GetEquipToolClass_Implementation() const { return ToolToEquip; }

protected:
   // from UOSEGameplayAbility_EquipToolBase
   virtual TSubclassOf<UToolComponent> _GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const { return ToolToEquip; }
};
