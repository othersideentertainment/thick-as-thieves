// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Perception/OSEAISense_Sight.h"

// ose
#include "AI/OSEAIController.h"
#include "AI/OSEAISettings.h"
#include "AI/Perception/OSEAIPerceptionHelpers.h"
#include "AI/Perception/OSEAISenseConfig_Sight.h"
#include "AI/Perception/OSEAISenseSharedConfigData.h"
#include "AI/Perception/OSEAISightInterface.h"

// ose 
#include "AI/Perception/OSEAITargetSightInterface.h"

// ue4
#include "AIHelpers.h"
#include "AISystem.h"
#include "CollisionQueryParams.h"
#include "EngineDefines.h"
#include "EngineGlobals.h"
#include "GameplayTagAssetInterface.h"
#include "Engine/Engine.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISightTargetInterface.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAISense_Sight)

#define DO_SIGHT_VLOGGING (ENABLE_VISUAL_LOG)

#define DO_STAT_TARGET_AGE 1

#if DO_STAT_TARGET_AGE
#define STAT_LOG_TARGET_AGE(bucket, statTag) \
const int lastIndexOutOfRange = bucket.SightQueriesOutOfRange.Num() - 1; \
const int lastIndexInRange = bucket.SightQueriesInRange.Num() - 1; \
if (bucket.SightQueriesOutOfRange.IsValidIndex(lastIndexOutOfRange)) \
{ \
   SET_DWORD_STAT(statTag##_Out, bucket.SightQueriesOutOfRange[lastIndexOutOfRange].GetAge());\
}\
if (bucket.SightQueriesInRange.IsValidIndex(lastIndexInRange))\
{\
   SET_DWORD_STAT(statTag##_In, bucket.SightQueriesInRange[lastIndexInRange].GetAge());\
}
#else 
#define STAT_LOG_TARGET_AGE(bucket, statTag) 
#endif

#if DO_SIGHT_VLOGGING
#define SIGHT_LOG_SEGMENT(LogOwner, SegmentStart, SegmentEnd, Color, Format, ...) UE_VLOG_SEGMENT(LogOwner, LogAIPerception, Verbose, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__)
#define SIGHT_LOG_LOCATION(LogOwner, Location, Radius, Color, Format, ...) UE_VLOG_LOCATION(LogOwner, LogAIPerception, Verbose, Location, Radius, Color, Format, ##__VA_ARGS__)
#else
#define SIGHT_LOG_SEGMENT(...)
#define SIGHT_LOG_LOCATION(...)
#endif // DO_SIGHT_VLOGGING

DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight"), STAT_OSE_AI_Sense_Sight, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Update Sort"), STAT_OSE_AI_Sense_Sight_UpdateSort, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Listener Update"), STAT_OSE_AI_Sense_Sight_ListenerUpdate, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Register Target"), STAT_OSE_AI_Sense_Sight_RegisterTarget, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Remove By Listener"), STAT_OSE_AI_Sense_Sight_RemoveByListener, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight, Remove To Target"), STAT_OSE_AI_Sense_Sight_RemoveToTarget, STATGROUP_AI);

DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight [Quality Bucket Overall]"), STAT_OSE_AI_Sense_Sight_QualityBucket, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight [Quality Bucket: High]"), STAT_OSE_AI_Sense_Sight_QualityBucket_High, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight [Quality Bucket: Medium]"), STAT_OSE_AI_Sense_Sight_QualityBucket_Medium, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight [Quality Bucket: Low]"), STAT_OSE_AI_Sense_Sight_QualityBucket_Low, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("OSE Perception Sense: Sight [Quality Bucket: None]"), STAT_OSE_AI_Sense_Sight_QualityBucket_None, STATGROUP_AI);

DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Overall] Traces Per frame"), STAT_OSE_AI_Sense_Sight_TracesPerFrame, STATGROUP_AI);

DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: High] Traces Per frame"), STAT_OSE_AI_Sense_Sight_TracesPerFrame_High, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Medium] Traces Per frame"), STAT_OSE_AI_Sense_Sight_TracesPerFrame_Medium, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Low] Traces Per frame"), STAT_OSE_AI_Sense_Sight_TracesPerFrame_Low, STATGROUP_AI);

DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: High] Oldest Age Target (Out)"), STAT_OSE_AI_Sense_Sight_OldestAgeTarget_High_Out, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: High] Oldest Age Target (In)"), STAT_OSE_AI_Sense_Sight_OldestAgeTarget_High_In, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Medium] Oldest Age Target (Out)"), STAT_OSE_AI_Sense_Sight_OldestAgeTarget_Medium_Out, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Medium] Oldest Age Target (In)"), STAT_OSE_AI_Sense_Sight_OldestAgeTarget_Medium_In, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Low] Oldest Age Target (Out)"), STAT_OSE_AI_Sense_Sight_OldestAgeTarget_Low_Out, STATGROUP_AI);
DECLARE_DWORD_COUNTER_STAT(TEXT("OSE Sight [Quality Bucket: Low] Oldest Age Target (In)"), STAT_OSE_AI_Sense_Sight_OldestAgeTarget_Low_In, STATGROUP_AI);

namespace TATAIPerceptionSightSenseCVars
{
   static bool IsSightSenseQualityScalingBucketEnabled = true;
   FAutoConsoleVariableRef CVarSightSenseQualityBucketVersion(
      TEXT("TAT.Perception.Sight.QualityBucket.Enabled"),
      IsSightSenseQualityScalingBucketEnabled,
      TEXT(""),
      ECVF_Default);
   
   static float HighBucketMaxSightFrameTime = 0.003f;
   FAutoConsoleVariableRef CVarHighBucketMaxSightFrameTime(
      TEXT("TAT.Perception.Sight.QualityBucket.HighBucketMaxSightFrameTime"),
      HighBucketMaxSightFrameTime,
      TEXT(""),
      ECVF_Default);
   
   static float MediumBucketMaxSightFrameTime = 0.002f;
   FAutoConsoleVariableRef CVarMediumBucketMaxSightFrameTime(
      TEXT("TAT.Perception.Sight.QualityBucket.MediumBucketMaxSightFrameTime"),
      MediumBucketMaxSightFrameTime,
      TEXT(""),
      ECVF_Default);
   
   static float LowBucketMaxSightFrameTime = 0.001f;
   FAutoConsoleVariableRef CVarLowBucketMaxSightFrameTime(
      TEXT("TAT.Perception.Sight.QualityBucket.LowBucketMaxSightFrameTime"),
      LowBucketMaxSightFrameTime,
      TEXT(""),
      ECVF_Default);
   
   static int HighBucketMaxProcessedQueriesPerFrame = 30;
   FAutoConsoleVariableRef CVarHighBucketMaxQueriesPerFrame(
      TEXT("TAT.Perception.Sight.QualityBucket.HighBucketMaxProcessedQueriesPerFrame"),
      HighBucketMaxProcessedQueriesPerFrame,
      TEXT(""),
      ECVF_Default);
   
   static int MediumBucketMaxProcessedQueriesPerFrame = 20;
   FAutoConsoleVariableRef CVarMediumBucketMaxQueriesPerFrame(
      TEXT("TAT.Perception.Sight.QualityBucket.MediumBucketMaxProcessedQueriesPerFrame"),
      MediumBucketMaxProcessedQueriesPerFrame,
      TEXT(""),
      ECVF_Default);
   
