// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/OSEAIFunctionLibrary.h"

// ose
#include "OSECommon.h"
#include "AI/OSEAISettings.h"
#include "AI/BehaviorTree/Blackboard/BlackboardKeyType_SingleGameplayTag.h"
#include "AI/BehaviorTree/Blackboard/BlackboardKeyType_UtilityStateTarget.h"
#include "AI/Utility/UtilityAIBehavior.h"

// ue4
#include "AIController.h"
#include "AISystem.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTFunctionLibrary.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Touch.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAIFunctionLibrary)

#if WITH_RECAST
#include "NavMesh/RecastNavMesh.h"
#endif // WITH_RECAST

////////////////////////////////////////////////////////////////////////////////////////////
///                    FOSEBlackboardKeyOrGameplayTag
////////////////////////////////////////////////////////////////////////////////////////////

bool FOSEBlackboardKeyOrGameplayTag::ExportTextItem(FString& valueStr, FOSEBlackboardKeyOrGameplayTag const& defaultValue, UObject* parent, int32 portFlags, UObject* exportRootScope) const
{
   // we only care about writing text to the property window (particularly in the behavior tree view)
   if (portFlags & EPropertyPortFlags::PPF_PropertyWindow)
   {
      if (UseBlackboard)
      {
         valueStr += FString::Printf(TEXT("GameplayTag Blackboard Key: %s"), *GameplayTagKey.SelectedKeyName.ToString());
      }
      else
      {
         valueStr += FString::Printf(TEXT("Gameplay Tag: %s"), *GameplayTag.ToString());
      }
      return true;
   }
   return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
///                    FOSEAINavMeshCalcPathResult
////////////////////////////////////////////////////////////////////////////////////////////

bool FOSEAINavMeshCalcPathResult::HasFullPath() const
{
   // do we have a full path? (success)
   return (QueryResult == ENavigationQueryResult::Success);
}

bool FOSEAINavMeshCalcPathResult::HasPartialOrFullPath() const
{
   // do we have a full path? (success)
   // or do we have a partial path? (fail + a non-zero path length)
   return (HasFullPath() || (QueryResult == ENavigationQueryResult::Fail && PathLength > 0.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////
///                    UOSEAIFunctionLibrary
////////////////////////////////////////////////////////////////////////////////////////////

bool UOSEAIFunctionLibrary::IsAggressiveState(UUtilityAIStateBase* state)
{
   if (state)
   {
      const UOSEAISettings& settings = UOSEAISettings::Get();
      return settings.AggressiveBehaviorTag.MatchesAny(state->GetStateTags());
   }
   return false;
}

bool UOSEAIFunctionLibrary::IsDefenselessState(UUtilityAIStateBase* state)
{
   if (state)
   {
      const UOSEAISettings& settings = UOSEAISettings::Get();
      return settings.DefenselessBehaviorTag.MatchesAny(state->GetStateTags());
   }
   return false;
}

bool UOSEAIFunctionLibrary::IsTransitionalState(UUtilityAIStateBase* state)
{
   if (state)
   {
      const UOSEAISettings& settings = UOSEAISettings::Get();
      return settings.TransitionalStateTag.MatchesAny(state->GetStateTags());
   }
   return false;
}

// Note: We could convert this to use `FindPathAsync` if this becomes a bottleneck.
bool UOSEAIFunctionLibrary::HasPathToLocation(const AAIController* aiController, const FVector targetLocation, const bool allowPartialPaths)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_OSEAIFunctionLibrary_HasPathToLocation);

   bool hasPath = false;

   if (!aiController->GetPawn())
      return hasPath;

   const FVector myLocation = aiController->GetNavAgentLocation();

   const UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(aiController->GetWorld());
   const ANavigationData* navData = aiController ? navSys->GetNavDataForProps(aiController->GetNavAgentPropertiesRef(), aiController->GetNavAgentLocation()) : nullptr;
   if (navData)
   {
      FSharedConstNavQueryFilter navFilter = UNavigationQueryFilter::GetQueryFilter(*navData, aiController, aiController->GetDefaultNavigationFilterClass());
      FPathFindingQuery pathQuery = FPathFindingQuery(aiController, *navData, myLocation, targetLocation, navFilter);
      pathQuery.SetAllowPartialPaths(allowPartialPaths);

      // Hierarchical is faster, but it doesn't seem to account for conditions on nav-links, so use Regular when you desire a full path.
      EPathFindingMode::Type pathFindingMode = allowPartialPaths ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular;

      hasPath = navSys->TestPathSync(pathQuery, pathFindingMode);
   }
   return hasPath;
}

bool UOSEAIFunctionLibrary::IsStimValid(const FStimInfo& stimInfo)
{
   return stimInfo.IsValid();
}

bool UOSEAIFunctionLibrary::SightSenseLineTrace(FHitResult& outHitResult, const FVector& observerLocation, const AActor* target, const AActor* ignoreActor, const FVector* targetLocation)
{
   if (!target)
   {
      outHitResult.Reset();
      return false;
   }

   const FVector traceEnd = targetLocation ? *targetLocation : target->GetActorLocation();

   constexpr bool bTraceComplex = false;
   ECollisionChannel defaultSightCollisionChannel = GET_AI_CONFIG_VAR(DefaultSightCollisionChannel);
   const FCollisionQueryParams queryParams(SCENE_QUERY_STAT(AILineOfSight), bTraceComplex, ignoreActor);
   const bool hitSuccess = target->GetWorld()->LineTraceSingleByChannel(
      outHitResult,
      observerLocation,
      traceEnd,
      defaultSightCollisionChannel,
      queryParams
   );
   
   const AActor* hitResultActor = outHitResult.GetActor();
   // If the line trace didn't hit anything, we can be seen at the given location.
   return !hitSuccess || hitResultActor == target;
}

void UOSEAIFunctionLibrary::ReportTouchEvent(UObject* worldContextObject, AActor* instigator, AActor* otherActor, const FVector& location)
{
   UAIPerceptionSystem* perceptionSystem = UAIPerceptionSystem::GetCurrent(worldContextObject);
   if (perceptionSystem)
   {
      FAITouchEvent touchEvent(otherActor, instigator, location);
      perceptionSystem->OnEvent(touchEvent);
   }
}

//////////////////////////////////////////////////
/// Gameplay Tag Blackboard Utls
//////////////////////////////////////////////////

FGameplayTag UOSEAIFunctionLibrary::GetValueAsGameplayTag(UBlackboardComponent* blackboardComponent, const FName& keyName)
{
   if (blackboardComponent)
      return blackboardComponent->GetValue<UBlackboardKeyType_SingleGameplayTag>(keyName);
   return FGameplayTag::EmptyTag;
}

void UOSEAIFunctionLibrary::SetValueAsGameplayTag(UBlackboardComponent* blackboardComponent, const FName& keyName, const FGameplayTag& gameplayTag)
{
   if (blackboardComponent)
   {
      const FBlackboard::FKey keyID = blackboardComponent->GetKeyID(keyName);
      blackboardComponent->SetValue<UBlackboardKeyType_SingleGameplayTag>(keyID, gameplayTag);
   }
}

FGameplayTag UOSEAIFunctionLibrary::GetBlackboardValueAsGameplayTag(UBTNode* nodeOwner, const FBlackboardKeySelector& key)
{
   UBlackboardComponent* blackboardComp = UBTFunctionLibrary::GetOwnersBlackboard(nodeOwner);
   return blackboardComp ? blackboardComp->GetValue<UBlackboardKeyType_SingleGameplayTag>(key.SelectedKeyName) : FGameplayTag::EmptyTag;
}

void UOSEAIFunctionLibrary::SetBlackboardValueAsGameplayTag(UBTNode* nodeOwner, const FBlackboardKeySelector& key, FGameplayTag value)
{
   if (UBlackboardComponent* blackboardComp = UBTFunctionLibrary::GetOwnersBlackboard(nodeOwner))
   {
      blackboardComp->SetValue<UBlackboardKeyType_SingleGameplayTag>(key.SelectedKeyName, value);
   }
}

FGameplayTag UOSEAIFunctionLibrary::ExtractGameplayTag(const FOSEBlackboardKeyOrGameplayTag& blackboardKeyOrTag, UBlackboardComponent* blackboardComponent)
{
   // even though we dont need this for the raw gameplay tag version we might as well be consistent here when we're not passed a bb comp...?
   if (!blackboardComponent)
      return FGameplayTag::EmptyTag;

   if (blackboardKeyOrTag.UseBlackboard)
   {
      return GetValueAsGameplayTag(blackboardComponent, blackboardKeyOrTag.GameplayTagKey.SelectedKeyName);
   }
   else
   {
      return blackboardKeyOrTag.GameplayTag;
   }
}

//////////////////////////////////////////////////
/// UtilityStateTarget Blackboard Utls
//////////////////////////////////////////////////

FUtilityStateTarget UOSEAIFunctionLibrary::GetValueAsUtilityStateTarget(UBlackboardComponent* blackboardComponent, const FName& keyName)
{
   if (blackboardComponent)
      return blackboardComponent->GetValue<UBlackboardKeyType_UtilityStateTarget>(keyName);
   return FUtilityStateTarget::Invalid;
}

void UOSEAIFunctionLibrary::SetValueAsUtilityStateTarget(UBlackboardComponent* blackboardComponent, const FName& keyName, const FUtilityStateTarget& target)
{
   if (blackboardComponent)
   {
      const FBlackboard::FKey keyID = blackboardComponent->GetKeyID(keyName);
      blackboardComponent->SetValue<UBlackboardKeyType_UtilityStateTarget>(keyID, target);
   }
}

FUtilityStateTarget UOSEAIFunctionLibrary::GetBlackboardValueAsUtilityStateTarget(UBTNode* nodeOwner, const FBlackboardKeySelector& key)
{
   UBlackboardComponent* blackboardComp = UBTFunctionLibrary::GetOwnersBlackboard(nodeOwner);
   return blackboardComp ? blackboardComp->GetValue<UBlackboardKeyType_UtilityStateTarget>(key.SelectedKeyName) : FUtilityStateTarget::Invalid;
}

void UOSEAIFunctionLibrary::SetBlackboardValueAsUtilityStateTarget(UBTNode* nodeOwner, const FBlackboardKeySelector& key, const FUtilityStateTarget& value)
{
   if (UBlackboardComponent* blackboardComp = UBTFunctionLibrary::GetOwnersBlackboard(nodeOwner))
   {
      blackboardComp->SetValue<UBlackboardKeyType_UtilityStateTarget>(key.SelectedKeyName, value);
   }
}

FOSEAINavMeshCalcPathAndCostResult UOSEAIFunctionLibrary::CalcNavMeshPathLengthAndCostFromCurrentLocation(const AActor* actor, const FVector& targetLocation)
{
   FOSEAINavMeshCalcPathAndCostResult result;
   result.QueryResult = ENavigationQueryResult::Error;
   if (const AAIController* controller = UOSECommon::GetController<AAIController>(actor))
   {
      result.QueryResult = CalcNavMeshPathLengthAndCost(*controller,
                                                   controller->GetNavAgentLocation(),
                                                   targetLocation,
                                                   controller->GetDefaultNavigationFilterClass(),
                                                   controller->GetNavAgentPropertiesRef(),
                                                   result.PathLength,
                                                   result.PathCost);
   }
   return result;
}

FOSEAINavMeshCalcPathResult UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(const AActor* actor, const FVector& targetLocation)
{
   FOSEAINavMeshCalcPathResult result;
   result.QueryResult = ENavigationQueryResult::Error;
   if (const AAIController* controller = UOSECommon::GetController<AAIController>(actor))
   {
      double unusedOutPathCost = 0.0;
      result.QueryResult = CalcNavMeshPathLengthAndCost(*controller,
                                                   controller->GetNavAgentLocation(),
                                                   targetLocation,
                                                   controller->GetDefaultNavigationFilterClass(),
                                                   controller->GetNavAgentPropertiesRef(),
                                                   result.PathLength,
                                                   unusedOutPathCost);
   }
   return result;
}

ENavigationQueryResult::Type UOSEAIFunctionLibrary::CalcNavMeshPathLengthAndCost(const UObject& worldQuerierContextObject, const FVector& startLocation, const FVector& targetLocation, const TSubclassOf<UNavigationQueryFilter>& navigationFilterClass, const FNavAgentProperties& navAgentProperties, double& outPathLength, double& outPathCost)
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_OSEAIFunctionLibrary_CalcNavMeshPathLengthAndCost);

   outPathLength = 0.0f;
   outPathCost = 0.0f;
   
   // don't check invalid locations
   if (targetLocation == FAISystem::InvalidLocation)
      return ENavigationQueryResult::Error;

   UWorld* world = worldQuerierContextObject.GetWorld();
   check(world);
   const UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
   const ANavigationData* navData = navSys->GetNavDataForProps(navAgentProperties, startLocation);
#if WITH_RECAST
   const ARecastNavMesh* recastNavMesh = Cast<ARecastNavMesh>(navData);
   if (navData)
   {
      FSharedConstNavQueryFilter navFilter = UNavigationQueryFilter::GetQueryFilter(*navData, &worldQuerierContextObject, navigationFilterClass);
      return recastNavMesh->CalcPathLengthAndCost(startLocation, targetLocation, outPathLength, outPathCost, navFilter);
   }
#else
#error NEED SUPPORT FOR NON-RECAST PATHFINDING HERE
#endif // WITH_RECAST
   return ENavigationQueryResult::Error;
}

