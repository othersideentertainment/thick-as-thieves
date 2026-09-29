// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Items/OSEGameplayAbility_EquipToolFromSet.h"

// ue4

#include "OSEGameplayAbility_EquipToolInCategories.generated.h"

class UToolComponent;

UCLASS(ClassGroup = (Ability), Blueprintable)
class OSECORE_API UOSEGameplayAbility_EquipToolInCategories : public UOSEGameplayAbility_EquipToolFromSet
{
   GENERATED_BODY()

public:
   // Which types of tools do we care about?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE");
   FGameplayTagContainer ToolCategories;

   // Exclude tools that are HideInUI?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|OSE");
   bool ExcludeHideInUI = false;

protected:
   // from UOSEGameplayAbility_EquipToolFromSet
   virtual const TArray<UToolComponent*>& _GetToolsToSelectFrom(const FGameplayAbilityActorInfo* actorInfo) const override;
};