   static int LowBucketMaxProcessedQueriesPerFrame = 20;
   FAutoConsoleVariableRef CVarLowBucketMaxQueriesPerFrame(
      TEXT("TAT.Perception.Sight.QualityBucket.LowBucketMaxProcessedQueriesPerFrame"),
      LowBucketMaxProcessedQueriesPerFrame,
      TEXT(""),
      ECVF_Default);
   
}

static const int32 kDefaultMaxTracesPerTick = 6;
static const int32 kDefaultMinQueriesPerTimeSliceCheck = 10;

enum class EForEachResult : uint8
{
   Break,
   Continue,
};

template <typename T, class PREDICATE_CLASS>
EForEachResult ForEach(T& array, const PREDICATE_CLASS& predicate)
{
   for (typename T::ElementType& element : array)
   {
      if (predicate(element) == EForEachResult::Break)
      {
         return EForEachResult::Break;
      }
   }
   return EForEachResult::Continue;
}

enum EReverseForEachResult : uint8
{
   UnTouched,
   Modified,
};

template <typename T, class PREDICATE_CLASS>
EReverseForEachResult ReverseForEach(T& array, const PREDICATE_CLASS& predicate)
{
   EReverseForEachResult retVal = EReverseForEachResult::UnTouched;
   for (int32 index = array.Num() - 1; index >= 0; --index)
   {
      if (predicate(array, index) == EReverseForEachResult::Modified)
      {
         retVal = EReverseForEachResult::Modified;
      }
   }
   return retVal;
}

//----------------------------------------------------------------------//
// FOSEAISightTarget
//----------------------------------------------------------------------//
const FOSEAISightTarget::FTargetId FOSEAISightTarget::InvalidTargetId = FAISystem::InvalidUnsignedID;

FOSEAISightTarget::FOSEAISightTarget(AActor* inTarget, FGenericTeamId inTeamId)
   : Target(inTarget), SightTargetInterface(nullptr), TeamId(inTeamId)
{
   if (inTarget)
   {
      TargetId = inTarget->GetUniqueID();
   }
   else
   {
      TargetId = InvalidTargetId;
   }
}

//----------------------------------------------------------------------//
// FDigestedSightProperties
//----------------------------------------------------------------------//
FDigestedSightProperties::FDigestedSightProperties()
{
   SightRadiusSq = -1.0f;
   LoseSightRadiusSq = -1.0f;
   MinimumSightRadiusSq = -1.0f;
   HalfFOVInDegrees = 0.0f;
   PointOfViewBackwardOffset = 0.0f;
   MinSightFarClip = 0.f;
   NearClip = 0.0f;
   FarClip = 0.0f;
   LoseSightFarClip = 0.0f;
   FrustumPitch = -18.0f;
   FrustumAspectRatio = 2.2f;
   AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
   AffiliationFlags = -1;
}

//----------------------------------------------------------------------//
// UOSEAISense_Sight
//----------------------------------------------------------------------//
UOSEAISense_Sight::UOSEAISense_Sight(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , MaxTracesPerTick(kDefaultMaxTracesPerTick)
   , MinQueriesPerTimeSliceCheck(kDefaultMinQueriesPerTimeSliceCheck)
   , MaxTimeSlicePerTick(0.005) // 5ms
   , HighImportanceQueryDistanceThreshold(300.f)
   , MaxQueryImportance(60.f)
   , SightLimitQueryImportance(10.f)
{
   if (HasAnyFlags(RF_ClassDefaultObject) == false)
   {
      UOSEAISenseConfig_Sight* sightConfigCDO = GetMutableDefault<UOSEAISenseConfig_Sight>();
      sightConfigCDO->Implementation = UOSEAISense_Sight::StaticClass();

      OnNewListenerDelegate.BindUObject(this, &UOSEAISense_Sight::_OnNewListenerImpl);
      OnListenerUpdateDelegate.BindUObject(this, &UOSEAISense_Sight::_OnListenerUpdateImpl);
      OnListenerRemovedDelegate.BindUObject(this, &UOSEAISense_Sight::_OnListenerRemovedImpl);
   }

   NotifyType = EAISenseNotifyType::OnPerceptionChange;

   bAutoRegisterAllPawnsAsSources = true;
   bNeedsForgettingNotification = true;

   for (int i = EAISightBucket::High; i < EAISightBucket::Count; ++i)
   {
      SightQueryBuckets.Add(static_cast<EAISightBucket>(i), FOSEAISightBucket());
   }
   
   DefaultSightCollisionChannel = GET_AI_CONFIG_VAR(DefaultSightCollisionChannel);
}

bool UOSEAISense_Sight::_IsTargetWithinViewArea(const FPerceptionListener& listener,
                                                const AActor* target,
                                                const FDigestedSightProperties& propDigest,
                                                const bool wasLastQuerySuccessful) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_IsTargetWithinViewArea)

   bool shouldCheckBounds = false;
   if(const IOSEAISightInterface* sightInterface = propDigest.SightInterface)
   {
      shouldCheckBounds = sightInterface->ShouldCheckBoundsForVision();
   }
   if(shouldCheckBounds)
   {
      FVector targetLocation;
      FVector targetExtents;
      {
         TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::GetActorBounds)
         constexpr bool onlyCollidingComponents = true;
         target->GetActorBounds(onlyCollidingComponents, targetLocation, targetExtents);
      }
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_PerformFrustumCheckWithBounds)
      return _PerformFrustumCheck(listener, propDigest, target, targetLocation, targetExtents, wasLastQuerySuccessful);
   }
   else
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_PerformFrustumCheckWithoutBounds)
      // if we are only checking the point, pass in a zero target extent vector.
      return _PerformFrustumCheck(listener, propDigest, target, target->GetActorLocation(), FVector::ZeroVector, wasLastQuerySuccessful);
   }
}

FORCEINLINE_DEBUGGABLE float UOSEAISense_Sight::_CalcQueryImportance(const FPerceptionListener& listener, const FVector& targetLocation, const float sightRadiusSq) const
{
   const float distanceSq = FVector::DistSquared(listener.CachedLocation, targetLocation);
   return distanceSq <= _highImportanceDistanceSquare ? MaxQueryImportance
      : FMath::Clamp((SightLimitQueryImportance - MaxQueryImportance) / sightRadiusSq * distanceSq + MaxQueryImportance, 0.f, MaxQueryImportance);
}

