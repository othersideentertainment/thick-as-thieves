// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/TATGameplayAbility_EquipRecentTool.h"

// ose
#include "Player/TATCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_EquipRecentTool)

// ue4

UTATGameplayAbility_EquipRecentTool::UTATGameplayAbility_EquipRecentTool()
{
}

TSubclassOf<UToolComponent> UTATGameplayAbility_EquipRecentTool::_GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const
{
   if (ATATCharacter* tatCharacter = Cast<ATATCharacter>(_GetCharacter(actorInfo)))
   {
      return tatCharacter->GetRecentlyEquippedToolClass();
   }
   return nullptr;
}

