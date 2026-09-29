// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSESchedulerWorldSubsystem.h"

#include "OSESchedulerTaskSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESchedulerWorldSubsystem)

DECLARE_CYCLE_STAT(TEXT("OSE Scheduler: Tick"), STAT_OSESchedulerWorldSubsystem_Tick, STATGROUP_AI);

void UOSESchedulerWorldSubsystem::CreateOrAddScheduledTask(
   const FName scheduleGroup,
   UActorComponent& component,
   const TFunctionRef<TSharedRef<FOSESchedulerTaskSet> ()>& createTaskSet)
{
   const TSharedRef<FOSESchedulerTaskSet>* taskSet = _TaskSets.Find(scheduleGroup);
   if(taskSet == nullptr)
   {
      const TSharedRef<FOSESchedulerTaskSet> createdTaskSet = createTaskSet();
      _TaskSets.Add(scheduleGroup, createdTaskSet);
      taskSet = &createdTaskSet;
   }
   FOSESchedulerTaskSet& taskSetRef = taskSet->Get();
   taskSetRef.AddComponent(&component);
}

void UOSESchedulerWorldSubsystem::RemoveScheduledTask(const FName scheduleGroup, UActorComponent& component)
{
   const TSharedRef<FOSESchedulerTaskSet>* taskSet = _TaskSets.Find(scheduleGroup);
   if(taskSet != nullptr)
   {
      FOSESchedulerTaskSet& dereferencedTaskSet = taskSet->Get();
      dereferencedTaskSet._Components.Remove(&component);
      if(dereferencedTaskSet._Components.Num() == 0)
      {
         _TaskSets.Remove(scheduleGroup);
      }
   }
}

void UOSESchedulerWorldSubsystem::Deinitialize()
{
   _TaskSets.Empty();
   Super::Deinitialize();
}