bool UOSEAISense_Sight::_PerformFrustumCheck(const FPerceptionListener& listener,
   const FDigestedSightProperties& propDigest,
   const AActor* target,
   const FVector& targetLocation,
   const FVector& targetExtents,
   const bool wasLastQuerySuccessful) const
{
   const AActor* bodyActor = listener.GetBodyActor();
   const FVector observerLocation = listener.CachedLocation;
   // If the pawn that this controller is attached to is possessed by another controller (e.g. a player),
   // then our bodyActor will be null. In that case, we should skip any frustum checks since we don't know
   // where the pawn is anymore. We could use the cached location/direction, but this will be wrong as the pawn moves
   // away from where it was the last time we controlled it
   if (!bodyActor)
   {
      return false;
   }

   const float halfFOVInDegrees = propDigest.HalfFOVInDegrees;
   const float backwardOffset = propDigest.PointOfViewBackwardOffset;
   const float nearClip = propDigest.NearClip;

   // Scale the far clip on the frustum so that it matches the max sight radius when it's modified by the shared config
   float farClip = wasLastQuerySuccessful ? propDigest.LoseSightFarClip : propDigest.FarClip;
   _ModifySightRadius(listener.Listener.Get(), propDigest, farClip);
   if (const IOSEAISightInterface* sightInterface = propDigest.SightInterface)
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_PerformFrustumCheck_ModifySightRangeForSpecificActor)
      sightInterface->ModifySightRangeForSpecificActor(target, farClip);
   }
   // Enforce the minimum far clip
   farClip = FMath::Max(propDigest.MinSightFarClip, farClip);
   // Early out if this target has reduced sight to zero.
   if(farClip <= 0.f)
      return false;
   
   FVector actorEyeLocation;
   FRotator frustumRotation;
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_PerformFrustumCheck_GetActorEyesViewPoint)
      bodyActor->GetActorEyesViewPoint(actorEyeLocation, frustumRotation);
   }
   
   // Sight cone offsets backwards based on view direction
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_PerformFrustumCheck_CheckIsTargetInFrustum)
      const FVector frustumOrigin = observerLocation - (frustumRotation.Vector() * backwardOffset);
      const bool targetInFrustum = OSEAIPerceptionHelpers::CheckIsTargetInFrustum(
         bodyActor->GetWorld(),
         frustumOrigin,
         frustumRotation,
         targetLocation,
         targetExtents,
         halfFOVInDegrees,
         nearClip,
         farClip,
         propDigest.FrustumPitch,
         propDigest.FrustumAspectRatio);

      if(targetInFrustum)
      {
         TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_PerformFrustumCheck_ComputeSquaredDistanceFromBoxToPoint)
         const FBoxSphereBounds bounds = FBoxSphereBounds(targetLocation, targetExtents, targetExtents.Size());
         // Okay we are inside the frustum, but are we outside of max range for the vision.
         const float actorDistance = bounds.ComputeSquaredDistanceFromBoxToPoint(actorEyeLocation);
         if(actorDistance > FMath::Square(farClip))
            return false;
      }
   return targetInFrustum;
   }
}

void UOSEAISense_Sight::PostInitProperties()
{
   Super::PostInitProperties();
   _highImportanceDistanceSquare = FMath::Square(HighImportanceQueryDistanceThreshold);
}

bool UOSEAISense_Sight::_ShouldAutomaticallySeeTarget(
   const FDigestedSightProperties& propDigest,
   FOSEAISightQuery* sightQuery, 
   FPerceptionListener& listener,
   AActor* targetActor) const
{
   if ((propDigest.AutoSuccessRangeSqFromLastSeenLocation != FAISystem::InvalidRange) && (sightQuery->LastSeenLocation != FAISystem::InvalidLocation))
   {
      const float distanceToLastSeenLocationSq = FVector::DistSquared(targetActor->GetActorLocation(), sightQuery->LastSeenLocation);
      return (distanceToLastSeenLocationSq <= propDigest.AutoSuccessRangeSqFromLastSeenLocation);
   }

   return false;
}

bool UOSEAISense_Sight::_ShouldNeverSeeTarget(
   const FDigestedSightProperties& propDigest,
   FOSEAISightQuery* sightQuery,
   FPerceptionListener& listener,
   AActor* targetActor) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_ShouldNeverSeeTarget)
   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(targetActor))
   {
      const UOSEAISettings& settings = UOSEAISettings::Get();
      if (tagInterface->HasAnyMatchingGameplayTags(settings.DoNotSeeActorTags))
         return true;
   }

   if (const IOSEAISightInterface* sightInterface = propDigest.SightInterface)
   {
      if (!sightInterface->IsAllowedToSeeActor(targetActor))
         return true;
   }

   return false;
}

void UOSEAISense_Sight::_ModifySightRadius(
   const UAIPerceptionComponent* listenerPtr,
   const FDigestedSightProperties& propDigest,
   float& sightRadiusSq)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_ModifySightRadius)
   if(propDigest.SharedConfigData.IsValid())
   {
      // only calculate once per frame
      if(propDigest.CachedRangeModifiersFrame != GFrameCounter)
      {
         propDigest.CachedRangeModifiers = propDigest.SharedConfigData->CalculateRangePerceptionModifiers(listenerPtr);
         propDigest.CachedRangeModifiersFrame = GFrameCounter;
      }
      sightRadiusSq *= propDigest.CachedRangeModifiers;
   }
}

void UOSEAISense_Sight::_EnforceMinimumSightSquareRadius(const FDigestedSightProperties& propDigest,
   float& sightRadiusSq)
{
   sightRadiusSq = FMath::Max(propDigest.MinimumSightRadiusSq, sightRadiusSq);
}

bool UOSEAISense_Sight::_ComputeVisibility(
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
   FVector& outSeenLocation) const
{

   if (_ShouldAutomaticallySeeTarget(propDigest, sightQuery, listener, targetActor))
   {
      SIGHT_LOG_SEGMENT(listenerPtr->GetOwner(), listener.CachedLocation, targetLocation, FColor::Purple, TEXT("TargetID %d"), target.TargetId);
      return _ShouldNeverSeeTarget(propDigest, sightQuery, listener, targetActor) == false;
   }
   
   if (_IsTargetWithinViewArea(listener, targetActor, propDigest, sightQuery->bLastResult) == false)
   {
      SIGHT_LOG_SEGMENT(listenerPtr->GetOwner(), listener.CachedLocation, targetLocation, FColor::Yellow, TEXT("TargetID %d"), target.TargetId);
      return false;
   }
   
   if(_ShouldNeverSeeTarget(propDigest, sightQuery, listener, targetActor))
   {
      SIGHT_LOG_SEGMENT(listenerPtr->GetOwner(), listener.CachedLocation, targetLocation, FColor::Magenta, TEXT("TargetID %d"), target.TargetId);
      return false;
   }
   
   
   // do line checks
   if (target.SightTargetInterface != nullptr)
   {
      TRACE_CPUPROFILER_EVENT_SCOPE(DoTraceCustom)
      const bool bWasVisible = sightQuery->bLastResult;
      FCanBeSeenFromContext Context;
      Context.SightQueryID = FAISightQueryID(sightQuery->ObserverId, sightQuery->TargetId);
      Context.ObserverLocation = listener.CachedLocation;
      Context.IgnoreActor = listenerPtr->GetBodyActor();
      Context.bWasVisible = &bWasVisible;

      int32 numberOfLoSChecksPerformed = 0;  
      int32 numberOfLoSChecksRequested = 0;
      bool visible = UAISense_Sight::EVisibilityResult::Visible == target.SightTargetInterface->CanBeSeenFrom(
         Context,
         outSeenLocation,
         numberOfLoSChecksPerformed,
         numberOfLoSChecksRequested,
         outStimulusStrength,
         &sightQuery->UserData
      );
      SIGHT_LOG_SEGMENT(listenerPtr->GetOwner(), listener.CachedLocation, targetLocation, visible ? FColor::Green : FColor::Red, TEXT("TargetID %d"), target.TargetId);
      tracesCount += numberOfLoSChecksPerformed;
      return visible;
   }
   
   TRACE_CPUPROFILER_EVENT_SCOPE(DoTrace)
   constexpr bool bTraceComplex = false;
   // we need to do tests ourselves
   FHitResult hitResult;
   const bool wasHit = world->LineTraceSingleByChannel(hitResult, 
      listener.CachedLocation,
      targetLocation,
      DefaultSightCollisionChannel,
      FCollisionQueryParams(
         SCENE_QUERY_STAT(AILineOfSight),
         bTraceComplex,
         listenerPtr->GetBodyActor()
      )
   );

   ++tracesCount;

   auto HitResultActorIsOwnedByTargetActor = [&hitResult, targetActor]()
   {
      const AActor* hitResultActor = hitResult.HitObjectHandle.FetchActor();
      return (hitResultActor ? hitResultActor->IsOwnedBy(targetActor) : false);
   };

   const bool visible =  (wasHit == false || HitResultActorIsOwnedByTargetActor());
   SIGHT_LOG_SEGMENT(listenerPtr->GetOwner(), listener.CachedLocation, targetLocation, visible ? FColor::Green : FColor::Red, TEXT("TargetID %d"), target.TargetId);
   
   if(visible)
   {
      outSeenLocation = targetActor->GetActorLocation();
   }
   return visible;
}

