// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/TATUtilityAIBehaviorComponent.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue4
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "Engine/LevelBounds.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUtilityAIBehaviorComponent)

namespace UtilityGoalTargetsCVars
{
   static float ActorTargetQueriesRandomDeviation = 0.2f;
   FAutoConsoleVariableRef CVarEQSTargetQueriesRandomDeviation(
      TEXT("TAT.Goals.ActorTargetQueriesRandomDeviation"),
      ActorTargetQueriesRandomDeviation,
      TEXT("Random deviation for Actor Target Queries"),
      ECVF_Default);
}

UTATUtilityAIBehaviorComponent::UTATUtilityAIBehaviorComponent()
   : Super()
{
}

void UTATUtilityAIBehaviorComponent::BeginPlay()
{
   Super::BeginPlay();

   _tatAIController = Cast<ATATAIController>(_aiController);
   _potentialTargetsCache.SetWorld(GetWorld());

   GetWorld()->GetTimerManager().SetTimer(_smartObjectQueryTimerHandle, this, &ThisClass::_OnRunSmartObjectQuery, _GenerateSmartObjectQueryFrequency(), true);
   _CacheAlwaysKnownSmartObjects();
}

void UTATUtilityAIBehaviorComponent::AddAllowedEnemyActor(AActor* actor)
{
   _potentialTargetsCache.AddAllowedEnemyActor(actor, _tatAIController);
}

void UTATUtilityAIBehaviorComponent::RemoveAllowedEnemyActor(AActor* actor)
{
   _potentialTargetsCache.RemoveAllowedEnemyActor(actor);
}


void UTATUtilityAIBehaviorComponent::CallAndRegisterInjectionBehaviorChanged(
   FGameplayTag gameplayTag,
   FOnInjectedBehaviorsChanged::FDelegate&& delegateToBind)
{
   delegateToBind.Execute(this);
   FOnInjectedBehaviorsChanged& injectionDelegate = InjectionDelegateMap.FindOrAdd(gameplayTag);
   injectionDelegate.Add(MoveTemp(delegateToBind));
}

void UTATUtilityAIBehaviorComponent::AddInjectedBehaviors(
   const FGameplayTag& gameplayTag,
   const TArray<UUtilityBehaviorSet*>& behaviorsToAdd)
{
   // There may be situations where we have multiple sources of injected behaviors for the same tag.
   // In that case, we would need to extend the implementation to return some sort of handle which can be used to track
   // the specific entry.

   // To keep things simple, I'm going with the assumption that we will only inject one set of behaviors per escalation system
   // Which is the only user of this functionality.
   FInjectedBehaviorEntry entry;
   entry.InjectionTag = gameplayTag;
   entry.Behaviors = behaviorsToAdd;
   InjectedBehaviorEntries.Add(entry);
   if(const FOnInjectedBehaviorsChanged* delegate = InjectionDelegateMap.Find(gameplayTag))
   {
      delegate->Broadcast(this);
   }
}

void UTATUtilityAIBehaviorComponent::RemoveInjectedBehaviors(const FGameplayTag& gameplayTag)
{
   for(auto it = InjectedBehaviorEntries.CreateIterator(); it; ++it)
   {
      const FInjectedBehaviorEntry& entry = *it;
      if(entry.InjectionTag == gameplayTag)
      {
         it.RemoveCurrent();
         return;
      }
   }
}

void UTATUtilityAIBehaviorComponent::GetInjectedBehaviors(const FGameplayTag& gameplayTag, TArray<UUtilityBehaviorSet*>& outBehaviors) const
{
   for (const FInjectedBehaviorEntry& behaviorEntry : InjectedBehaviorEntries)
   {
      if(behaviorEntry.InjectionTag == gameplayTag)
      {
         outBehaviors = behaviorEntry.Behaviors;
      }
   }
}

bool UTATUtilityAIBehaviorComponent::_HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   return _potentialTargetsCache.HasCachedPotentialTargetsFor(targeting, targetingGroup);
}

void UTATUtilityAIBehaviorComponent::_CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup)
{
   _potentialTargetsCache.CachePotentialTargetsFor(targeting, targetingGroup, _tatAIController);
}

