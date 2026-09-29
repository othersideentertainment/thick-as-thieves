// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/TATGameplayAbility_EquipToolByIndex.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolComponent.h"

// tat
#include "Tools/TATToolSetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_EquipToolByIndex)


TSubclassOf<UToolComponent> UTATGameplayAbility_EquipToolByIndex::_GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const
{
   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(actorInfo->AvatarActor.Get()))
   {
      if (UTATToolSetComponent* toolSetComponent = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
      {
         if (UToolComponent* toolComponent = toolSetComponent->GetGearToolAtIndex(ToolIndexToEquip))
         {
            return toolComponent->GetClass();
         }
      }
   }

   return nullptr;
}
