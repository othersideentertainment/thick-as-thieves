// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/OSEGameplayAbility_EquipToolInCategories.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/Items/OSEGameplayAbility_EquipToolInterface.h"
#include "Character/OSECharacterBase.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_EquipToolInCategories)

const TArray<UToolComponent*>& UOSEGameplayAbility_EquipToolInCategories::_GetToolsToSelectFrom(const FGameplayAbilityActorInfo* actorInfo) const
{
   _toolsToSelectFrom.Reset();
   if (AOSECharacterBase* character = _GetCharacter(actorInfo))
   {
      TScriptInterface<IToolSetInterface> toolSet = character->GetToolSetInterface();

      // all tools that match our filter
      _toolsToSelectFrom = toolSet->FindToolsInCategories(ToolCategories);

      if (ExcludeHideInUI)
      {
         _toolsToSelectFrom.RemoveAll([this](UToolComponent* tool) { return tool->GetToolInfo().HideInUI; });
      }
   }
   return _toolsToSelectFrom;
}