void UOSEAIFunctionLibrary::SortAndFilterByNavMeshDistanceToTarget(TArray<AActor*>& actors, const FVector& targetLocation, bool allowPartialPaths)
{
   // TODO: this might end up being too much of a perf hit
   QUICK_SCOPE_CYCLE_COUNTER(STAT_OSEAIFunctionLibrary_SortAndFilterByNavMeshDistanceToTarget);

   // prevent doing a navmesh path calculation more than once per actor
   TMap<const AActor*, float> cachedDistanceMap;
   cachedDistanceMap.Reserve(actors.Num());

   for(AActor* actor : actors)
   {
      if (!IsValid(actor))
         continue;

      FOSEAINavMeshCalcPathResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(actor, targetLocation);
      const bool success = allowPartialPaths ? result.HasPartialOrFullPath() : result.HasFullPath();
      if (success)
      {
         cachedDistanceMap.Add(actor, result.PathLength);
      }
      else
      {
         cachedDistanceMap.Add(actor, FLT_MAX);
      }
   }

   // filter
   actors.RemoveAll([&cachedDistanceMap](const AActor* actor)
   {
      return !IsValid(actor) || cachedDistanceMap.FindChecked(actor) == FLT_MAX;
   });

   // sort
   actors.Sort([&cachedDistanceMap](const AActor& lhs, const AActor& rhs)
   {
      float lhsPathLength = cachedDistanceMap.FindChecked(&lhs);
      float rhsPathLength = cachedDistanceMap.FindChecked(&rhs);
      return lhsPathLength < rhsPathLength;      
   });
}

