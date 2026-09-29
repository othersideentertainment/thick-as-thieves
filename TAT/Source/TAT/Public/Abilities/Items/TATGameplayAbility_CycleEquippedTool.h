// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Abilities/Items/OSEGameplayAbility_EquipToolFromSet.h"

#include "TATGameplayAbility_CycleEquippedTool.generated.h"

enum class ETATPlayerToolsetDirection : uint8;
class ATATCharacter;

//---------------------------------------------------------------------------------------------------
/// Ability class that allows cycling clockwise (or counter-clockwise) through tools in our toolset
//---------------------------------------------------------------------------------------------------

UCLASS(ClassGroup = (Ability), Abstract, Blueprintable)
class TAT_API UTATGameplayAbility_CycleEquippedTool : public UOSEGameplayAbility_EquipToolFromSet
{
   GENERATED_BODY()

protected:
   // from UOSEGameplayAbility_EquipToolFromSet
   virtual const TArray<UToolComponent*>& _GetToolsToSelectFrom(const FGameplayAbilityActorInfo* actorInfo) const override;

   UPROPERTY(EditDefaultsOnly)
   bool _skipUnusableTools = false;
};
