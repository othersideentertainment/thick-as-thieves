// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Items/TATGameplayAbility_UnequipCurrentTool.h"

// ose
#include "Items/ToolSetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_UnequipCurrentTool)

UTATGameplayAbility_UnequipCurrentTool::UTATGameplayAbility_UnequipCurrentTool()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UTATGameplayAbility_UnequipCurrentTool::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(actorInfo->AvatarActor.Get()))
   {
      toolSetInterface->UnequipCurrentTool();
   }

   const bool wasCancelled = false;
   const bool replicateEndAbility = true;
   EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}
