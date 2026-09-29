// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_RepeatIndefinitely.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_RepeatIndefinitely)

UAbilityTask_RepeatIndefinitely* UAbilityTask_RepeatIndefinitely::RepeatActionIndefinitely(UGameplayAbility* owningAbility, float timeBetweenActions, bool jitterStartTime)
{
   UAbilityTask_RepeatIndefinitely* task = NewAbilityTask<UAbilityTask_RepeatIndefinitely>(owningAbility);
   task->_timeBetweenActions = timeBetweenActions;
   task->_jitterStartTime = jitterStartTime;
   return task;
}

void UAbilityTask_RepeatIndefinitely::Activate()
{
   Super::Activate();

   if (UWorld* world = GetWorld())
   {
      const float startDelay = _jitterStartTime ? FMath::RandRange(0.f, _timeBetweenActions) : -1;
      world->GetTimerManager().SetTimer(_timerHandle, this, &UAbilityTask_RepeatIndefinitely::_PerformAction, _timeBetweenActions, true, startDelay);
   }
}

void UAbilityTask_RepeatIndefinitely::_PerformAction()
{
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      OnPerformAction.Broadcast();
   }
}

void UAbilityTask_RepeatIndefinitely::OnDestroy(bool abilityIsEnding)
{
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().ClearTimer(_timerHandle);
   }

   Super::OnDestroy(abilityIsEnding);
}

