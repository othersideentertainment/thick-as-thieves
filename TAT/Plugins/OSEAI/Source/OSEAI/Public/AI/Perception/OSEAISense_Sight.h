// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// unreal
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GenericTeamAgentInterface.h"
#include "OSEAITargetSightInterface.h"
#include "Perception/AISense.h"

#include "OSEAISense_Sight.generated.h"

class UOSEAISenseSharedConfigData;
class IAISightTargetInterface;
class UOSEAISense_Sight;
class UOSEAISenseConfig_Sight;
class AOSEAIController;
class IOSEAISightInterface;

namespace EOSESightPerceptionEventName
{
   enum Type
   {
      Undefined,
      GainedSight,
      LostSight
   };
}

USTRUCT()
struct OSEAI_API FOSEAISightEvent
{
   GENERATED_USTRUCT_BODY()

   typedef UOSEAISense_Sight FSenseClass;

   float Age;
   EOSESightPerceptionEventName::Type EventType;

   UPROPERTY()
   AActor* SeenActor;

   UPROPERTY()
   AActor* Observer;

   FOSEAISightEvent() : SeenActor(nullptr), Observer(nullptr) {}

   FOSEAISightEvent(AActor* inSeenActor, AActor* inObserver, EOSESightPerceptionEventName::Type inEventType)
      : Age(0.f), EventType(inEventType), SeenActor(inSeenActor), Observer(inObserver)
   {
   }
};

struct FOSEAISightTarget
{
   typedef uint32 FTargetId;
   static const FTargetId InvalidTargetId;

   TWeakObjectPtr<AActor> Target;
   
   IAISightTargetInterface* SightTargetInterface;

   FGenericTeamId TeamId;
   FTargetId TargetId;

   FOSEAISightTarget(AActor* inTarget = NULL, FGenericTeamId inTeamId = FGenericTeamId::NoTeam);

   FORCEINLINE FVector GetLocationSimple() const
   {
      const AActor* targetPtr = Target.Get();
      return targetPtr ? targetPtr->GetActorLocation() : FVector::ZeroVector;
   }

   FORCEINLINE const AActor* GetTargetActor() const { return Target.Get(); }
};

struct FOSEAISightQuery
{
   FPerceptionListenerID ObserverId;
   FOSEAISightTarget::FTargetId TargetId;

   float Score;
   float Importance;

   FVector LastSeenLocation;

   /** User data that can be used inside the IAISightTargetInterface::TestVisibilityFrom method to store a persistence state */
   mutable int32 UserData;

   uint64 bLastResult : 1;
   uint64 LastProcessedFrameNumber : 63;

   FOSEAISightQuery(FPerceptionListenerID listenerId = FPerceptionListenerID::InvalidID(),
                    FOSEAISightTarget::FTargetId target = FOSEAISightTarget::InvalidTargetId)
      : ObserverId(listenerId), TargetId(target), Score(0), Importance(0), LastSeenLocation(FAISystem::InvalidLocation),
        UserData(0), bLastResult(false), LastProcessedFrameNumber(GFrameCounter)
   {
   }

   float GetAge() const
   {
      return (float)(GFrameCounter - LastProcessedFrameNumber);
   }

   void RecalcScore()
   {
      Score = GetAge() + Importance;
   }

   void OnProcessed()
   {
      LastProcessedFrameNumber = GFrameCounter;
   }

   void ForgetPreviousResult()
   {
      LastSeenLocation = FAISystem::InvalidLocation;
      bLastResult = false;
   }

   class FSortPredicate
   {
   public:
      FSortPredicate()
      {}

      bool operator()(const FOSEAISightQuery& A, const FOSEAISightQuery& B) const
      {
         return A.Score > B.Score;
      }
   };
};

struct OSEAI_API FDigestedSightProperties
{
   float AutoSuccessRangeSqFromLastSeenLocation;
   uint8 AffiliationFlags;
   TWeakObjectPtr<UOSEAISenseSharedConfigData> SharedConfigData;
   const IOSEAISightInterface* SightInterface = nullptr; // only valid if listener is valid
      
