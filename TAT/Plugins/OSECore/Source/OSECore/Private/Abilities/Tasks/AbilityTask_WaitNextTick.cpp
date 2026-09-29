// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_WaitNextTick.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitNextTick)

UAbilityTask_WaitNextTick::UAbilityTask_WaitNextTick(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , _ticksToWait(0)
   , _ticksElapsed(0)
   , _timeElapsed(0)
   , _endAutomatically(true)
{
   bTickingTask = true;
}

void UAbilityTask_WaitNextTick::TickTask(float deltaTime)
{
   if (_ticksElapsed >= _ticksToWait)
   {
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         OnCompleted.Broadcast(_ticksElapsed, _timeElapsed);
      }
      
      if (_endAutomatically)
      {
         EndTask();
      }
   }
   else
   {
      _ticksElapsed++;
      _timeElapsed += deltaTime;
   }
}

UAbilityTask_WaitNextTick* UAbilityTask_WaitNextTick::CreateWaitNextTickCount(UGameplayAbility* owningAbility, int32 numTicksToWait)
{
   UAbilityTask_WaitNextTick* task = NewAbilityTask<UAbilityTask_WaitNextTick>(owningAbility);

   task->_ticksToWait = numTicksToWait;
   task->_ticksElapsed = 0;
   task->_timeElapsed = 0;

   return task;
}

UAbilityTask_WaitNextTick* UAbilityTask_WaitNextTick::CreateWaitNextTick(UGameplayAbility* owningAbility)
{
   return CreateWaitNextTickCount(owningAbility, 1);
}

UAbilityTask_WaitNextTick* UAbilityTask_WaitNextTick::CreateTickForeverTask(UGameplayAbility* owningAbility)
{
   UAbilityTask_WaitNextTick* task = NewAbilityTask<UAbilityTask_WaitNextTick>(owningAbility);

   task->_ticksToWait = 1;
   task->_ticksElapsed = 0;
   task->_timeElapsed = 0;
   task->_endAutomatically = false;

   return task;
}

