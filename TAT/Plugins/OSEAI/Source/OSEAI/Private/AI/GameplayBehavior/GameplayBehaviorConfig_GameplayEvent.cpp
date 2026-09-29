// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/GameplayBehavior/GameplayBehaviorConfig_GameplayEvent.h"

// ose
#include "AI/GameplayBehavior/GameplayBehavior_GameplayEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehaviorConfig_GameplayEvent)

// ue

UGameplayBehaviorConfig_GameplayEvent::UGameplayBehaviorConfig_GameplayEvent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   BehaviorClass = UGameplayBehavior_GameplayEvent::StaticClass();
}

