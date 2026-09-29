// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/SyncedAnimation/TATGameplayAbility_SyncedSelect.h"

// ose
#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"
#include "Abilities/OSESyncedAnimations.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_SyncedSelect)

UTATGameplayAbility_SyncedSelect::UTATGameplayAbility_SyncedSelect()
   : Super()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}


bool UTATGameplayAbility_SyncedSelect::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* actorInfo, const FGameplayEventData* payload) const
{
   if (!Super::ShouldAbilityRespondToEvent(actorInfo, payload))
   {
      return false;
   }

   return FindMatchingAnimation(SyncedAnimationAssets, actorInfo->AvatarActor.Get(), payload->Target) != nullptr;
}

void UTATGameplayAbility_SyncedSelect::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   if (triggerEventData && CommitAbility(handle, ownerInfo, activationInfo))
   {
      const AActor* source = ownerInfo->AvatarActor.Get();
      const AActor* target = triggerEventData->Target;

      const UOSESyncedAnimationDataAsset* animAsset = FindMatchingAnimation(SyncedAnimationAssets, source, target);

      // is this parameter packing too specific for the OSE layer?
      FGameplayEventData eventData;
      eventData.Instigator = source;
      eventData.Target = target;
      eventData.OptionalObject = animAsset;

      UAbilitySystemComponent* asc = ownerInfo->AbilitySystemComponent.Get();
      check(asc);
      FScopedPredictionWindow newScopedWindow(asc, true);
      asc->HandleGameplayEvent(DispatchEventTag, &eventData);
   }

   const bool bReplicateEndAbility = false;
   const bool bWasCancelled = false;
   EndAbility(handle, ownerInfo, activationInfo, bReplicateEndAbility, bWasCancelled);
}

const UOSESyncedAnimationDataAsset* UTATGameplayAbility_SyncedSelect::FindMatchingAnimation(const TArray<UOSESyncedAnimationDataAsset*>& animOptions, const AActor* sourceActor, const AActor* targetActor)
{
   if (targetActor == nullptr)
   {
      return nullptr;
   }

   FSyncedAnimationSearchParameters searchParams;
   searchParams.Source = sourceActor;
   searchParams.Target = targetActor;

   for (const UOSESyncedAnimationDataAsset* animData : animOptions)
   {
      if (animData && animData->SyncedAnimationData.MeetsConstraints(searchParams, true))
      {
         return animData;
      }
   }
   return nullptr;
}