   float SightRadiusSq;
   float LoseSightRadiusSq;
   float MinimumSightRadiusSq;
   float HalfFOVInDegrees;
   float PointOfViewBackwardOffset;
   float NearClip;
   float FarClip;
   float LoseSightFarClip;
   float MinSightFarClip;
   float FrustumPitch;
   float FrustumAspectRatio;
   // The range modifiers, cached on a single frame number
   mutable float CachedRangeModifiers = 0;
   mutable uint64 CachedRangeModifiersFrame = 0;
      
   FDigestedSightProperties();
};

USTRUCT()
struct FOSEAISightBucket
{
   GENERATED_BODY()
   
   TArray<FOSEAISightQuery> SightQueriesOutOfRange;
   TArray<FOSEAISightQuery> SightQueriesInRange;
   
   int NextOutOfRangeIndex;
   bool bSightQueriesOutOfRangeDirty = true;
};

UCLASS(ClassGroup = AI, config = Game)
class OSEAI_API UOSEAISense_Sight : public UAISense
{
   GENERATED_UCLASS_BODY()

public:
   typedef TMap<FOSEAISightTarget::FTargetId, FOSEAISightTarget> FTargetsContainer;
   FTargetsContainer ObservedTargets;
   TMap<FPerceptionListenerID, FDigestedSightProperties> DigestedProperties;

   typedef TMap<EAISightBucket, FOSEAISightBucket> FBucketsContainer;
   FBucketsContainer SightQueryBuckets;

protected:
   UPROPERTY(EditDefaultsOnly, Category = "AI Perception", config)
   int32 MaxTracesPerTick;

   UPROPERTY(EditDefaultsOnly, Category = "AI Perception", config)
   int32 MinQueriesPerTimeSliceCheck;

   UPROPERTY(EditDefaultsOnly, Category = "AI Perception", config)
   double MaxTimeSlicePerTick;

   UPROPERTY(EditDefaultsOnly, Category = "AI Perception", config)
   float HighImportanceQueryDistanceThreshold;

   float _highImportanceDistanceSquare;

   UPROPERTY(EditDefaultsOnly, Category = "AI Perception", config)
   float MaxQueryImportance;

   UPROPERTY(EditDefaultsOnly, Category = "AI Perception", config)
   float SightLimitQueryImportance;

   ECollisionChannel DefaultSightCollisionChannel;

public:

   virtual void PostInitProperties() override;

   void RegisterEvent(const FOSEAISightEvent& event);

   virtual void RegisterSource(AActor& sourceActors) override;
   virtual void UnregisterSource(AActor& sourceActor) override;

   virtual void OnListenerForgetsActor(const FPerceptionListener& listener, AActor& actorToForget) override;
   virtual void OnListenerForgetsAll(const FPerceptionListener& listener) override;

protected:
   virtual float Update() override;

   virtual bool _ShouldAutomaticallySeeTarget(const FDigestedSightProperties& propDigest, FOSEAISightQuery* sightQuery, FPerceptionListener& listener, AActor* targetActor) const;
   virtual bool _ShouldNeverSeeTarget(const FDigestedSightProperties& propDigest, FOSEAISightQuery* sightQuery, FPerceptionListener& listener, AActor* targetActor) const;


   void _OnNewListenerImpl(const FPerceptionListener& newListener);
   void _OnListenerUpdateImpl(const FPerceptionListener& updatedListener);
   void _OnListenerRemovedImpl(const FPerceptionListener& removedListener);
   virtual void OnListenerConfigUpdated(const FPerceptionListener& updatedListener) override;

   void _GenerateQueriesForListener(const FPerceptionListener& listener, const FDigestedSightProperties& propertyDigest, const TFunction<void(FOSEAISightQuery&)>& onAddedFunc = nullptr);

