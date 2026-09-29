// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/GameplayBehavior/GameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent.h"

// tat
#include "AI/GameplayBehavior/GameplayBehavior_ResolveIncorrectObjectStateGameplayEvent.h"

// ose
#include "AI/GameplayBehavior/GameplayBehavior_GameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent)

// ue

UGameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent::UGameplayBehaviorConfig_ResolveIncorrectObjectStateGameplayEvent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   BehaviorClass = UGameplayBehavior_ResolveIncorrectObjectStateGameplayEvent::StaticClass();
}

