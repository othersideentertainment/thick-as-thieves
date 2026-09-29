// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/OSEGameplayAbility_EquipToolFromSet.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_EquipToolFromSet)

UOSEGameplayAbility_EquipToolFromSet::UOSEGameplayAbility_EquipToolFromSet()
{
   // this version needs to be instanced per actor so that we can keep track of some per-actor state.
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

TSubclassOf<UToolComponent> UOSEGameplayAbility_EquipToolFromSet::_GetToolClassToEquip(const FGameplayAbilityActorInfo* actorInfo) const
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_OSEGameplayAbility_EquipToolInCategories_GetToolClassToEquip);

   if (const AOSECharacterBase* character = _GetCharacter(actorInfo))
   {
      const TScriptInterface<IToolSetInterface> toolSet = character->GetToolSetInterface();

      // all tools that match our filter
      const TArray<UToolComponent*>& tools = _GetToolsToSelectFrom(actorInfo);

      // can't do anything without some tools
      const int numTools = tools.Num();
      if (numTools == 0)
      {
         return nullptr;
      }

      UToolComponent* currentTool = toolSet->GetCurrentTool();

      UToolComponent* const* lastToolPtr = tools.FindByPredicate([&](UToolComponent* tool)
         {
            return tool->GetClass() == _GetLastEquippedToolClass(actorInfo);
         }
      );

      // is our current tool in the list?
      int searchToolIdx = INDEX_NONE;
      int nextToolIdx = INDEX_NONE;
      if (tools.Find(currentTool, searchToolIdx))
      {
         // it's in the list, so go to the next tool
         nextToolIdx = DirectionSwitch ? (searchToolIdx + 1) : (searchToolIdx - 1);
      }
      else if (lastToolPtr && tools.Find(*lastToolPtr, searchToolIdx))
      {
         // it's not in the list, go back to whatever we had equipped previously, or the first thing
         // NOTE: Another popular game moves the index along in this case but that behavior feels bad to me
         nextToolIdx = searchToolIdx;
      }
      else
      {
         // default to front or back depending on our settings
         nextToolIdx = DefaultToFirstOrLastTool ? 0 : (numTools - 1);
      }

      // optionally support looping through the list instead of just scrolling to the start/end of it
      if (Loop)
      {
         if (nextToolIdx == numTools)
         {
            nextToolIdx = 0;
         }
         else if (nextToolIdx < 0)
         {
            nextToolIdx = (numTools - 1);
         }
      }

      // did we end up with a valid index?  that is where we're going next
      if (tools.IsValidIndex(nextToolIdx))
      {
         return tools[nextToolIdx]->GetClass();
      }
   }
   
   return nullptr;
}

void UOSEGameplayAbility_EquipToolFromSet::_OnToolClassEquipped(const FGameplayAbilityActorInfo* actorInfo, const TSubclassOf<UToolComponent>& toolClass)
{
   _lastEquippedToolClass = toolClass;
}

