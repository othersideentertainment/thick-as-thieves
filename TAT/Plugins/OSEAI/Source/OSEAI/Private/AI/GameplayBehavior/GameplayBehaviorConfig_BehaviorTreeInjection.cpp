// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/GameplayBehavior/GameplayBehaviorConfig_BehaviorTreeInjection.h"

// ose
#include "AI/GameplayBehavior/GameplayBehavior_BehaviorTreeInjection.h"

// ue
#include "BehaviorTree/BehaviorTree.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehaviorConfig_BehaviorTreeInjection)

UGameplayBehaviorConfig_BehaviorTreeInjection::UGameplayBehaviorConfig_BehaviorTreeInjection(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   BehaviorClass = UGameplayBehavior_BehaviorTreeInjection::StaticClass();
}

UBehaviorTree* UGameplayBehaviorConfig_BehaviorTreeInjection::GetBehaviorTree() const
{
   return _behaviorTree.IsPending() ? _behaviorTree.LoadSynchronous() : _behaviorTree.Get();
}

