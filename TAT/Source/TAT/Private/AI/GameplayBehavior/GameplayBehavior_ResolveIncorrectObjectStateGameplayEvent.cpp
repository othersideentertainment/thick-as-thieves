// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/GameplayBehavior/GameplayBehavior_ResolveIncorrectObjectStateGameplayEvent.h"

// tat
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"

// ose
#include "AI/GameplayBehavior/GameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent.h"

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

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehavior_ResolveIncorrectObjectStateGameplayEvent)

//----------------------------------------------------------------------//
// UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent
//----------------------------------------------------------------------//

UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent::UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FGameplayTag UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent::_GetGameplayEventTag(AActor& inAvatar, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner) const
{
   // no Super:: call, we are overriding the tag we're using

   const UGameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent* btConfig = Cast<const UGameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent>(config);
   if (!btConfig)
      return FGameplayTag();

   if (smartObjectOwner && smartObjectOwner->Implements<UTATAIIncorrectObjectStateInterface>())
   {
      if (!ITATAIIncorrectObjectStateInterface::Execute_AuthorityIsObjectInCorrectState(smartObjectOwner, true))
      {
         return btConfig->GetCorrectStateGameplayEventTag();
      }
      else
      {
         return btConfig->GetIncorrectGameplayEventTag();
      }
   }

   return FGameplayTag();
}

