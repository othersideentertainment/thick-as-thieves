// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEGameplayAbility_SyncedAnimationPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbility_SyncedAnimationPlayer)

//////////////////////////////////////////////////////////////////////////
//                  UOSESyncedAnimationDataAsset
//////////////////////////////////////////////////////////////////////////

bool UOSESyncedAnimationDataAsset::MeetsConstraints(AActor* sourceCharacter, AActor* targetCharacter, bool requiresTarget) const
{
   FSyncedAnimationSearchParameters searchParams;
   searchParams.Source = sourceCharacter;
   searchParams.Target = targetCharacter;
   return SyncedAnimationData.MeetsConstraints(searchParams, requiresTarget);
}

//////////////////////////////////////////////////////////////////////////
//                  FOSESyncedAnimationsTargetDataFilter
//////////////////////////////////////////////////////////////////////////

bool FOSESyncedAnimationsTargetDataFilter::FilterPassesForActor(const AActor* actorToBeFiltered) const
{
   // requires super to pass, and at least one of the synced animations meets the req
   const bool superPasses = Super::FilterPassesForActor(actorToBeFiltered);
   bool syncedAnimationPasses = false;

   if (superPasses)
   {
      for (UOSESyncedAnimationDataAsset* syncedAnim : SyncedAnimations)
      {
         if (!syncedAnim)
            continue;

         if (syncedAnim->MeetsConstraints(SelfActor, const_cast<AActor*>(actorToBeFiltered), RequiresTarget))
         {
            syncedAnimationPasses = true;
            break;
         }
      }
   }

   return superPasses && syncedAnimationPasses;
}

//////////////////////////////////////////////////////////////////////////
///                  UOSESyncedAnimationsTargetDataFilterFunctionLibrary
//////////////////////////////////////////////////////////////////////////

FGameplayTargetDataFilterHandle UOSESyncedAnimationsTargetDataFilterFunctionLibrary::MakeSyncedAnimationTargetDataFilter(const FOSESyncedAnimationsTargetDataFilter& filter, AActor* filterActor)
{
   FGameplayTargetDataFilter* newFilter = new FOSESyncedAnimationsTargetDataFilter(filter);
   newFilter->InitializeFilterContext(filterActor);

   FGameplayTargetDataFilterHandle filterHandle;
   filterHandle.Filter = TSharedPtr<FGameplayTargetDataFilter>(newFilter);
   return filterHandle;
}

//////////////////////////////////////////////////////////////////////////
//                  UOSEGameplayAbility_SyncedAnimationPlayer
//////////////////////////////////////////////////////////////////////////

UOSEGameplayAbility_SyncedAnimationPlayer::UOSEGameplayAbility_SyncedAnimationPlayer()
   : Super()
{
}

bool UOSEGameplayAbility_SyncedAnimationPlayer::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* actorInfo, const FGameplayEventData* payload) const
{
   // we require synced animation data to respond to the event
   const UOSESyncedAnimationDataAsset* syncedAnimationData = payload ? Cast<UOSESyncedAnimationDataAsset>(payload->OptionalObject) : nullptr;
   if (!syncedAnimationData)
      return false;

   return Super::ShouldAbilityRespondToEvent(actorInfo, payload);
}