bool UOSEAISense_Sight::_HandleQuery(
   const UWorld* world,
   AIPerception::FListenerMap& listenersMap,
   int& tracesCount,
   FOSEAISightQuery& query,
   const int& inRangeIndex,
   const int& outOfRangeIndex,
   const bool& isInRangeQuery,
   TArray<FQueryOperation>& queryOperations,
   TArray<FOSEAISightTarget::FTargetId>& invalidTargets)
{
   FPerceptionListener& listener = listenersMap[query.ObserverId];
   const FOSEAISightTarget& target = ObservedTargets[query.TargetId];

   AActor* targetActor = target.Target.Get();
   const UAIPerceptionComponent* listenerPtr = listener.Listener.Get();
   ensure(listenerPtr);
   if (targetActor && listenerPtr)
   {
      const FVector targetLocation = targetActor->GetActorLocation();

      const FDigestedSightProperties& propDigest = DigestedProperties[query.ObserverId];

      float sightRadiusSq = query.bLastResult ? propDigest.LoseSightRadiusSq : propDigest.SightRadiusSq;
      _ModifySightRadius(listenerPtr, propDigest, sightRadiusSq);
      _EnforceMinimumSightSquareRadius(propDigest, sightRadiusSq);

      float stimulusStrength = 1;
      FVector seenLocation = FAISystem::InvalidLocation;
      const bool visible = _ComputeVisibility(
         world,
         tracesCount,
         &query,
         listener,
         target,
         targetActor,
         listenerPtr,
         targetLocation,
         propDigest,
         stimulusStrength,
         seenLocation
      );

      if(visible)
      {
         const bool hasValidSeenLocation = seenLocation != FAISystem::InvalidLocation;
         listener.RegisterStimulus(
            targetActor,
            FAIStimulus(
               *this,
               stimulusStrength,
               hasValidSeenLocation ? seenLocation : query.LastSeenLocation,
               listener.CachedLocation)
         );
         query.bLastResult = true;
         if(hasValidSeenLocation)
         {
            query.LastSeenLocation = seenLocation;
         }
      }
      else if (query.bLastResult)
      {
         constexpr float failedStimulusStrength = 0.0f;
         listener.RegisterStimulus(
            targetActor,
            FAIStimulus(
               *this, 
               failedStimulusStrength, 
               targetLocation, 
               listener.CachedLocation, 
               FAIStimulus::SensingFailed)
         );
         query.bLastResult = false;
         query.LastSeenLocation = FAISystem::InvalidLocation;
      }

      query.Importance = _CalcQueryImportance(listener, targetLocation, sightRadiusSq);
      
      const bool shouldBeInRange = query.Importance > 0.0f;
      if (isInRangeQuery != shouldBeInRange)
      {
         queryOperations.Add(FQueryOperation(isInRangeQuery, EOperationType::SwapList, isInRangeQuery ? inRangeIndex : outOfRangeIndex));
      }
      // restart query
      query.OnProcessed();
      return true;
   }
   
   // put this index to "to be removed" array
   queryOperations.Add(FQueryOperation(isInRangeQuery, EOperationType::Remove, isInRangeQuery ? inRangeIndex : outOfRangeIndex));
   if (targetActor == nullptr)
   {
      invalidTargets.AddUnique(query.TargetId);
   }
   return false;
}

void UOSEAISense_Sight::_HandleBucketQuerySorting(FOSEAISightBucket& bucket)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleBucketQuerySorting)
   auto RecalcScore = [](FOSEAISightQuery& query)->EForEachResult
   {
      query.RecalcScore();
      return EForEachResult::Continue;
   };

   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_UpdateSort);
   // Sort out of range queries
   if (bucket.bSightQueriesOutOfRangeDirty)
   {
      ForEach(bucket.SightQueriesOutOfRange, RecalcScore);
      bucket.SightQueriesOutOfRange.Sort(FOSEAISightQuery::FSortPredicate());
      bucket.NextOutOfRangeIndex = 0;
      bucket.bSightQueriesOutOfRangeDirty = false;
   }

   // Sort in range queries
   ForEach(bucket.SightQueriesInRange, RecalcScore);
   bucket.SightQueriesInRange.Sort(FOSEAISightQuery::FSortPredicate());
}

double UOSEAISense_Sight::_HandleTimeSlicingForBucket(
   const UWorld* world,
   AIPerception::FListenerMap& listenersMap,
   const float maxFrameTimeForQuery,
   const int maxQueries,
   int& traceCount,
   const int maxTraces,
   FOSEAISightBucket& bucket)
{
   // other queries have run out of time, exit out.
   if (maxFrameTimeForQuery <= 0.f || traceCount >= maxTraces)
      return 0.f;
   _HandleBucketQuerySorting(bucket);
   
   TArray<FQueryOperation> queryOperations;
   TArray<FOSEAISightTarget::FTargetId> invalidTargets;
   static constexpr int32 kInitialInvalidItemsSize = 16;
   
   queryOperations.Reserve(kInitialInvalidItemsSize);
   invalidTargets.Reserve(kInitialInvalidItemsSize);

   int32 inRangeItr = 0;
   int32 outOfRangeItr = 0;
   int numQueriesProcessed = 0;
   const double timeSliceEnd = FPlatformTime::Seconds() + maxFrameTimeForQuery;
   bool hitTimeSliceLimit = false;
   
   for (int32 queryIndex = 0; queryIndex < bucket.SightQueriesInRange.Num() + bucket.SightQueriesOutOfRange.Num(); ++queryIndex)
   {
      // Calculate next in range query
      int32 inRangeIndex = bucket.SightQueriesInRange.IsValidIndex(inRangeItr) ? inRangeItr : INDEX_NONE;
      FOSEAISightQuery* inRangeQuery = inRangeIndex != INDEX_NONE ? &bucket.SightQueriesInRange[inRangeIndex] : nullptr;

      // Calculate next out of range query
      int32 outOfRangeIndex = bucket.SightQueriesOutOfRange.IsValidIndex(outOfRangeItr) ? (bucket.NextOutOfRangeIndex + outOfRangeItr) % bucket.SightQueriesOutOfRange.Num() : INDEX_NONE;
      FOSEAISightQuery* outOfRangeQuery = outOfRangeIndex != INDEX_NONE ? &bucket.SightQueriesOutOfRange[outOfRangeIndex] : nullptr;
      if (outOfRangeQuery)
      {
         outOfRangeQuery->RecalcScore();
      }

      // Compare to real find next query
      const bool isInRangeQuery = (inRangeQuery && outOfRangeQuery) ? FOSEAISightQuery::FSortPredicate()(*inRangeQuery, *outOfRangeQuery) : !outOfRangeQuery;
      FOSEAISightQuery* sightQuery = isInRangeQuery ? inRangeQuery : outOfRangeQuery;

      // Time slice limit check - spread out checks to every N queries so we don't spend more time checking timer than doing work
      numQueriesProcessed++;
#ifdef AISENSE_SIGHT_TIMESLICING_DEBUG
      timeSpent += (FPlatformTime::Seconds() - lastTime);
      lastTime = FPlatformTime::Seconds();
#endif // AISENSE_SIGHT_TIMESLICING_DEBUG
      if (hitTimeSliceLimit == false && (numQueriesProcessed % MinQueriesPerTimeSliceCheck) == 0 && FPlatformTime::Seconds() > timeSliceEnd)
      {
         hitTimeSliceLimit = true;
         // do not break here since that would bypass queue aging
      }

      if (queryIndex < maxQueries && traceCount < maxTraces && hitTimeSliceLimit == false)
      {
         TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleQuery)
         isInRangeQuery ? ++inRangeItr : ++outOfRangeItr;

         // @todo figure out what should we do if not valid
         _HandleQuery(world,
            listenersMap,
            traceCount,
            *sightQuery,
            inRangeIndex,
            outOfRangeIndex,
            isInRangeQuery,
            queryOperations,
            invalidTargets);
      }
      else
      {
         break;
      }
   }
   bucket.NextOutOfRangeIndex = bucket.SightQueriesOutOfRange.Num() > 0 ? (bucket.NextOutOfRangeIndex + outOfRangeItr) % bucket.SightQueriesOutOfRange.Num() : 0;

