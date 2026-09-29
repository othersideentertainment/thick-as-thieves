// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/OSEGameplayAbility_ImmediateEnd.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_ImmediateEnd)

UOSEGameplayAbility_ImmediatateEnd::UOSEGameplayAbility_ImmediatateEnd()
{
}

void UOSEGameplayAbility_ImmediatateEnd::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   CommitAbility(handle, actorInfo, activationInfo);

   bool bReplicateEndAbility = false;
   bool bWasCancelled = false;
   EndAbility(handle, actorInfo, activationInfo, bReplicateEndAbility, bWasCancelled);
}

