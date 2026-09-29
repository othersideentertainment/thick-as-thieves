// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/GameplayBehavior/GameplayBehavior_GameplayEvent.h"

// ose
#include "AI/GameplayBehavior/GameplayBehaviorConfig_GameplayEvent.h"

// ue
#include "AIController.h"
#include "TimerManager.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "VisualLogger/VisualLogger.h"
#include "GameplayTagAssetInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehavior_GameplayEvent)

//----------------------------------------------------------------------//
// UGameplayBehavior_GameplayEvent
//----------------------------------------------------------------------//

UGameplayBehavior_GameplayEvent::UGameplayBehavior_GameplayEvent(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{

}

bool UGameplayBehavior_GameplayEvent::Trigger(AActor& inAvatar, const UGameplayBehaviorConfig* config /* = nullptr*/, AActor* smartObjectOwner /* = nullptr*/)
{
   FGameplayTagContainer instigatorTags;
   if (const IGameplayTagAssetInterface* inAvatarTagInterface = Cast<IGameplayTagAssetInterface>(&inAvatar))
   {
      inAvatarTagInterface->GetOwnedGameplayTags(instigatorTags);
   }
   
   FGameplayTagContainer targetTags;
   if (const IGameplayTagAssetInterface* targetTagInterface = Cast<IGameplayTagAssetInterface>(smartObjectOwner))
   {
      targetTagInterface->GetOwnedGameplayTags(targetTags);
   }

   FGameplayEventData payload;
   payload.EventTag = _GetGameplayEventTag(inAvatar, config, smartObjectOwner);
   payload.Instigator = &inAvatar;
   payload.InstigatorTags = instigatorTags;
   payload.Target = smartObjectOwner;
   payload.TargetTags = targetTags;
   payload.OptionalObject = this; // send in a reference to ourselves so that handling abilities can work backwards to where the event came from

   UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&inAvatar, payload.EventTag, payload);

   // false means we're not still running, this was a synchronous trigger
   return false;
}

void UGameplayBehavior_GameplayEvent::EndBehavior(AActor& inAvatar, const bool interrupted)
{
   Super::EndBehavior(inAvatar, interrupted);
}

FGameplayTag UGameplayBehavior_GameplayEvent:: _GetGameplayEventTag(AActor& inAvatar, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner) const
{
   if (const UGameplayBehaviorConfig_GameplayEvent* btConfig = Cast<const UGameplayBehaviorConfig_GameplayEvent>(config))
   {
      return btConfig->GetGameplayEventTag();
   }
   return FGameplayTag();
}