   void _RemoveAllQueriesByListener(const FPerceptionListener& listener, const TFunction<void(const FOSEAISightQuery&)>& onRemoveFunc = nullptr);
   void _RemoveAllQueriesToTarget(const FOSEAISightTarget::FTargetId& targetId, const TFunction<void(const FOSEAISightQuery&)>& onRemoveFunc = nullptr);

   /** returns information whether new LoS queries have been added */
   bool _RegisterTarget(AActor& targetActor, const TFunction<void(FOSEAISightQuery&)>& onAddedFunc = nullptr);
   virtual const FDigestedSightProperties& _SetupDigestedPropertiesForListener(const UAIPerceptionComponent& perceptionComponent);

   float _CalcQueryImportance(const FPerceptionListener& listener, const FVector& targetLocation, const float sightRadiusSq) const;

private:
   void _CreateQueriesForTargetAndListener(const AActor& targetActor, const TFunction<void(FOSEAISightQuery&)>& onAddedFunc,
                                           const FOSEAISightTarget::FTargetId& targetID, bool& newQueriesAdded,
                                           const FVector& targetLocation, const FPerceptionListener& listener,
                                           const IGenericTeamAgentInterface* listenersTeamAgent,
                                           const FDigestedSightProperties& propDigest, const EAISightBucket& bucketToUse);
   bool _PerformFrustumCheck(const FPerceptionListener& listener,
                             const FDigestedSightProperties& propDigest,
                             const AActor* target,
                             const FVector& targetLocation,
                             const FVector& targetExtents,
                             bool wasLastQuerySuccessful) const;

   bool _IsTargetWithinViewArea(
      const FPerceptionListener& listener, 
      const AActor* target, 
      const FDigestedSightProperties& propDigest, 
      bool wasLastQuerySuccessful) const;
  
   static void _ModifySightRadius(
      const UAIPerceptionComponent* listenerPtr,
      const FDigestedSightProperties& propDigest,
      float& sightRadiusSq);
   
   static void _EnforceMinimumSightSquareRadius(
      const FDigestedSightProperties& propDigest,
      float& sightRadiusSq);
   
   bool _ComputeVisibility(
      const UWorld* world,
      int32& tracesCount,
      FOSEAISightQuery* sightQuery,
      FPerceptionListener& listener,
      const FOSEAISightTarget& target,
      AActor* targetActor,
      const UAIPerceptionComponent* listenerPtr, 
      const FVector& targetLocation,
      const FDigestedSightProperties& propDigest,
      float& outStimulusStrength,
      FVector& outSeenLocation) const;
   
   float _HandleStandardSightUpdate();
   float _HandleQualityScalingSightUpdate();
   
   enum class EOperationType : uint8
   {
      Remove,
      SwapList
   };
   struct FQueryOperation
   {
      FQueryOperation(const bool isInRange, const EOperationType InOpType, const int32 InIndex) : IsInRange(isInRange), OpType(InOpType), Index(InIndex) {}
      bool IsInRange;
      EOperationType OpType;
      int32 Index;
   };

   static void _HandleSwapOrRemoveOperations(
      TArray<FQueryOperation>& queryOperations, 
      FOSEAISightBucket& bucket);
   
   void _HandleInvalidTargets(
      TArray<FOSEAISightTarget::FTargetId>& invalidTargets,
      FTargetsContainer& observedTargets);
   
   double _HandleTimeSlicingForBucket(
      const UWorld* world,
      AIPerception::FListenerMap& listenersMap,
      float maxFrameTimeForQuery,
      int maxQueries,
      int& traceCount,
      int maxTraces, FOSEAISightBucket& bucket);

   bool _HandleQuery(
      const UWorld* world,
      AIPerception::FListenerMap& listenersMap,
      int& tracesCount,
      FOSEAISightQuery& query,
      const int& inRangeIndex, 
      const int& outOfRangeIndex,
      const bool& isInRangeQuery, 
      TArray<FQueryOperation>& queryOperations,
      TArray<FOSEAISightTarget::FTargetId>& invalidTargets);
   
   void _HandleBucketQuerySorting(FOSEAISightBucket& bucket);
};
