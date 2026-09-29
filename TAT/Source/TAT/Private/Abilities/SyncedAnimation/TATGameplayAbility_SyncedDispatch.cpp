// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/SyncedAnimation/TATGameplayAbility_SyncedDispatch.h"

// ose
#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_SyncedDispatch)

UTATGameplayAbility_SyncedDispatch::UTATGameplayAbility_SyncedDispatch()
   : Super()
{
}

void UTATGameplayAbility_SyncedDispatch::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   if (triggerEventData && triggerEventData->OptionalObject && CommitAbility(handle, ownerInfo, activationInfo))
   {
      const AActor* source = ownerInfo->AvatarActor.Get();
      const AActor* target = triggerEventData->Target;

      // is this parameter packing too specific for the OSE layer?
      FGameplayEventData eventData;
      eventData.Instigator = source;
      eventData.Target = target;
      eventData.OptionalObject = triggerEventData->OptionalObject;

      // send to source
      {
         UAbilitySystemComponent* asc = ownerInfo->AbilitySystemComponent.Get();
         check(asc);
         FScopedPredictionWindow newScopedWindow(asc, true);
         asc->HandleGameplayEvent(PlayerEventTag, &eventData);
      }


      // send to target
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(target))
      {
         eventData.EventMagnitude = 1.f;
         FScopedPredictionWindow newScopedWindow(asc, true);
         asc->HandleGameplayEvent(PlayerEventTag, &eventData);
      }

      BP_OnTriggered(source, target, *ownerInfo);
   }

   const bool bReplicateEndAbility = false;
   const bool bWasCancelled = false;
   EndAbility(handle, ownerInfo, activationInfo, bReplicateEndAbility, bWasCancelled);
}

