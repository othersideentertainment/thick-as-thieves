// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Items/OSEGameplayAbility_EquipToolBase.h"

#include "OSEGameplayAbility_EquipToolFromSet.generated.h"

class UToolComponent;

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class OSECORE_API UOSEGameplayAbility_EquipToolFromSet : public UOSEGameplayAbility_EquipToolBase
{
   GENERATED_BODY()

public:

   UOSEGameplayAbility_EquipToolFromSet();

   // Controls the direction of tool cycling. (True = forward, False = backward)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE");
   bool DirectionSwitch = false;

   // Loop around the set, or stop at the start/end?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE");
   bool Loop = false;

   // Controls whether to default to first/last tool in the list when we can't resolve previous state. (True = first tool, False = last)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE");
   bool DefaultToFirstOrLastTool = true;

protected:

   // from UOSEGameplayAbility_EquipToolBase
   virtual TSubclassOf<UToolComponent> _GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const override;
   virtual void _OnToolClassEquipped(const FGameplayAbilityActorInfo* actorInfo, const TSubclassOf<UToolComponent>& toolClass) override;

   // Returns the tools that should be cycled through by _GetToolClassToEquip(). Implementations are expected to assign and return mutable array _toolsToSelectFrom.
   virtual const TArray<UToolComponent*>& _GetToolsToSelectFrom(const FGameplayAbilityActorInfo* actorInfo) const { return _toolsToSelectFrom; }
   virtual TSubclassOf<UToolComponent> _GetLastEquippedToolClass(const FGameplayAbilityActorInfo* actorInfo) const { return _lastEquippedToolClass; }

   // Intended for use as return-by-reference value of const method _GetToolsToSelectFrom(). 
   // Marked as mutable because _GetToolsToSelectFrom() must be called from UOSEGameplayAbility_EquipToolBase::_GetToolClassToEquip(), 
   // which is called from UGameplayAbility::CanActivateAbility(), which is a UE4 const method.
   mutable TArray<UToolComponent*> _toolsToSelectFrom;

private:
   // default impl
   UPROPERTY(Transient)
   TSubclassOf<UToolComponent> _lastEquippedToolClass;
};
