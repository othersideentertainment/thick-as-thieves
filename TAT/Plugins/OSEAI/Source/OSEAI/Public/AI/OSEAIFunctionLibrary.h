// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"
#include "AI/Utility/ConsiderationInput.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Engine/EngineTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSEAIFunctionLibrary.generated.h"

class AAIController;
class UUtilityAIBehavior;
class UBTNode;

////////////////////////////////////////////////////////////////////////////////////////////
///                    FOSEBlackboardKeyOrGameplayTag
////////////////////////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct OSEAI_API FOSEBlackboardKeyOrGameplayTag
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
   bool UseBlackboard = false;

   UPROPERTY(EditAnywhere, meta = (EditCondition = "!UseBlackboard", EditConditionHides))
   FGameplayTag GameplayTag;
   
   UPROPERTY(EditAnywhere, meta = (EditCondition = "UseBlackboard", EditConditionHides))
   FBlackboardKeySelector GameplayTagKey;

   bool ExportTextItem(FString& valueStr, FOSEBlackboardKeyOrGameplayTag const& defaultValue, UObject* parent, int32 portFlags, UObject* exportRootScope) const;
};

template<>
struct TStructOpsTypeTraits<FOSEBlackboardKeyOrGameplayTag> : public TStructOpsTypeTraitsBase2<FOSEBlackboardKeyOrGameplayTag>
{
   enum
   {
      WithExportTextItem = true
   };
};

////////////////////////////////////////////////////////////////////////////////////////////
///                    FOSEAINavMeshCalcPathResult
////////////////////////////////////////////////////////////////////////////////////////////

struct OSEAI_API FOSEAINavMeshCalcPathResult
{
   ENavigationQueryResult::Type QueryResult = ENavigationQueryResult::Error;
   double PathLength = 0.0;

   bool HasFullPath() const;
   bool HasPartialOrFullPath() const;
};

struct OSEAI_API FOSEAINavMeshCalcPathAndCostResult : public FOSEAINavMeshCalcPathResult
{
   double PathCost = 0.0;
};

////////////////////////////////////////////////////////////////////////////////////////////
///                    UOSEAIFunctionLibrary
////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class OSEAI_API UOSEAIFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   static bool IsAggressiveState(UUtilityAIStateBase* state);
   
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   static bool IsDefenselessState(UUtilityAIStateBase* state);

   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   static bool IsTransitionalState(UUtilityAIStateBase* state);

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   static bool HasPathToLocation(const AAIController* actor, const FVector targetLocation, const bool allowPartialPaths);

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   static bool IsStimValid(const FStimInfo& stimInfo);

   /// Report a touch event.
   ///
   /// @param Instigator Actor that triggered the touch.
   /// @param OtherActor Actor that got touched.
   UFUNCTION(BlueprintCallable, Category = "AI|Perception", meta = (WorldContext = "WorldContextObject"))
   static void ReportTouchEvent(UObject* worldContextObject, AActor* instigator, AActor* otherActor, const FVector& location);

   /// A utility to do a sight line trace in the way AISense_Sight would.
   ///
   /// Allows passing in a location to trace to instead of using AActor::GetActorLocation. Useful when doing multiple traces for one actor.
   static bool SightSenseLineTrace(FHitResult& outHitResult, const FVector& observerLocation, const AActor* target, const AActor* ignoreActor = nullptr, const FVector* targetLocation = nullptr);

   //////////////////////////////////////////////////
   /// Gameplay Tag Blackboard Utls
   //////////////////////////////////////////////////

   UFUNCTION(BlueprintCallable, Category = "AI|Components|Blackboard")
   static FGameplayTag GetValueAsGameplayTag(UBlackboardComponent* blackboardComponent, const FName& keyName);

   UFUNCTION(BlueprintCallable, Category = "AI|Components|Blackboard")
   static void SetValueAsGameplayTag(UBlackboardComponent* blackboardComponent, const FName& keyName, const FGameplayTag& gameplayTag);
   
   UFUNCTION(BlueprintPure, Category = "AI|BehaviorTree", Meta = (HidePin = "nodeOwner", DefaultToSelf = "nodeOwner"))
   static FGameplayTag GetBlackboardValueAsGameplayTag(UBTNode* nodeOwner, const FBlackboardKeySelector& key);

   UFUNCTION(BlueprintCallable, Category = "AI|BehaviorTree", Meta = (HidePin = "nodeOwner", DefaultToSelf = "nodeOwner"))
   static void SetBlackboardValueAsGameplayTag(UBTNode* nodeOwner, const FBlackboardKeySelector& key, FGameplayTag value);

   UFUNCTION(BlueprintCallable, Category = "AI|Components|Blackboard")
   static FGameplayTag ExtractGameplayTag(const FOSEBlackboardKeyOrGameplayTag& blackboardKeyOrTag, UBlackboardComponent* blackboardComponent);

   //////////////////////////////////////////////////
   /// UtilityStateTarget Blackboard Utls
   //////////////////////////////////////////////////

   UFUNCTION(BlueprintCallable, Category = "AI|Components|Blackboard")
   static FUtilityStateTarget GetValueAsUtilityStateTarget(UBlackboardComponent* blackboardComponent, const FName& keyName);

   UFUNCTION(BlueprintCallable, Category = "AI|Components|Blackboard")
   static void SetValueAsUtilityStateTarget(UBlackboardComponent* blackboardComponent, const FName& keyName, const FUtilityStateTarget& target);
   
   UFUNCTION(BlueprintPure, Category = "AI|BehaviorTree", Meta = (HidePin = "nodeOwner", DefaultToSelf = "nodeOwner"))
   static FUtilityStateTarget GetBlackboardValueAsUtilityStateTarget(UBTNode* nodeOwner, const FBlackboardKeySelector& key);

   UFUNCTION(BlueprintCallable, Category = "AI|BehaviorTree", Meta = (HidePin = "nodeOwner", DefaultToSelf = "nodeOwner"))
   static void SetBlackboardValueAsUtilityStateTarget(UBTNode* nodeOwner, const FBlackboardKeySelector& key, const FUtilityStateTarget& value);

   //////////////////////////////////////////////////
   /// Nav Mesh Utl
   //////////////////////////////////////////////////

   static FOSEAINavMeshCalcPathAndCostResult CalcNavMeshPathLengthAndCostFromCurrentLocation(const AActor* actor,
                                                                                             const FVector& targetLocation);

   static FOSEAINavMeshCalcPathResult CalcNavMeshPathLengthFromCurrentLocation(const AActor* actor,
                                                                               const FVector& targetLocation);

   static ENavigationQueryResult::Type CalcNavMeshPathLengthAndCost(const UObject& worldQuerierContextObject,
                                                                    const FVector& startLocation,
                                                                    const FVector& targetLocation,
                                                                    const TSubclassOf<UNavigationQueryFilter>& navigationFilterClass,
                                                                    const FNavAgentProperties& navAgentProperties,
                                                                    double& outPathLength,
                                                                    double& outPathCost);

   static void SortAndFilterByNavMeshDistanceToTarget(TArray<AActor*>& actors, const FVector& targetLocation, bool allowPartialPaths);
};