#ifdef AISENSE_SIGHT_TIMESLICING_DEBUG
   UE_LOG(LogAIPerception, VeryVerbose, TEXT("UOSEAISense_Sight::Update processed %d sources in %f seconds [time slice limited? %d]"), numQueriesProcessed, timeSpent, hitTimeSliceLimit ? 1 : 0);
#else
   UE_LOG(LogAIPerception, VeryVerbose, TEXT("UOSEAISense_Sight::Update processed %d sources [time slice limited? %d]"), numQueriesProcessed, hitTimeSliceLimit ? 1 : 0);
#endif // AISENSE_SIGHT_TIMESLICING_DEBUG
   _HandleSwapOrRemoveOperations(queryOperations, bucket);
   _HandleInvalidTargets(invalidTargets, ObservedTargets);
   
   return FPlatformTime::Seconds() - timeSliceEnd;
}

float UOSEAISense_Sight::_HandleStandardSightUpdate()
{
   AIPerception::FListenerMap& listenersMap = *GetListeners();
   int traceCount = 0;   
   const UWorld* world = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter(), EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr)
      return SuspendNextUpdate;
   
   // Half of the time can be used for the high queries
   const double maxFrameTimeForHighQueries = MaxTimeSlicePerTick;
      SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_QualityBucket_None)
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleStandardSightUpdate)
   _HandleTimeSlicingForBucket(
      world, 
      listenersMap, 
      maxFrameTimeForHighQueries,
      99999, // Max queries was never set for the previous implementation
      traceCount,
      MaxTracesPerTick, 
      SightQueryBuckets[EAISightBucket::NonQualityControlledBucket]
   );
   
   SET_DWORD_STAT(STAT_OSE_AI_Sense_Sight_TracesPerFrame, traceCount);
   
   return 0.f;
}

float UOSEAISense_Sight::_HandleQualityScalingSightUpdate()
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_QualityBucket)
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleQualityScalingSightUpdate)
   
   const UWorld* world = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter(), EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr)
      return SuspendNextUpdate;
   
   AIPerception::FListenerMap& listenersMap = *GetListeners();
   int traceCount = 0;
   
   FOSEAISightBucket& highQueries = SightQueryBuckets[EAISightBucket::High];
   FOSEAISightBucket& mediumQueries = SightQueryBuckets[EAISightBucket::Medium];
   FOSEAISightBucket& lowQueries = SightQueryBuckets[EAISightBucket::Low];
   
   {
      SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_QualityBucket_High)
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleTimeSlicingForBucket_High)
      const int traceCountBefore = traceCount;
      _HandleTimeSlicingForBucket(
         world, 
         listenersMap, 
         TATAIPerceptionSightSenseCVars::HighBucketMaxSightFrameTime,
         TATAIPerceptionSightSenseCVars::HighBucketMaxProcessedQueriesPerFrame,
         traceCount,
         MaxTracesPerTick,
         highQueries);
      SET_DWORD_STAT(STAT_OSE_AI_Sense_Sight_TracesPerFrame_High, traceCount - traceCountBefore);

      STAT_LOG_TARGET_AGE(highQueries, STAT_OSE_AI_Sense_Sight_OldestAgeTarget_High);
   }
   {
      SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_QualityBucket_Medium)
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleTimeSlicingForBucket_Medium)
      
      const int traceCountBefore = traceCount;      
      _HandleTimeSlicingForBucket(
         world,
         listenersMap,
         TATAIPerceptionSightSenseCVars::MediumBucketMaxSightFrameTime, 
         TATAIPerceptionSightSenseCVars::MediumBucketMaxProcessedQueriesPerFrame, 
         traceCount,
         MaxTracesPerTick,
         mediumQueries);
      SET_DWORD_STAT(STAT_OSE_AI_Sense_Sight_TracesPerFrame_Medium, traceCount - traceCountBefore);
      STAT_LOG_TARGET_AGE(mediumQueries, STAT_OSE_AI_Sense_Sight_OldestAgeTarget_Medium);
   }
   {
      SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_QualityBucket_Low)
      TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAISense_Sight::_HandleTimeSlicingForBucket_Low)
      
      const int traceCountBefore = traceCount;      
      _HandleTimeSlicingForBucket(
         world, 
         listenersMap, 
         TATAIPerceptionSightSenseCVars::LowBucketMaxSightFrameTime, 
         TATAIPerceptionSightSenseCVars::LowBucketMaxProcessedQueriesPerFrame,
         traceCount,
         MaxTracesPerTick,
         lowQueries);
      SET_DWORD_STAT(STAT_OSE_AI_Sense_Sight_TracesPerFrame_Low, traceCount - traceCountBefore);
      STAT_LOG_TARGET_AGE(lowQueries, STAT_OSE_AI_Sense_Sight_OldestAgeTarget_Low);
   }
   
   SET_DWORD_STAT(STAT_OSE_AI_Sense_Sight_TracesPerFrame, traceCount);
   return 0.f;
}

