// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSESchedulerTaskSet.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESchedulerTaskSet)

void FOSESchedulerTaskSetTickFunction::ExecuteTick(
   const float deltaTime,
   ELevelTick tickType,
   ENamedThreads::Type currentThread,
   const FGraphEventRef& myCompletionGraphEvent)
{
   if(Target->_World.IsValid())
   {
      Target->ExecuteTick( Target->_World->GetTimeSeconds(), deltaTime);
   }
}

FString FOSESchedulerTaskSetTickFunction::DiagnosticMessage()
{
   return TEXT("FOSESchedulerTaskSetTickFunction::Tick");
}

FName FOSESchedulerTaskSetTickFunction::DiagnosticContext(bool bDetailed)
{
   return FName("FOSESchedulerTaskSetTickFunction");
}

FOSESchedulerTaskSet::FOSESchedulerTaskSet(const ETickingGroup tickingGroup, const UWorld* world)
{
   _TickFunction.TickGroup = tickingGroup;
   _TickFunction.Target = this;
   _TickFunction.RegisterTickFunction(world->PersistentLevel);
   _World = world;
}

FOSESchedulerTaskSet::~FOSESchedulerTaskSet()
{
   _TickFunction.UnRegisterTickFunction();
}


void FOSESchedulerTaskSet::ExecuteTick(const float worldTime, const float deltaTime)
{
   for (auto it = _Components.CreateIterator(); it; ++it)
   {
      auto component = *it;
      if(component.IsValid() == false)
      {
         it.RemoveCurrent();
         continue;
      }
      TryExecuteTickOnComponent(component.Get(), deltaTime);
   }
}

void FOSESchedulerTaskSet::TryExecuteTickOnComponent(UActorComponent* component, const float deltaTime)
{
   if(component->IsRegistered())
   {
      component->TickComponent(deltaTime, LEVELTICK_All, nullptr);
   }
}

FOSESchedulerTaskSet_MaxFrameTimeTick::FOSESchedulerTaskSet_MaxFrameTimeTick(
   const ETickingGroup tickingGroup,
   const UWorld* world,
   const float maxFrameTime):
   FOSESchedulerTaskSet(tickingGroup, world)
{
   MaxFrameTime = maxFrameTime;
}

void FOSESchedulerTaskSet_MaxFrameTimeTick::ExecuteTick(const float worldTime, const float deltaTime)
{
   // Calculate the start + end time based off the max frame time.
   const double startTime = FPlatformTime::Seconds();
   const double endTime = startTime + MaxFrameTime;
   // Grab the last ticked index, so that we don't always start from 0 and never tick the later tasks.
   int startTickIndex = IndexToTickIfBreaking;
   int numberOfTasks = _Components.Num();
   // Loop back to zero if the starting index is over or equal to the number of tasks
   if(startTickIndex >= numberOfTasks)
   {
      startTickIndex = 0;
   }
      
   int index = startTickIndex;
   for (int i = 0; i < numberOfTasks; ++i)
   {
      auto component = _Components[index];
      if(component.IsValid() == false)
      {
         continue;
      }
      TryExecuteTickOnComponent(component.Get(), deltaTime);
      // The work function _might_ cause the task count to go down.
      numberOfTasks = _Components.Num();
      index = index +1;
         
      // if the index goes off the end of the number of tasks, reset back to zero. 
      if (index >= numberOfTasks)
      {
         index = 0;
      }         
      // Stash off the last ticked index into the set.
      IndexToTickIfBreaking = index;
      if(index == startTickIndex)
      {
         // in-case of a fast frame, exit out if we've looped around to our starting
         // index as theres no point evaluating twice per frame
         break;
      }
      if(endTime < FPlatformTime::Seconds())
      {
         // the platform time is now past the end time, break out.
         break;
      }
   }
}

FOSESchedulerTaskSet_DelayedComponentTick::FOSESchedulerTaskSet_DelayedComponentTick(
   const ETickingGroup tickingGroup,
   const UWorld* world,
   const float timeBetweenIndividualComponentTicks) : FOSESchedulerTaskSet(tickingGroup, world)
{
   TimeBetweenIndividualComponentTicks = timeBetweenIndividualComponentTicks;
}

void FOSESchedulerTaskSet_DelayedComponentTick::ExecuteTick(const float worldTime, const float deltaTime)
{
   for (auto it = _Components.CreateIterator(); it; ++it)
   {
      auto component = *it;
      if(component.IsValid() == false)
      {
         MapOfComponentsToTimeLastTicked.Remove(*it);
         it.RemoveCurrent();
         continue;
      }
      const float timeLastTicked = MapOfComponentsToTimeLastTicked.FindOrAdd(component, 0.f);
      if(timeLastTicked < worldTime)
      {
         MapOfComponentsToTimeLastTicked[component] = timeLastTicked + TimeBetweenIndividualComponentTicks;
         TryExecuteTickOnComponent(component.Get(), deltaTime);
      }
   }
}

void FOSESchedulerTaskSet_DelayedComponentTick::AddComponent(TWeakObjectPtr<UActorComponent> component)
{
   MapOfComponentsToTimeLastTicked.Add(component, JitterOffsetForPreviouslyAddedComponent);
   JitterOffsetForPreviouslyAddedComponent += TimeBetweenIndividualComponentTicks;
   FOSESchedulerTaskSet::AddComponent(component);
}
