// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/OSEGameplayAbility_SyncedAnimationEvent.h"

// ose
#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_SyncedAnimationEvent)

UOSEGameplayAbility_SyncedAnimationEvent::UOSEGameplayAbility_SyncedAnimationEvent()
   : Super()
{
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}


bool UOSEGameplayAbility_SyncedAnimationEvent::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* actorInfo, const FGameplayEventData* payload) const
{
   if (!Super::ShouldAbilityRespondToEvent(actorInfo, payload))
   {
      return false;
   }

   return _MeetsConstraints(actorInfo->AvatarActor.Get(), payload->Target);
}

void UOSEGameplayAbility_SyncedAnimationEvent::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   if (triggerEventData && CommitAbility(handle, ownerInfo, activationInfo))
   {
      const AActor* source = ownerInfo->AvatarActor.Get();
      const AActor* target = triggerEventData->Target;

      // is this parameter packing too specific for the OSE layer?
      FGameplayEventData eventData;
      eventData.Instigator = source;
      eventData.Target = target;
      eventData.OptionalObject = SyncedAnimationDataAsset;

      if (!SkipSource && !BP_ShouldSkipSourceAnimation(source, target))
      {
         UAbilitySystemComponent* asc = ownerInfo->AbilitySystemComponent.Get();
         check(asc);
         FScopedPredictionWindow newScopedWindow(asc, true);
         asc->HandleGameplayEvent(PlayerEventTag, &eventData);
      }

      if (!SkipTarget)
      {
         if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(target))
         {
            eventData.EventMagnitude = 1.f;
            FScopedPredictionWindow newScopedWindow(asc, true);
            asc->HandleGameplayEvent(PlayerEventTag, &eventData);
         }
      }

      BP_OnTriggered(source, target, *ownerInfo);
   }

   const bool bReplicateEndAbility = false;
   const bool bWasCancelled = false;
   EndAbility(handle, ownerInfo, activationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UOSEGameplayAbility_SyncedAnimationEvent::_MeetsConstraints(const AActor* sourceActor, const AActor* targetActor) const
{
   if (targetActor == nullptr)
   {
      return false;
   }

   FSyncedAnimationSearchParameters searchParams;
   searchParams.Source = sourceActor;
   searchParams.Target = targetActor;
   return SyncedAnimationDataAsset ? SyncedAnimationDataAsset->SyncedAnimationData.MeetsConstraints(searchParams, true) : false;
}