void UOSEAISense_Sight::_HandleSwapOrRemoveOperations(
   TArray<FQueryOperation>& queryOperations,
   FOSEAISightBucket& bucket
   )
{
   if (queryOperations.Num() > 0)
   {
      // Sort by InRange and by descending Index 
      queryOperations.Sort([](const FQueryOperation& LHS, const FQueryOperation& RHS)->bool
      {
         if (LHS.IsInRange != RHS.IsInRange)
            return LHS.IsInRange;
         return LHS.Index > RHS.Index;
      });
      // Do all the removes first and save the out of range swaps because we will insert them at the right location to prevent sorting
      TArray<FOSEAISightQuery> sightQueriesOutOfRangeToInsert;
      for (const FQueryOperation& operation : queryOperations)
      {
         if (operation.OpType == EOperationType::SwapList)
         {
            if (operation.IsInRange)
            {
               sightQueriesOutOfRangeToInsert.Push(bucket.SightQueriesInRange[operation.Index]);
            }
            else
            {
               bucket.SightQueriesInRange.Add(bucket.SightQueriesOutOfRange[operation.Index]);
            }
         }

         if (operation.IsInRange)
         {
            // In range queries are always sorted at the beginning of the update
            bucket.SightQueriesInRange.RemoveAtSwap(operation.Index, EAllowShrinking::No);
         }
         else
         {
            // Preserve the list ordered
            bucket.SightQueriesOutOfRange.RemoveAt(operation.Index, EAllowShrinking::No);
            if (operation.Index < bucket.NextOutOfRangeIndex)
            {
               bucket.NextOutOfRangeIndex--;
            }
         }
      }
      
      // Reinsert the saved out of range swaps
      if (sightQueriesOutOfRangeToInsert.Num() > 0)
      {
         bucket.SightQueriesOutOfRange.Insert(sightQueriesOutOfRangeToInsert.GetData(), sightQueriesOutOfRangeToInsert.Num(), bucket.NextOutOfRangeIndex);
         bucket.NextOutOfRangeIndex += sightQueriesOutOfRangeToInsert.Num();
      }
   }

}

void UOSEAISense_Sight::_HandleInvalidTargets(
   TArray<FOSEAISightTarget::FTargetId>& invalidTargets,
   FTargetsContainer& observedTargets)
{
   if (invalidTargets.Num() > 0)
   {
      // this should not be happening since UAIPerceptionSystem::OnPerceptionStimuliSourceEndPlay introduction
      UE_VLOG(GetPerceptionSystem(), LogAIPerception, Error, TEXT("Invalid sight targets found during UOSEAISense_Sight::Update call"));

      for (const auto& targetId : invalidTargets)
      {
         // remove affected queries
         _RemoveAllQueriesToTarget(targetId);
         // remove target itself
         observedTargets.Remove(targetId);
      }

      // remove holes
      observedTargets.Compact();
   }
}

float UOSEAISense_Sight::Update()
{
   if (TATAIPerceptionSightSenseCVars::IsSightSenseQualityScalingBucketEnabled)
   {
      return _HandleQualityScalingSightUpdate();
   }
   return _HandleStandardSightUpdate();
}

void UOSEAISense_Sight::RegisterEvent(const FOSEAISightEvent& event)
{

}

void UOSEAISense_Sight::RegisterSource(AActor& sourceActor)
{
   _RegisterTarget(sourceActor);
}

void UOSEAISense_Sight::UnregisterSource(AActor& sourceActor)
{
   const FOSEAISightTarget::FTargetId asTargetId = sourceActor.GetUniqueID();
   FOSEAISightTarget asTarget;

   if (ObservedTargets.RemoveAndCopyValue(asTargetId, asTarget))
   {
      if (AActor* targetActor = asTarget.Target.Get())
      {
         // notify all interested observers that this source is no longer
         // visible      
         AIPerception::FListenerMap& listenersMap = *GetListeners();
         auto removeQuery = [this, &listenersMap, &asTargetId, &targetActor](TArray<FOSEAISightQuery>& sightQueries, const int32 queryIndex)->EReverseForEachResult
         {
            FOSEAISightQuery* sightQuery = &sightQueries[queryIndex];
            if (sightQuery->TargetId == asTargetId)
            {
               if (sightQuery->bLastResult == true)
               {
                  FPerceptionListener& listener = listenersMap[sightQuery->ObserverId];
                  ensure(listener.Listener.IsValid());

                  listener.RegisterStimulus(targetActor, FAIStimulus(*this, 0.f, sightQuery->LastSeenLocation, listener.CachedLocation, FAIStimulus::SensingFailed));
               }

               sightQueries.RemoveAtSwap(queryIndex, EAllowShrinking::No);
               return EReverseForEachResult::Modified;
            }
            return EReverseForEachResult::UnTouched;
         };
         
         for (int i = EAISightBucket::High; i < EAISightBucket::Count; ++i)
         {
            FOSEAISightBucket& bucket = SightQueryBuckets[static_cast<EAISightBucket>(i)];

            ReverseForEach(bucket.SightQueriesInRange, removeQuery);
            if (ReverseForEach(bucket.SightQueriesOutOfRange, removeQuery) == EReverseForEachResult::Modified)
            {
               bucket.bSightQueriesOutOfRangeDirty = true;
            }
         }
      }
   }
}

void UOSEAISense_Sight::_CreateQueriesForTargetAndListener(const AActor& targetActor,
                                                           const TFunction<void(FOSEAISightQuery&)>& onAddedFunc,
                                                           const FOSEAISightTarget::FTargetId& targetID,
                                                           bool& newQueriesAdded,
                                                           const FVector& targetLocation,
                                                           const FPerceptionListener& listener,
                                                           const IGenericTeamAgentInterface* listenersTeamAgent,
                                                           const FDigestedSightProperties& propDigest,
                                                           const EAISightBucket& bucketToUse)
{
   if (FAISenseAffiliationFilter::ShouldSenseTeam(listenersTeamAgent, targetActor, propDigest.AffiliationFlags))
   {
      // calculate max sight radius
      float sightRadiusSq = propDigest.SightRadiusSq;
      _ModifySightRadius(listener.Listener.Get(), propDigest, sightRadiusSq);
      _EnforceMinimumSightSquareRadius(propDigest, sightRadiusSq);

      // create a sight query      
      const float importance = _CalcQueryImportance(listener, targetLocation, sightRadiusSq);
      const bool inRange = importance > 0.0f;
          
      FOSEAISightBucket& bucket = SightQueryBuckets[bucketToUse];
      if (inRange == false)
      {
         bucket.bSightQueriesOutOfRangeDirty = true;
      }
      FOSEAISightQuery& addedQuery = inRange ? bucket.SightQueriesInRange.AddDefaulted_GetRef() : bucket.SightQueriesOutOfRange.AddDefaulted_GetRef();
      addedQuery.ObserverId = listener.GetListenerID();
      addedQuery.TargetId = targetID;
      addedQuery.Importance = importance;
      if (onAddedFunc)
      {
         onAddedFunc(addedQuery);
      }
      newQueriesAdded = true;
   }
}

EAISightBucket GetSightBucketForTargetActor(const AActor* targetActor)
{
   if (targetActor && targetActor->Implements<UOSEAITargetSightInterface>())
   {
      return IOSEAITargetSightInterface::Execute_GetBucketForTarget(targetActor);
   }
   return EAISightBucket::Low;
}

