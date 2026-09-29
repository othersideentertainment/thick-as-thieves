// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/UtilityAIManagerWorldSubsystem.h"

// OSE
#include "AI/Utility/UtilityAIComponent.h"
#include "AI/Utility/UtilityAISettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIManagerWorldSubsystem)

DECLARE_CYCLE_STAT(TEXT("AI Utility System: Tick"), STAT_UtilityAIManagerWorldSubsystem_Tick, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("AI Utility System: Tick Goals"), STAT_UtilityAIManagerWorldSubsystem_Tick_Goals, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("AI Utility System: Tick Behaviors"), STAT_UtilityAIManagerWorldSubsystem_Tick_Behaviors, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("AI Goals Ticked Per Frame"), STAT_AI_TickedGoalsPerFrame, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("AI Behaviors Ticked Per Frame"), STAT_AI_TickedBehaviorsPerFrame, STATGROUP_AI);

void RegisterUtilityComponent(UUtilityAIComponent* component, TArray<UUtilityAIComponent*>& componentArray)
{
   check(!componentArray.Contains(component));
   componentArray.AddUnique(component);
}

void DeregisterUtilityComponent(UUtilityAIComponent* component, TArray<UUtilityAIComponent*>& componentArray, int& componentTickIndex)
{
   const int index = componentArray.IndexOfByKey(component);
   check(index != INDEX_NONE);
   
   componentArray.RemoveAt(index);
   
   if (componentTickIndex > index)
   {
      --componentTickIndex;
   }
   else if (componentTickIndex >= componentArray.Num())
   {
      componentTickIndex = 0;
   }
}

void UUtilityAIManagerWorldSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
   _utilityAISettings = UUtilityAISettings::Get();
}

void UUtilityAIManagerWorldSubsystem::RegisterAIGoalComponent(UUtilityAIComponent* component)
{
   RegisterUtilityComponent(component, _goalComponents);
}

void UUtilityAIManagerWorldSubsystem::DeregisterAIGoalComponent(UUtilityAIComponent* component)
{
   DeregisterUtilityComponent(component, _goalComponents, _nextGoalIndex);
}

void UUtilityAIManagerWorldSubsystem::RegisterAIBehaviorComponent(UUtilityAIComponent* component)
{
   RegisterUtilityComponent(component, _behaviorComponents);
}

void UUtilityAIManagerWorldSubsystem::DeregisterAIBehaviorComponent(UUtilityAIComponent* component)
{
   DeregisterUtilityComponent(component, _behaviorComponents, _nextBehaviorIndex);
}

int TickGroupOfComponents(const int componentsToTick, const double maxTimeForGroupOfComponentsToTick, const TArray<UUtilityAIComponent*>& components, int& index)
{
   if (componentsToTick == 0)
   {
      // early out if we have nothing to iterate
      return 0;
   }
   
   // Set the end time by grabbing the seconds from the platform and adding
   // the total amount of seconds we want to spend on this operation
   const double startTime = FPlatformTime::Seconds();
   const double endTime = startTime + maxTimeForGroupOfComponentsToTick;
   const int numberOfComponents = components.Num();
   int componentsTickedThisFrame = 0;   
   const int startTickIndex = index;
   for(int i = 0; i < componentsToTick; ++i)
   {
      components[index]->CheckForNewState();
      componentsTickedThisFrame++;
      if (++index >= numberOfComponents)
      {
         index = 0;
      }
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
   return componentsTickedThisFrame;
}

int GetNumberOfBehaviorsToTick(const int count, const float deltaTime, const float minTime)
{
   const int min = FMath::Min(count, 1);
   if(FMath::IsNearlyZero(minTime))
      return min;

      
   const int behaviorsToTick = FMath::Clamp(FMath::CeilToInt(count * (deltaTime / minTime)), min, count);
   return behaviorsToTick;
};

void UUtilityAIManagerWorldSubsystem::Tick(const float deltaTime)
{
   Super::Tick(deltaTime);
   if(_utilityAISettings == nullptr)
      return;
   SCOPE_CYCLE_COUNTER(STAT_UtilityAIManagerWorldSubsystem_Tick);
   constexpr float millisecondMultiplier = 0.001f;
   {
      SCOPE_CYCLE_COUNTER(STAT_UtilityAIManagerWorldSubsystem_Tick_Goals);
      const double maxTimeForUtilityManagerTickGoals = _utilityAISettings->MaxFrameBudgetForGoals * millisecondMultiplier;
      const int goalsToTick = GetNumberOfBehaviorsToTick(_goalComponents.Num(), deltaTime,
                                                         _utilityAISettings->MinTimeBetweenConsiderationEvaluations * millisecondMultiplier);
      const int numberOfGoalsTicked = TickGroupOfComponents(goalsToTick,
                                                            maxTimeForUtilityManagerTickGoals,
                                                            _goalComponents,
                                                            _nextGoalIndex);
      SET_DWORD_STAT(STAT_AI_TickedGoalsPerFrame, numberOfGoalsTicked);
   }
   {
      SCOPE_CYCLE_COUNTER(STAT_UtilityAIManagerWorldSubsystem_Tick_Behaviors);
      const double maxTimeForUtilityManagerTickBehaviors = _utilityAISettings->MaxFrameBudgetForBehaviors * millisecondMultiplier;
      const int behaviorsToTick = GetNumberOfBehaviorsToTick(_behaviorComponents.Num(), deltaTime,
                                                         _utilityAISettings->MinTimeBetweenConsiderationEvaluations * millisecondMultiplier);
      
      const int numberOfBehaviorsTicked = TickGroupOfComponents(behaviorsToTick, 
                                                                maxTimeForUtilityManagerTickBehaviors,
                                                                _behaviorComponents,
                                                                _nextBehaviorIndex);
      SET_DWORD_STAT(STAT_AI_TickedBehaviorsPerFrame, numberOfBehaviorsTicked);
   }
}

