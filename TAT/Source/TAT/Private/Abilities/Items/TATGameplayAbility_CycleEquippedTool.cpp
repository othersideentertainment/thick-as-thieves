// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/TATGameplayAbility_CycleEquippedTool.h"

// tat
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolSetComponent.h"

// ose
#include "Items/ToolSetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_CycleEquippedTool)

const TArray<UToolComponent*>& UTATGameplayAbility_CycleEquippedTool::_GetToolsToSelectFrom(const FGameplayAbilityActorInfo* actorInfo) const
{
   _toolsToSelectFrom.Reset();

   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(actorInfo->AvatarActor.Get()))
   {
      if (UTATToolSetComponent* toolSetComponent = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
      {
         const int32 numGearTools = toolSetComponent->GetNumGearTools();

         for (int32 i = 0; i < numGearTools; i++)
         {
            UToolComponent* toolComponent = toolSetComponent->GetGearToolAtIndex(i);
            if (IsValid(toolComponent))
            {
               // For tools implementing UTATToolComponent, skip those that cannot currently be used
               if (const UTATToolComponent* tatToolComponent = Cast<UTATToolComponent>(toolComponent))
               {
                  if (_skipUnusableTools && !tatToolComponent->CanCurrentlyBeUsed())
                  {
                     continue;
                  }
               }
               _toolsToSelectFrom.Add(toolComponent);
            }
         }
      }
   }

   return _toolsToSelectFrom;
}