bool UOSEAISense_Sight::_RegisterTarget(AActor& targetActor, const TFunction<void(FOSEAISightQuery&)>& onAddedFunc /*= nullptr*/)
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_RegisterTarget);

   FOSEAISightTarget* sightTarget = ObservedTargets.Find(targetActor.GetUniqueID());

   if (sightTarget != nullptr && sightTarget->GetTargetActor() != &targetActor)
   {
      // this means given unique ID has already been recycled. 
      FOSEAISightTarget newSightTarget(&targetActor);

      sightTarget = &(ObservedTargets.Add(newSightTarget.TargetId, newSightTarget));
      sightTarget->SightTargetInterface = Cast<IAISightTargetInterface>(&targetActor);
   }
   else if (sightTarget == nullptr)
   {
      FOSEAISightTarget newSightTarget(&targetActor);

      sightTarget = &(ObservedTargets.Add(newSightTarget.TargetId, newSightTarget));
      sightTarget->SightTargetInterface = Cast<IAISightTargetInterface>(&targetActor);
   }

   // set/update data
   sightTarget->TeamId = FGenericTeamId::GetTeamIdentifier(&targetActor);

   // generate all pairs and add them to current Sight Queries
   bool newQueriesAdded = false;
   const AIPerception::FListenerMap& listenersMap = *GetListeners();
   const FVector targetLocation = targetActor.GetActorLocation();
   EAISightBucket sightBucket = GetSightBucketForTargetActor(&targetActor);
   
   for (AIPerception::FListenerMap::TConstIterator listenerIt(listenersMap); listenerIt; ++listenerIt)
   {
      const FPerceptionListener& listener = listenerIt->Value;
      const IGenericTeamAgentInterface* listenersTeamAgent = listener.GetTeamAgent();

      if (listener.HasSense(GetSenseID()) && listener.GetBodyActor() != &targetActor)
      {
         const FDigestedSightProperties& propDigest = DigestedProperties[listener.GetListenerID()];
         _CreateQueriesForTargetAndListener(
            targetActor, 
            onAddedFunc, 
            targetActor.GetUniqueID(), 
            newQueriesAdded, 
            targetLocation,
            listener,
            listenersTeamAgent, 
            propDigest, 
            sightBucket);
         
         // TEMP: Until we deprecate the old sight queries
         _CreateQueriesForTargetAndListener(
           targetActor, 
           onAddedFunc, 
           targetActor.GetUniqueID(), 
           newQueriesAdded, 
           targetLocation,
           listener,
           listenersTeamAgent, 
           propDigest, 
           EAISightBucket::NonQualityControlledBucket);
         // END TEMP
      }
   }

   // sort Sight Queries
   if (newQueriesAdded)
   {
      RequestImmediateUpdate();
   }

   return newQueriesAdded;
}

static EAlertnessLevel GetAlertnessLevelForActor(const AActor* actor)
{
   EAlertnessLevel alertnessLevel = EAlertnessLevel::Neutral;
   if (const auto* alertnessInterface = Cast<const IOSEAlertnessInterface>(actor))
   {
      alertnessLevel = alertnessInterface->GetAlertnessLevel();
   }
   else if (actor)
   {
      UE_LOG(LogAIPerception, Warning, TEXT("Actor '%s' has an AI sight perception component, but does not implement IOSEAlertnessInterface"),
         *actor->GetFullName());
   }

   return alertnessLevel;
}

const FDigestedSightProperties& UOSEAISense_Sight::_SetupDigestedPropertiesForListener(const UAIPerceptionComponent& perceptionComponent)
{
   const UOSEAISenseConfig_Sight* senseConfig = Cast<const UOSEAISenseConfig_Sight>(perceptionComponent.GetSenseConfig(GetSenseID()));
   check(senseConfig);

   const EAlertnessLevel alertnessLevel = GetAlertnessLevelForActor(perceptionComponent.GetBodyActor());
   const FOSEPerAlertLevelSettings& settings = senseConfig->GetSettingsForAlertLevel(alertnessLevel);
   
   FDigestedSightProperties& propertyDigest = DigestedProperties.FindOrAdd(perceptionComponent.GetListenerId());
   propertyDigest.SightInterface = Cast<IOSEAISightInterface>(perceptionComponent.GetBodyActor());
   propertyDigest.SightRadiusSq = FMath::Square(settings.SightRadius + settings.PointOfViewBackwardOffset);
   propertyDigest.LoseSightRadiusSq = FMath::Square(settings.LoseSightRadius + settings.PointOfViewBackwardOffset);
   propertyDigest.MinimumSightRadiusSq = FMath::Square(settings.MinimumSightRadius + settings.PointOfViewBackwardOffset);
   propertyDigest.HalfFOVInDegrees = settings.PeripheralVisionAngleDegrees;
   propertyDigest.PointOfViewBackwardOffset = settings.PointOfViewBackwardOffset;
   propertyDigest.NearClip = settings.NearClippingRadius;
   propertyDigest.FarClip = settings.SightRadius;
   propertyDigest.LoseSightFarClip = settings.LoseSightRadius;
   propertyDigest.MinSightFarClip = settings.MinimumSightRadius;
   propertyDigest.FrustumPitch = settings.FrustumPitch;
   propertyDigest.FrustumAspectRatio = settings.FrustumAspectRatio;
   
   if (senseConfig->AutoSuccessRangeFromLastSeenLocation == FAISystem::InvalidRange)
   {
      propertyDigest.AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
   }
   else
   {
      propertyDigest.AutoSuccessRangeSqFromLastSeenLocation = FMath::Square(senseConfig->AutoSuccessRangeFromLastSeenLocation);
   }
   
   propertyDigest.SharedConfigData = senseConfig->SharedConfigData;
   propertyDigest.AffiliationFlags = senseConfig->DetectionByAffiliation.GetAsFlags();
   return propertyDigest;
}

void UOSEAISense_Sight::_OnNewListenerImpl(const FPerceptionListener& newListener)
{
   const UAIPerceptionComponent* newListenerPtr = newListener.Listener.Get();
   check(newListenerPtr);
   _GenerateQueriesForListener(newListener, _SetupDigestedPropertiesForListener(*newListenerPtr));
}

void UOSEAISense_Sight::_GenerateQueriesForListener(const FPerceptionListener& listener,
                                                    const FDigestedSightProperties& propertyDigest,
                                                    const TFunction<void(FOSEAISightQuery&)>& onAddedFunc/*= nullptr */)
{
   bool newQueriesAdded = false;
   const IGenericTeamAgentInterface* listenersTeamAgent = listener.GetTeamAgent();
   const AActor* avatar = listener.GetBodyActor();

   // create sight queries with all legal targets
   for (FTargetsContainer::TConstIterator targetIt(ObservedTargets); targetIt; ++targetIt)
   {
      const AActor* targetActor = targetIt->Value.GetTargetActor();
      if (targetActor == nullptr || targetActor == avatar)
      {
         continue;
      }
      EAISightBucket sightBucket = GetSightBucketForTargetActor(targetActor);
      _CreateQueriesForTargetAndListener(
         *targetActor,
         onAddedFunc,
         targetIt->Key,
         newQueriesAdded,
         targetIt->Value.GetLocationSimple(),
         listener,
         listenersTeamAgent,
         propertyDigest,
         sightBucket
      );
      
      // TEMP: Until we deprecate the old sight queries
      _CreateQueriesForTargetAndListener(
         *targetActor,
         onAddedFunc,
         targetIt->Key,
         newQueriesAdded,
         targetIt->Value.GetLocationSimple(),
         listener,
         listenersTeamAgent,
         propertyDigest,
         EAISightBucket::NonQualityControlledBucket
      );
      // END TEMP
   }

   // sort Sight Queries
   if (newQueriesAdded)
   {
      RequestImmediateUpdate();
   }
}