const TArray<FUtilityStateTarget>& UTATUtilityAIBehaviorComponent::_GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const
{
   return _potentialTargetsCache.GetPotentialTargetsFor(targeting, targetingGroup);
}

void UTATUtilityAIBehaviorComponent::_ResetPotentialTargetCache()
{
   _potentialTargetsCache.ResetPotentialTargetCache();
}

void UTATUtilityAIBehaviorComponent::_CacheAlwaysKnownSmartObjects()
{
   // TODO: Do we need to deal with any level streaming etc where this would not be reasonable to cache off at BeginPlay()?
   const USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
   const ULevel* persistentLevel = GetWorld()->PersistentLevel.Get();
   const TWeakObjectPtr<ALevelBounds>& levelBoundsActor = persistentLevel ? persistentLevel->LevelBoundsActor : nullptr;
   if (smartObjectSubsystem && levelBoundsActor.IsValid())
   {
      const FBox queryBox = levelBoundsActor->GetComponentsBoundingBox();

      const UTATAISettings& aiSettings = UTATAISettings::Get();

      FSmartObjectRequest request;
      request.QueryBox = queryBox;
      request.Filter.ActivityRequirements = aiSettings.AlwaysKnownSmartObjectsActivityQuery;

      _alwaysKnownSmartObjects.Reset();
      smartObjectSubsystem->FindSmartObjects(request, _alwaysKnownSmartObjects);
   }
}

void UTATUtilityAIBehaviorComponent::_OnRunSmartObjectQuery()
{
   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
   if (!smartObjectSubsystem)
      return;

   const APawn* pawn = _aiController->GetPawn();
   if (!pawn)
      return;

   // start w/ map-wide smart nodes with custom tagging
   TArray<FSmartObjectRequestResult> allResults;
   allResults.Reserve(_alwaysKnownSmartObjects.Num());
   for (FSmartObjectRequestResult& smartObjectRequestResult : _alwaysKnownSmartObjects)
   {
      if(smartObjectSubsystem->CanBeClaimed(smartObjectRequestResult.SlotHandle))
      {
         allResults.AddUnique(smartObjectRequestResult);
      }
   }

   // find nearby smart nodes
   {
      const FVector minBounds = pawn->GetActorLocation() - SmartObjectSearchBoxExtent;
      const FVector maxBounds = pawn->GetActorLocation() + SmartObjectSearchBoxExtent;
      const FBox queryBox = FBox(minBounds, maxBounds);
      FSmartObjectRequest request;
      request.QueryBox = queryBox;

      TArray<FSmartObjectRequestResult> nearbyResults;
      smartObjectSubsystem->FindSmartObjects(request, nearbyResults);
      
      // assume nearbyResults are unique from allResults (even though there may be some overlap) so we can do one reserve alloc here
      allResults.Reserve(allResults.Num() + nearbyResults.Num());
      for(const FSmartObjectRequestResult& nearbyResult : nearbyResults)
      {
         allResults.AddUnique(nearbyResult);
      }
   }

   //Add the current state target back in in-order to be reconsidered.
   //We do this because the claims system removes the object from the search system.
   //this is a work around in order to keep the current state target considered. This could potentially provide
   //issues with moving out of the query range which is a known risk as we work on a more robust solution.
   if (_currentStateTarget.TargetType == EBehaviorTargetType::SmartObjectRequest)
   {
      allResults.AddUnique(_currentStateTarget.SmartObjectRequestTarget);
   }

   _potentialTargetsCache.SetSmartObjects(MoveTemp(allResults));

}

float UTATUtilityAIBehaviorComponent::_GenerateSmartObjectQueryFrequency() const
{
   // jitter the time a bit each time we need it
   return FMath::FRandRange(FMath::Max(0.0f, SmartObjectQueryFrequency - UtilityGoalTargetsCVars::ActorTargetQueriesRandomDeviation), (SmartObjectQueryFrequency + UtilityGoalTargetsCVars::ActorTargetQueriesRandomDeviation));
}
