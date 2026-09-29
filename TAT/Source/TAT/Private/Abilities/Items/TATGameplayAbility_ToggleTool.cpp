// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/TATGameplayAbility_ToggleTool.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"

// tat
#include "Tools/TATToolSetComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_ToggleTool)


UTATGameplayAbility_ToggleTool::UTATGameplayAbility_ToggleTool()
{
   // local only to reduce GA instances
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UTATGameplayAbility_ToggleTool::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, FGameplayTagContainer* optionalRelevantTags) const
{
   if (!Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags))
   {
      return false;
   }

   if (ToolClass)
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(actorInfo->AvatarActor.Get()))
      {
         return toolSetInterface->HasToolClass(ToolClass);
      }
   }

   return false;
}

void UTATGameplayAbility_ToggleTool::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   if (ToolClass)
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(actorInfo->AvatarActor.Get()))
      {
         if (UTATToolSetComponent* toolSetComponent = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
         {
            toolSetComponent->ToggleEquippedTool(ToolClass);
         }
      }
   }

   // end ability immediately
   const bool replicateEndAbility = true;
   const bool wasCancelled = false;
   EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}