void UOSEAISense_Sight::_OnListenerUpdateImpl(const FPerceptionListener& updatedListener)
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_ListenerUpdate);

   // first, naive implementation:
   // 1. remove all queries by this listener
   // 2. proceed as if it was a new listener

   // see if this listener is a Target as well
   const FOSEAISightTarget::FTargetId asTargetId = updatedListener.GetBodyActorUniqueID();
   FOSEAISightTarget* asTarget = ObservedTargets.Find(asTargetId);
   if (asTarget != nullptr)
   {
      if (asTarget->Target.IsValid())
      {
         // if still a valid target then backup list of observers for which the listener was visible to restore in the newly created queries
         TSet<FPerceptionListenerID> lastVisibleObservers;
         _RemoveAllQueriesToTarget(asTargetId, [&lastVisibleObservers](const FOSEAISightQuery& query)
         {
            if (query.bLastResult)
            {
               lastVisibleObservers.Add(query.ObserverId);
            }
         });

         _RegisterTarget(*(asTarget->Target.Get()), [&lastVisibleObservers](FOSEAISightQuery& query)
         {
            query.bLastResult = lastVisibleObservers.Contains(query.ObserverId);
         });
      }
      else
      {
         _RemoveAllQueriesToTarget(asTargetId);
      }
   }

   const FPerceptionListenerID listenerID = updatedListener.GetListenerID();

   if (updatedListener.HasSense(GetSenseID()))
   {
      // if still a valid sense then backup list of targets that were visible by the listener to restore in the newly created queries
      TSet<FOSEAISightTarget::FTargetId> lastVisibleTargets;
      _RemoveAllQueriesByListener(updatedListener, [&lastVisibleTargets](const FOSEAISightQuery& query)
      {
         if (query.bLastResult)
         {
            lastVisibleTargets.Add(query.TargetId);
         }
      });
      
      const UAIPerceptionComponent* listenerPtr = updatedListener.Listener.Get();
      check(listenerPtr);
      const FDigestedSightProperties& propertiesDigest = _SetupDigestedPropertiesForListener(*listenerPtr);

      _GenerateQueriesForListener(updatedListener, propertiesDigest, [&lastVisibleTargets](FOSEAISightQuery& query)
      {
         query.bLastResult = lastVisibleTargets.Contains(query.TargetId);
      });
   }
   else
   {
      // remove all queries
      _RemoveAllQueriesByListener(updatedListener);

      DigestedProperties.Remove(listenerID);
   }
}

void UOSEAISense_Sight::OnListenerConfigUpdated(const FPerceptionListener& updatedListener)
{
   bool skipListenerUpdate = false;
   const FPerceptionListenerID listenerID = updatedListener.GetListenerID();

   const UAIPerceptionComponent* listenerPtr = updatedListener.Listener.Get();
   check(listenerPtr);
   _SetupDigestedPropertiesForListener(*listenerPtr);
   
   if (!skipListenerUpdate)
   {
      Super::OnListenerConfigUpdated(updatedListener);
   }
}


void UOSEAISense_Sight::_OnListenerRemovedImpl(const FPerceptionListener& removedListener)
{
   _RemoveAllQueriesByListener(removedListener);

   DigestedProperties.FindAndRemoveChecked(removedListener.GetListenerID());

   // note: there use to be code to remove all queries _to_ listener here as well
   // but that was wrong - the fact that a listener gets unregistered doesn't have to
   // mean it's being removed from the game altogether.
}

void UOSEAISense_Sight::_RemoveAllQueriesByListener(const FPerceptionListener& listener,
                                                    const TFunction<void(const FOSEAISightQuery&)>& onRemoveFunc/*= nullptr */)
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_RemoveByListener);
   const uint32 listenerId = listener.GetListenerID();

   auto removeQuery = [&listenerId, &onRemoveFunc](TArray<FOSEAISightQuery>& sightQueries, const int32 queryIndex)->EReverseForEachResult
   {
      const FOSEAISightQuery& sightQuery = sightQueries[queryIndex];

      if (sightQuery.ObserverId == listenerId)
      {
         if (onRemoveFunc)
         {
            onRemoveFunc(sightQuery);
         }
         sightQueries.RemoveAtSwap(queryIndex, EAllowShrinking::No);
         return EReverseForEachResult::Modified;
      }
      return EReverseForEachResult::UnTouched;
   };
   
   for (int i = EAISightBucket::High; i < EAISightBucket::Count; ++i)
   {
      auto& bucket = SightQueryBuckets[static_cast<EAISightBucket>(i)];
      ReverseForEach(bucket.SightQueriesInRange, removeQuery);
      if (ReverseForEach(bucket.SightQueriesOutOfRange, removeQuery) == EReverseForEachResult::Modified)
      {
         bucket.bSightQueriesOutOfRangeDirty = true;
      }
   }
}

void UOSEAISense_Sight::_RemoveAllQueriesToTarget(const FOSEAISightTarget::FTargetId& targetId,
                                                  const TFunction<void(const FOSEAISightQuery&)>& onRemoveFunc/*= nullptr */)
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_Sense_Sight_RemoveToTarget);

   auto removeQuery = [&targetId, &onRemoveFunc](TArray<FOSEAISightQuery>& sightQueries, const int32 queryIndex)->EReverseForEachResult
   {
      const FOSEAISightQuery& sightQuery = sightQueries[queryIndex];

      if (sightQuery.TargetId == targetId)
      {
         if (onRemoveFunc)
         {
            onRemoveFunc(sightQuery);
         }
         sightQueries.RemoveAtSwap(queryIndex, EAllowShrinking::No);
         return EReverseForEachResult::Modified;
      }
      return EReverseForEachResult::UnTouched;
   };
   for (int i = EAISightBucket::High; i < EAISightBucket::Count; ++i)
   {
      FOSEAISightBucket& bucket = SightQueryBuckets[static_cast<EAISightBucket>(i)];
      ReverseForEach(bucket.SightQueriesInRange, removeQuery);
      if (ReverseForEach(bucket.SightQueriesOutOfRange, removeQuery) == EReverseForEachResult::Modified)
      {
         bucket.bSightQueriesOutOfRangeDirty = true;
      }
   }
}

void UOSEAISense_Sight::OnListenerForgetsActor(const FPerceptionListener& listener, AActor& actorToForget)
{
   const uint32 listenerId = listener.GetListenerID();
   const uint32 targetId = actorToForget.GetUniqueID();

   auto forgetPreviousResult = [&listenerId, &targetId](FOSEAISightQuery& sightQuery)->EForEachResult
   {
      if (sightQuery.ObserverId == listenerId && sightQuery.TargetId == targetId)
      {
         // assuming one query per observer-target pair
         sightQuery.ForgetPreviousResult();
         return EForEachResult::Break;
      }
      return EForEachResult::Continue;
   };
   for (int i = EAISightBucket::High; i < EAISightBucket::Count; ++i)
   {
      FOSEAISightBucket& bucket = SightQueryBuckets[static_cast<EAISightBucket>(i)];
      if (ForEach(bucket.SightQueriesInRange, forgetPreviousResult) == EForEachResult::Continue)
      {
         ForEach(bucket.SightQueriesOutOfRange, forgetPreviousResult);
      }
   }
}

void UOSEAISense_Sight::OnListenerForgetsAll(const FPerceptionListener& listener)
{
   const uint32 listenerId = listener.GetListenerID();

   auto forgetPreviousResult = [&listenerId](FOSEAISightQuery& sightQuery)->EForEachResult
   {
      if (sightQuery.ObserverId == listenerId)
      {
         sightQuery.ForgetPreviousResult();
      }
      return EForEachResult::Continue;
   };
   for (int i = EAISightBucket::High; i < EAISightBucket::Count; ++i)
   {
      FOSEAISightBucket& bucket = SightQueryBuckets[static_cast<EAISightBucket>(i)];
      ForEach(bucket.SightQueriesInRange, forgetPreviousResult);
      ForEach(bucket.SightQueriesOutOfRange, forgetPreviousResult);
   }
}

