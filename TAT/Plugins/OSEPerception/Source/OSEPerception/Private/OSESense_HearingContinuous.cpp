// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_HearingContinuous.h"

//OSE
#include "OSESenseConfig_HearingContinuous.h"
#include "OSEPerceptionComponent.h"
#include "OSESoundSourceComponent.h"
#include "OSESenseEvent_HearingContinuous.h"

//UE
#include "CollisionQueryParams.h"
#include "AIHelpers.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_HearingContinuous)

#define AISENSE_Hearing_TIMESLICING_DEBUG 0
#define DO_Hearing_VLOGGING (0 && ENABLE_VISUAL_LOG)

#if DO_Hearing_VLOGGING
	#define Hearing_LOG_SEGMENT(LogOwner, SegmentStart, SegmentEnd, Color, Format, ...) UE_VLOG_SEGMENT(LogOwner, LogOSEPerception, Verbose, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__)
	#define Hearing_LOG_LOCATION(LogOwner, Location, Radius, Color, Format, ...) UE_VLOG_LOCATION(LogOwner, LogOSEPerception, Verbose, Location, Radius, Color, Format, ##__VA_ARGS__)
#else
	#define Hearing_LOG_SEGMENT(...)
	#define Hearing_LOG_LOCATION(...)
#endif // DO_Hearing_VLOGGING

DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing"),STAT_OSE_Sense_HearingContinuous,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Update Sort"),STAT_OSE_Sense_HearingContinuous_UpdateSort,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Compute visibility"),STAT_OSE_Sense_HearingContinuous_ComputeVisibility,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Query operations"),STAT_OSE_Sense_HearingContinuous_QueryOperations,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Listener Update"), STAT_OSE_Sense_HearingContinuous_ListenerUpdate, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Register Target"), STAT_OSE_Sense_HearingContinuous_RegisterTarget, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Remove By Listener"), STAT_OSE_Sense_HearingContinuous_RemoveByListener, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Remove To Target"), STAT_OSE_Sense_HearingContinuous_RemoveToTarget, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Hearing, Process pending result"), STAT_OSE_Sense_HearingContinuous_ProcessPendingQuery, STATGROUP_AI);

static const int32 DefaultMaxTracesPerTick = 6;
static const int32 DefaultMinQueriesPerTimeSliceCheck = 40;

enum class EForEachResult : uint8
{
	Break,
	Continue,
};

template <typename T, class PREDICATE_CLASS>
EForEachResult ForEach(T& Array, const PREDICATE_CLASS& Predicate)
{
	for (typename T::ElementType& Element : Array)
	{
		if (Predicate(Element) == EForEachResult::Break)
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
EReverseForEachResult ReverseForEach(T& Array, const PREDICATE_CLASS& Predicate)
{
	EReverseForEachResult RetVal = EReverseForEachResult::UnTouched;
	for (int32 Index = Array.Num()-1; Index >= 0; --Index)
	{
		if (Predicate(Array, Index) == EReverseForEachResult::Modified)
		{
			RetVal = EReverseForEachResult::Modified;
		}
	}
	return RetVal;
}

//----------------------------------------------------------------------//
// FOSEHearingTarget
//----------------------------------------------------------------------//
const FOSEHearingTarget::FTargetId FOSEHearingTarget::InvalidTargetId = FAISystem::InvalidUnsignedID;

FOSEHearingTarget::FOSEHearingTarget(AActor* InTarget, FGenericTeamId InTeamId, float InIntensity)
	: Target(InTarget), TeamId(InTeamId), Intensity(InIntensity)
{
	if (InTarget)
	{
		TargetId = InTarget->GetUniqueID();
	}
	else
	{
		TargetId = InvalidTargetId;
	}
}

//----------------------------------------------------------------------//
// FDigestedProperties
//----------------------------------------------------------------------//

UOSESense_HearingContinuous::FDigestedProperties::FDigestedProperties()
   : MaxRadiusSq(-1.f)
   , HearingRadiusSq(-1.f)
   , LoseHearingRadiusSq(-1.f)
   , SoundMaskingPercent(-1.f)
   , MinErrorRadius(0.f)
   , MaxErrorRadius(0.f)
   , AffiliationFlags(-1)
{

}

UOSESense_HearingContinuous::FDigestedProperties::FDigestedProperties(const UOSESenseConfig_HearingContinuous& senseConfig)
{
   HearingRadiusSq = FMath::Square(senseConfig.HearingRadius);
   LoseHearingRadiusSq = FMath::Square(senseConfig.LoseHearingRadius);
   MaxRadiusSq = FMath::Square(LoseHearingRadiusSq);
   SoundMaskingPercent = senseConfig.PercentMaskingSound;
   MinErrorRadius = senseConfig.MinErrorRadius;
   MaxErrorRadius = senseConfig.MaxErrorRadius;
   AffiliationFlags = senseConfig.DetectionByAffiliation.GetAsFlags();
}

//----------------------------------------------------------------------//
// UOSESense_HearingContinuous
//----------------------------------------------------------------------//
UOSESense_HearingContinuous::UOSESense_HearingContinuous(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MaxTracesPerTick(DefaultMaxTracesPerTick)
	, MinQueriesPerTimeSliceCheck(DefaultMinQueriesPerTimeSliceCheck)
	, MaxTimeSlicePerTick(0.005) // 5ms
	, HighImportanceQueryDistanceThreshold(300.f)
	, MaxQueryImportance(60.f)
	, HearingLimitQueryImportance(10.f)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		OnNewListenerDelegate.BindUObject(this, &UOSESense_HearingContinuous::OnNewListenerImpl);
		OnListenerUpdateDelegate.BindUObject(this, &UOSESense_HearingContinuous::OnListenerUpdateImpl);
		OnListenerRemovedDelegate.BindUObject(this, &UOSESense_HearingContinuous::OnListenerRemovedImpl);
	}

	NotifyType = EOSESenseNotifyType::OnPerceptionChange;
	
	bNeedsForgettingNotification = true;

}

FORCEINLINE_DEBUGGABLE float UOSESense_HearingContinuous::_CalcQueryImportance(const FOSEPerceptionListener& Listener, const FVector& TargetLocation, const float HearingRadiusSq) const
{
	const float DistanceSq = FVector::DistSquared(Listener.CachedLocation, TargetLocation);
	return DistanceSq <= HighImportanceDistanceSquare ? MaxQueryImportance
		: FMath::Clamp((HearingLimitQueryImportance - MaxQueryImportance) / HearingRadiusSq * DistanceSq + MaxQueryImportance, 0.f, MaxQueryImportance);
}

void UOSESense_HearingContinuous::PostInitProperties()
{
	Super::PostInitProperties();
	HighImportanceDistanceSquare = FMath::Square(HighImportanceQueryDistanceThreshold);
}

bool UOSESense_HearingContinuous::_ShouldAutomaticallyHearTarget(const FDigestedProperties* propDigest, FOSEHearingQuery* HearingQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const
{
	return false;
}
bool UOSESense_HearingContinuous::_ShouldNeverHearTarget(const FDigestedProperties* propDigest, FOSEHearingQuery* HearingQuery, FOSEPerceptionListener& Listener, AActor* TargetActor) const
{
   return false;
}

#if AISENSE_Hearing_TIMESLICING_DEBUG
namespace 
{
	struct FTimingSlicingInfo
	{
		FTimingSlicingInfo() 
		{
			Start(); 
		}

		double StartTime = 0.;
		double EndTime = 0.;

		int32 InRangeCount = 0;
		int32 OutOfRangeCount = 0;

		float InRangeAgeSum = 0.f;
		float OutOfRangeAgeSum = 0.f;

		void Start() { StartTime = FPlatformTime::Seconds();}
		void Stop() { EndTime = FPlatformTime::Seconds();}

		void PushQueryInfo(const bool bIsInRange, const float Age)
		{
			if (bIsInRange)
			{
				++InRangeCount;
				InRangeAgeSum += Age;
			}
			else
			{
				++OutOfRangeCount;
				OutOfRangeAgeSum += Age;
			}
		}

		FString ToString() const
		{
			FString Info = FString::Format(TEXT("in {0} seconds"), {EndTime - StartTime});
			if (InRangeCount > 0)
			{
				Info.Append(FString::Format(TEXT("[{0} InRange Age:{1}]"), {InRangeCount, InRangeAgeSum/InRangeCount}));
			}
			if (OutOfRangeCount > 0)
			{
				Info.Append(FString::Format(TEXT("[{0} OutOfRange Age:{1}]"), {OutOfRangeCount, OutOfRangeAgeSum/OutOfRangeCount}));
			}
			return Info;
		}
	};
}
#endif // AISENSE_Hearing_TIMESLICING_DEBUG


/* 
*  we have three pieces of state information:
*     1. is this query in range?
*     2. is this query masked by a louder sound?
*     3. is this query blocked by another physical characteristic?
* 
* If the sound is out of range, we don't care about anything else.
* If the sound is in range, but masked, we don't care about anything else
* If the sound is in range, and isn't masked, we care if is is heard. 
* 
* 
* Update who is in range - this could be done lazily
* 
* If a sound enters or leaves range, we now need to know if it is a masking sound. 
* Sounds are masked relative to other sounds for a specific listener. To figure out who is masked/masking
* we need to process the sounds for a given listener in order for most intense to least intense.
*/

float UOSESense_HearingContinuous::Update()
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous);

	const UWorld* World = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter(), EGetWorldErrorMode::LogAndReturnNull);

	if (World == nullptr)
	{
		return SuspendNextUpdate;
	}

   SortQueriesForUpdate();


	int32 TracesCount = 0;
	int32 NumQueriesProcessed = 0;
	double TimeSliceEnd = FPlatformTime::Seconds() + MaxTimeSlicePerTick;
	bool bHitTimeSliceLimit = false;
#if AISENSE_Hearing_TIMESLICING_DEBUG
	FTimingSlicingInfo SlicingInfo;
#endif // AISENSE_Hearing_TIMESLICING_DEBUG
	static const int32 InitialInvalidItemsSize = 16;
	enum class EOperationType : uint8
	{
		Remove,
		SwapList
	};
	struct FQueryOperation
	{
		FQueryOperation(bool bInInRange, EOperationType InOpType, int32 InIndex) : bInRange(bInInRange), OpType(InOpType), Index(InIndex) {}
		bool bInRange;
		EOperationType OpType;
		int32 Index;
	};
	TArray<FQueryOperation> QueryOperations;
	TArray<FOSEHearingTarget::FTargetId> InvalidTargets;
	QueryOperations.Reserve(InitialInvalidItemsSize);
	InvalidTargets.Reserve(InitialInvalidItemsSize);

	OSEPerception::FListenerMap& ListenersMap = *GetListeners();
   TSet<FOSEPerceptionListenerID> DirtyListeners;

   TArray<FOSEHearingQuery*> ExpiredEventQueries;
	int32 InRangeItr = 0;
	int32 OutOfRangeItr = 0;
	for (int32 QueryIndex = 0; QueryIndex < HearingQueriesInRange.Num() + HearingQueriesOutOfRange.Num(); ++QueryIndex)
	{
		// Time slice limit check - spread out checks to every N queries so we don't spend more time checking timer than doing work
		NumQueriesProcessed++;
		if ((NumQueriesProcessed % MinQueriesPerTimeSliceCheck) == 0 && FPlatformTime::Seconds() > TimeSliceEnd)
		{
			bHitTimeSliceLimit = true;
		}

		if (bHitTimeSliceLimit || TracesCount >= MaxTracesPerTick)
		{
			break;
		}

		// Calculate next in range query
		int32 InRangeIndex = HearingQueriesInRange.IsValidIndex(InRangeItr) ? InRangeItr : INDEX_NONE;
		FOSEHearingQuery* InRangeQuery = InRangeIndex != INDEX_NONE ? &HearingQueriesInRange[InRangeIndex] : nullptr;

		// Calculate next out of range query
		int32 OutOfRangeIndex = HearingQueriesOutOfRange.IsValidIndex(OutOfRangeItr) ? (NextOutOfRangeIndex + OutOfRangeItr) % HearingQueriesOutOfRange.Num() : INDEX_NONE;
		FOSEHearingQuery* OutOfRangeQuery = OutOfRangeIndex != INDEX_NONE ? &HearingQueriesOutOfRange[OutOfRangeIndex] : nullptr;
		if (OutOfRangeQuery)
		{
			OutOfRangeQuery->RecalcScore();
		}

		// Compare to real find next query
		const bool bIsInRangeQuery = (InRangeQuery && OutOfRangeQuery) ? FOSEHearingQuery::FSortPredicate()(*InRangeQuery,*OutOfRangeQuery) : !OutOfRangeQuery;
		FOSEHearingQuery* HearingQuery = bIsInRangeQuery ? InRangeQuery : OutOfRangeQuery;
		ensure(HearingQuery);

#if AISENSE_Hearing_TIMESLICING_DEBUG
		SlicingInfo.PushQueryInfo(bIsInRangeQuery, HearingQuery->GetAge());
#endif //AISENSE_Hearing_TIMESLICING_DEBUG

		bIsInRangeQuery ? ++InRangeItr : ++OutOfRangeItr;

		FOSEPerceptionListener& listener = ListenersMap[HearingQuery->ObserverId];
		FOSEHearingTarget& Target = ObservedTargets[HearingQuery->TargetId];
     
		AActor* targetActor = Target.Target.Get();
		UOSEPerceptionComponent* ListenerPtr = listener.Listener.Get();
		ensure(ListenerPtr);

		// @todo figure out what should we do if not valid
		if (targetActor && ListenerPtr)
		{

     
         const FDigestedProperties* PropDigest = GetDigestedProperties(HearingQuery->ObserverId);
         const AActor* ListenerBodyActor = ListenerPtr->GetBodyActor();
         float StimulusStrength = 1.f;
         FVector SeenLocation(0.f);
         int32 NumberOfLoSChecksPerformed = 0;
         FVector TargetLocation = Target.GetLocationSimple();

         if (HearingQuery->bEvent)
         {
            TargetLocation = HearingQuery->EventFiredLocation;
         }

         bool bIsPropagated = ComputeHearibility(World, *HearingQuery, listener, ListenerBodyActor, Target, targetActor, PropDigest, StimulusStrength, SeenLocation, NumberOfLoSChecksPerformed);

         TracesCount += NumberOfLoSChecksPerformed;

         const bool bWasPropagated = HearingQuery->bLastPropagated;
         float radiusSq = bWasPropagated ? PropDigest->LoseHearingRadiusSq : PropDigest->HearingRadiusSq;
         HearingQuery->Importance = _CalcQueryImportance(listener, TargetLocation, radiusSq);
         const bool bShouldBeInRange = HearingQuery->Importance > 0.0f;

         float currentGameTime = World->GetTimeSeconds();
         if (HearingQuery->bEvent && ((currentGameTime - HearingQuery->EventCreationGameTime) > HearingQuery->EventDuration))
         {
            //if this has timed out, we don't hear it anymore.
            bIsPropagated = false;
            ExpiredEventQueries.Add(HearingQuery);
         }
         HearingQuery->bLastPropagated = bIsPropagated;

         if (bIsInRangeQuery != bShouldBeInRange)
         {
            //UpdateQueryVisibilityStatus(*HearingQuery, listener, false, TargetLocation, HearingQuery->Intensity, targetActor);
            QueryOperations.Add(FQueryOperation(bIsInRangeQuery, EOperationType::SwapList, bIsInRangeQuery ? InRangeIndex : OutOfRangeIndex));
         }
         else if (bIsInRangeQuery && bWasPropagated != bIsPropagated)
         {
            DirtyListeners.Add(HearingQuery->ObserverId);
         }
         else if (bIsInRangeQuery && bIsPropagated && HearingQuery->bLastResult)
         {
            //this could get stomped if we have a new sound that masks, but since the sound is continuous, we need to update it.

            FVector stimulusLocation = _CalcStimLocation(PropDigest, *HearingQuery, TargetLocation);
            UpdateQueryVisibilityStatus(*HearingQuery, listener, true, stimulusLocation, HearingQuery->Intensity, targetActor);
         }
         // restart query
         HearingQuery->OnProcessed();
      }
      else
      {
         // put this index to "to be removed" array
         QueryOperations.Add(FQueryOperation(bIsInRangeQuery, EOperationType::Remove, bIsInRangeQuery ? InRangeIndex : OutOfRangeIndex));
         if (targetActor == nullptr)
         {
            InvalidTargets.AddUnique(HearingQuery->TargetId);
         }
      }
	}
	NextOutOfRangeIndex = HearingQueriesOutOfRange.Num() > 0 ? (NextOutOfRangeIndex + OutOfRangeItr) % HearingQueriesOutOfRange.Num() : 0;

#if AISENSE_Hearing_TIMESLICING_DEBUG
	SlicingInfo.Stop();
	UE_LOG(LogOSEPerception, VeryVerbose, TEXT("UOSESense_HearingContinuous::Update processed %d sources %s [time slice limited? %d]"), NumQueriesProcessed, *SlicingInfo.ToString(), bHitTimeSliceLimit ? 1 : 0);
#else
	UE_LOG(LogOSEPerception, VeryVerbose, TEXT("UOSESense_HearingContinuous::Update processed %d sources [time slice limited? %d]"), NumQueriesProcessed, bHitTimeSliceLimit ? 1 : 0);
#endif // AISENSE_Hearing_TIMESLICING_DEBUG

	if (QueryOperations.Num() > 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_QueryOperations);

		// Sort by InRange and by descending Index 
		QueryOperations.Sort([](const FQueryOperation& LHS, const FQueryOperation& RHS)->bool
		{
			if (LHS.bInRange != RHS.bInRange)
				return LHS.bInRange;
			return LHS.Index > RHS.Index;
		});
        // Do all the removes first and save the out of range swaps because we will insert them at the right location to prevent sorting
		TArray<FOSEHearingQuery> HearingQueriesOutOfRangeToInsert;
		for (FQueryOperation& Operation : QueryOperations)
		{
			if (Operation.OpType == EOperationType::SwapList)
			{
				if (Operation.bInRange)
				{
					HearingQueriesOutOfRangeToInsert.Push(HearingQueriesInRange[Operation.Index]);
               DirtyListeners.Add(HearingQueriesInRange[Operation.Index].ObserverId);
				}
				else
				{
					HearingQueriesInRange.Add(HearingQueriesOutOfRange[Operation.Index]);
               DirtyListeners.Add(HearingQueriesOutOfRange[Operation.Index].ObserverId);
				}
			}

			if (Operation.bInRange)
			{
            DirtyListeners.Add(HearingQueriesInRange[Operation.Index].ObserverId);
				// In range queries are always sorted at the beginning of the update
				HearingQueriesInRange.RemoveAtSwap(Operation.Index, 1, EAllowShrinking::No);
			}
			else
			{
				// Preserve the list ordered
				HearingQueriesOutOfRange.RemoveAt(Operation.Index, 1, EAllowShrinking::No);
				if (Operation.Index < NextOutOfRangeIndex)
				{
					NextOutOfRangeIndex--;
				}
			}
		}
        // Reinsert the saved out of range swaps
		if (HearingQueriesOutOfRangeToInsert.Num() > 0)
		{
			HearingQueriesOutOfRange.Insert(HearingQueriesOutOfRangeToInsert.GetData(), HearingQueriesOutOfRangeToInsert.Num(), NextOutOfRangeIndex);
			NextOutOfRangeIndex += HearingQueriesOutOfRangeToInsert.Num();
		}

		if (InvalidTargets.Num() > 0)
		{
			// this should not be happening since UOSEPerceptionSystem::OnPerceptionStimuliSourceEndPlay introduction
			UE_VLOG(GetPerceptionSystem(), LogOSEPerception, Error, TEXT("Invalid Hearing targets found during UOSESense_HearingContinuous::Update call"));

			for (const auto& TargetId : InvalidTargets)
			{
				// remove affected queries
				RemoveAllQueriesToTarget(TargetId);
				// remove target itself
				ObservedTargets.Remove(TargetId);
			}

			// remove holes
			ObservedTargets.Compact();
		}
	}

   for (FOSEPerceptionListenerID listenerId : DirtyListeners)
   {
      TArray<FOSEHearingQuery*> SortedQueries;
      for (FOSEHearingQuery& Query : HearingQueriesInRange)
      {
         if(Query.ObserverId == listenerId)
         { 
            SortedQueries.Add(&Query);
         }
      }
      SortedQueries.Sort([](FOSEHearingQuery& LHS, FOSEHearingQuery& RHS)->bool
      {
         return LHS.Intensity > RHS.Intensity;
      });

      FOSEPerceptionListener& listener = ListenersMap[listenerId];
      const FDigestedProperties* PropDigest = GetDigestedProperties(listenerId);
      float maxIntensity = 0.f;

      for (FOSEHearingQuery* Query : SortedQueries)
      {
         if (Query->bLastPropagated)
         {
            maxIntensity = FMath::Max(maxIntensity, Query->Intensity);
         }

         const bool bMasked = maxIntensity * PropDigest->SoundMaskingPercent >= Query->Intensity;
         const bool bPerceived = !bMasked && Query->bLastPropagated;
         FOSEHearingTarget& Target = ObservedTargets[Query->TargetId];
         AActor* targetActor = Target.Target.Get();
         
         FVector TargetLocation = (Query->bEvent) ? Query->EventFiredLocation : Target.GetLocationSimple();
         FVector StimLocation = _CalcStimLocation(PropDigest, *Query, TargetLocation);
         UpdateQueryVisibilityStatus(*Query, listener, bPerceived, StimLocation, Query->Intensity, targetActor);
      }
   }

   if (ExpiredEventQueries.Num() > 0)
   {
      _RemoveExpiredQueries(ExpiredEventQueries);
   }
	//return HearingQueries.Num() > 0 ? 1.f/6 : FLT_MAX;
	return 0.f;
}

void UOSESense_HearingContinuous::_RemoveExpiredQueries(const TArray<FOSEHearingQuery*>& ExpiredEventQueries)
{
   const UWorld* World = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter(), EGetWorldErrorMode::LogAndReturnNull);
   if (!IsValid(World))
   {
      return;
   }

   //Remove old event queries
   TArray<AActor*> targetsToRemove;
   for (int HearingQueriesInRangeIdx = HearingQueriesInRange.Num() - 1; HearingQueriesInRangeIdx >= 0; --HearingQueriesInRangeIdx)
   {
      FOSEHearingQuery* currentQuery = &HearingQueriesInRange[HearingQueriesInRangeIdx];
      float currentGameTime = World->GetTimeSeconds();

      for (FOSEHearingQuery* currentExpiredQuery : ExpiredEventQueries)
      {
         if (currentExpiredQuery && currentQuery && currentExpiredQuery == currentQuery)
         {
            targetsToRemove.AddUnique(ObservedTargets[currentQuery->TargetId].Target.Get());
            HearingQueriesInRange.RemoveAt(HearingQueriesInRangeIdx);
            break;
         }
      }
   }

   for (int HearingQueriesOutOfRangeIdx = HearingQueriesOutOfRange.Num() - 1; HearingQueriesOutOfRangeIdx >= 0; --HearingQueriesOutOfRangeIdx)
   {
      FOSEHearingQuery* currentQuery = &HearingQueriesOutOfRange[HearingQueriesOutOfRangeIdx];
      float currentGameTime = World->GetTimeSeconds();

      for (FOSEHearingQuery* currentExpiredQuery : ExpiredEventQueries)
      {
         if (currentExpiredQuery && currentQuery && currentExpiredQuery == currentQuery)
         {
            targetsToRemove.AddUnique(ObservedTargets[currentQuery->TargetId].Target.Get());
            HearingQueriesOutOfRange.RemoveAt(HearingQueriesOutOfRangeIdx);
            break;
         }
      }
   }

   //We dont want to remove targets that still have event queries
   for (AActor* currentActor : targetsToRemove)
   {
      bool bFoundActor = false;
      for (FOSEHearingQuery& currentQuery : HearingQueriesInRange)
      {
         if (ObservedTargets[currentQuery.TargetId].Target.Get() == currentActor)
         {
            bFoundActor = true;
            break;
         }
      }

      if (bFoundActor)
      {
         continue;
      }

      for (FOSEHearingQuery& currentQuery : HearingQueriesOutOfRange)
      {
         if (ObservedTargets[currentQuery.TargetId].Target.Get() == currentActor)
         {
            bFoundActor = true;
            break;
         }
      }

      if (!bFoundActor)
      {
         UnregisterSource(*currentActor);
      }
   }
}

FVector UOSESense_HearingContinuous::_CalcStimLocation(const FDigestedProperties* propDigest, const FOSEHearingQuery& HearingQuery, const FVector& TargetLocation)
{
   float errorRadius = FMath::GetMappedRangeValueClamped(FVector2D(0, MaxQueryImportance), FVector2D(propDigest->MaxErrorRadius, propDigest->MinErrorRadius), HearingQuery.Importance);
   
   return errorRadius > 0 ? FVector(FMath::RandPointInCircle(errorRadius), 0.f ) + TargetLocation : TargetLocation;

}

void UOSESense_HearingContinuous::SortQueriesForUpdate()
{
   // sort Hearing Queries
   {
      auto RecalcScore = [](FOSEHearingQuery& HearingQuery)->EForEachResult
         {
            HearingQuery.RecalcScore();
            return EForEachResult::Continue;
         };

      SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_UpdateSort);
      // Sort out of range queries
      if (bHearingQueriesOutOfRangeDirty)
      {
         ForEach(HearingQueriesOutOfRange, RecalcScore);
         HearingQueriesOutOfRange.Sort(FOSEHearingQuery::FSortPredicate());
         NextOutOfRangeIndex = 0;
         bHearingQueriesOutOfRangeDirty = false;
      }

      // Sort in range queries
      ForEach(HearingQueriesInRange, RecalcScore);
      HearingQueriesInRange.Sort(FOSEHearingQuery::FSortPredicate());
   }
}

const UOSESense_HearingContinuous::FDigestedProperties* UOSESense_HearingContinuous::GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const
{
   const FDigestedProperties& PropDigest = DigestedProperties[ListenerID];
   return &PropDigest;
}

void UOSESense_HearingContinuous::ReportSoundEvent(UObject* WorldContextObject, FVector NoiseLocation, float Loudness /*= 1.f*/, AActor* Instigator /*= nullptr*/, float Duration /*= 0.f*/)
{

	UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(WorldContextObject);
	if (PerceptionSystem)
	{
		FOSEHearingEvent Event(Instigator, NoiseLocation, Loudness, Duration);
		PerceptionSystem->OnEvent(Event);
	}
}

bool UOSESense_HearingContinuous::ComputeHearibility(const UWorld* World, FOSEHearingQuery& HearingQuery, FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSEHearingTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, float& OutStimulusStrength, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed) const
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_ComputeVisibility);

   if (_ShouldNeverHearTarget(PropDigest, &HearingQuery, Listener, TargetActor))
   {
      OutSeenLocation = FAISystem::InvalidLocation;
      return false;
   }

	// @Note that automagical "seeing" does not care about Hearing range nor vision cone
	if (_ShouldAutomaticallyHearTarget(PropDigest, &HearingQuery, Listener, TargetActor, OutStimulusStrength))
	{
		OutSeenLocation = FAISystem::InvalidLocation;
		return true;
	}

   if (!IsTargetInHearingVolume(Listener, ListenerActor, Target, TargetActor, PropDigest, HearingQuery.bLastResult))
	{
		return false;
	}
   
   OutSeenLocation = Target.GetLocationSimple();

	return true;
}

bool UOSESense_HearingContinuous::IsTargetInHearingVolume(FOSEPerceptionListener& listener, const AActor* listenerActor, FOSEHearingTarget& target, AActor* targetActor, const FDigestedProperties* propDigest, bool bLastResult) const
{

   FVector bodyLocation, bodyFacing;
   listener.Listener->GetLocationAndDirection(bodyLocation, bodyFacing);
   const FVector rootLocation = bodyLocation;

   FVector targetLocation;
   FVector targetExtents;
   const bool onlyCollidingComponents = true;
   targetActor->GetActorBounds(onlyCollidingComponents, targetLocation, targetExtents);
   const FBox targetBounds = FBox::BuildAABB(targetLocation, targetExtents);

   bool bInVolume = false;
   if (!bLastResult)
   {
      const FVector sightCenter = rootLocation;
      const FSphere sightSphere(sightCenter, propDigest->HearingRadiusSq);

      bInVolume = FMath::SphereAABBIntersection(sightSphere, targetBounds);
   }
   else
   {
      const FVector loseSightCenter = rootLocation;
      const FSphere loseSightSphere(loseSightCenter, propDigest->LoseHearingRadiusSq);
      bInVolume = FMath::SphereAABBIntersection(loseSightSphere, targetBounds);
   }

   return bInVolume;
}

void UOSESense_HearingContinuous::UpdateQueryVisibilityStatus(FOSEHearingQuery& HearingQuery, FOSEPerceptionListener& Listener, const bool bIsVisible, const FVector& SeenLocation, const float StimulusStrength, AActor* TargetActor) const
{
	if (bIsVisible)
	{
		const bool bHasValidSeenLocation = SeenLocation != FAISystem::InvalidLocation;
		Listener.RegisterStimulus(TargetActor, FOSEStimulus(*this, StimulusStrength, bHasValidSeenLocation ? SeenLocation : HearingQuery.LastSeenLocation, Listener.CachedLocation));
		HearingQuery.bLastResult = true;
		if (bHasValidSeenLocation)
		{
			HearingQuery.LastSeenLocation = SeenLocation;
		}
	}
	// communicate failure only if we've seen given actor before
	else if (HearingQuery.bLastResult == true)
	{
		Listener.RegisterStimulus(TargetActor, FOSEStimulus(*this, 0.f, HearingQuery.LastSeenLocation, Listener.CachedLocation, FOSEStimulus::SensingFailed));
		HearingQuery.bLastResult = false;
		HearingQuery.LastSeenLocation = FAISystem::InvalidLocation;
	}

	Hearing_LOG_SEGMENT(ListenerPtr->GetOwner(), Listener.CachedLocation, HearingQuery.LastSeenLocation, bIsVisible ? FColor::Green : FColor::Red, TEXT("TargetID %d"), Target.TargetId);
}

void UOSESense_HearingContinuous::RegisterEvent(const FOSEHearingEvent& Event)
{
   if(Event.Instigator)
   {
      RegisterTarget(*Event.Instigator, Event.NoiseLocation, nullptr, true, Event.Duration, Event.BaseStimulusStrength);
   }
}

void UOSESense_HearingContinuous::RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent)
{
   UOSESenseEvent_HearingContinuous* hearingSenseEvent = Cast<UOSESenseEvent_HearingContinuous>(&PerceptionEvent);

   if (!IsValid(hearingSenseEvent))
   {
      return;
   }

   if(hearingSenseEvent->Event.Instigator)
   {
      RegisterTarget(*hearingSenseEvent->Event.Instigator, hearingSenseEvent->Event.NoiseLocation, nullptr, true, hearingSenseEvent->Event.Duration, hearingSenseEvent->Event.BaseStimulusStrength);
   }
   
}

void UOSESense_HearingContinuous::RegisterSource(AActor& SourceActor)
{
   const FVector TargetLocation = SourceActor.GetActorLocation();
	RegisterTarget(SourceActor, TargetLocation);
}

void UOSESense_HearingContinuous::UnregisterSource(AActor& SourceActor)
{
	const FOSEHearingTarget::FTargetId AsTargetId = SourceActor.GetUniqueID();
	FOSEHearingTarget AsTarget;
	
	if (ObservedTargets.RemoveAndCopyValue(AsTargetId, AsTarget) 
		&& (HearingQueriesInRange.Num() + HearingQueriesOutOfRange.Num()) > 0)
	{
		AActor* TargetActor = AsTarget.Target.Get();

		if (TargetActor)
		{
			// notify all interested observers that this source is no longer
			// visible		
			OSEPerception::FListenerMap& ListenersMap = *GetListeners();
			auto RemoveQuery = [this,&ListenersMap,&AsTargetId,&TargetActor](TArray<FOSEHearingQuery>& HearingQueries, const int32 QueryIndex)->EReverseForEachResult
			{
				FOSEHearingQuery* HearingQuery = &HearingQueries[QueryIndex];
				if (HearingQuery->TargetId == AsTargetId)
				{
					if (HearingQuery->bLastResult == true)
					{
						FOSEPerceptionListener& Listener = ListenersMap[HearingQuery->ObserverId];
						ensure(Listener.Listener.IsValid());

						Listener.RegisterStimulus(TargetActor, FOSEStimulus(*this, 0.f, HearingQuery->LastSeenLocation, Listener.CachedLocation, FOSEStimulus::SensingFailed));
					}

					HearingQueries.RemoveAtSwap(QueryIndex, 1, EAllowShrinking::No);
					return EReverseForEachResult::Modified;
				}
				return EReverseForEachResult::UnTouched;
			};
			ReverseForEach(HearingQueriesInRange, RemoveQuery);
			if (ReverseForEach(HearingQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
			{
				bHearingQueriesOutOfRangeDirty = true;
			}
		}
	}
}

bool UOSESense_HearingContinuous::RegisterTarget(AActor& TargetActor, const FVector& TargetLocation, const TFunction<void(FOSEHearingQuery&)>& OnAddedFunc /*= nullptr*/, bool bEvent /*= false*/, float EventDuration /*= 0.0f*/, float EventBaseStimulusStrength /*= 0.0f*/)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_RegisterTarget);
	
	FOSEHearingTarget* HearingTarget = ObservedTargets.Find(TargetActor.GetUniqueID());
	
	if (HearingTarget != nullptr && HearingTarget->GetTargetActor() != &TargetActor)
	{
		// this means given unique ID has already been recycled. 
		FOSEHearingTarget NewHearingTarget(&TargetActor);

		HearingTarget = &(ObservedTargets.Add(NewHearingTarget.TargetId, NewHearingTarget));
	}
	else if (HearingTarget == nullptr)
	{
		FOSEHearingTarget NewHearingTarget(&TargetActor);

		HearingTarget = &(ObservedTargets.Add(NewHearingTarget.TargetId, NewHearingTarget));
	}

	// set/update data
	HearingTarget->TeamId = FGenericTeamId::GetTeamIdentifier(&TargetActor);

   if (bEvent)
   {
      HearingTarget->Intensity = EventBaseStimulusStrength;
   }
   else if (UOSESoundSourceComponent* source = TargetActor.GetComponentByClass<UOSESoundSourceComponent>())
   {
      HearingTarget->Intensity = source->BaseStimulusStrength * source->StimulusStrengthScalar;
   }
	
	// generate all pairs and add them to current Hearing Queries
	bool bNewQueriesAdded = false;
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();
	

	for (OSEPerception::FListenerMap::TConstIterator ItListener(ListenersMap); ItListener; ++ItListener)
	{
		const FOSEPerceptionListener& Listener = ItListener->Value;
		const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();

		if (Listener.HasSense(GetSenseID()) && Listener.GetBodyActor() != &TargetActor)
		{
			const FDigestedProperties* PropDigest = GetDigestedProperties(Listener.GetListenerID());
			if (FOSESenseAffiliationFilter::ShouldSenseTeam(ListenersTeamAgent, TargetActor, PropDigest->AffiliationFlags))
			{
				// create a Hearing query		
				const float Importance = _CalcQueryImportance(ItListener->Value, TargetLocation, PropDigest->HearingRadiusSq);
				const bool bInRange = Importance > 0.0f;
				if (!bInRange)
				{
					bHearingQueriesOutOfRangeDirty = true;
				}
				FOSEHearingQuery& AddedQuery = bInRange ? HearingQueriesInRange.AddDefaulted_GetRef() : HearingQueriesOutOfRange.AddDefaulted_GetRef();
				AddedQuery.ObserverId = ItListener->Key;
				AddedQuery.TargetId = HearingTarget->TargetId;
            AddedQuery.Intensity = HearingTarget->Intensity;
				AddedQuery.Importance = Importance;

            if (bEvent)
            {
               const UWorld* World = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter(), EGetWorldErrorMode::LogAndReturnNull);
               AddedQuery.bEvent = true;
               AddedQuery.EventCreationGameTime = World->GetTimeSeconds();
               AddedQuery.EventDuration = EventDuration;
               AddedQuery.EventFiredLocation = TargetLocation;
            }
				
				if (OnAddedFunc)
				{
					OnAddedFunc(AddedQuery);
				}
				bNewQueriesAdded = true;
			}
		}
	}

	// sort Hearing Queries
	if (bNewQueriesAdded)
	{
		RequestImmediateUpdate();
	}

	return bNewQueriesAdded;
}

void UOSESense_HearingContinuous::OnNewListenerImpl(const FOSEPerceptionListener& NewListener)
{
   _NewListenerImpl(NewListener);
}

void UOSESense_HearingContinuous::_NewListenerImpl(const FOSEPerceptionListener& NewListener)
{
   UOSEPerceptionComponent* NewListenerPtr = NewListener.Listener.Get();
   check(NewListenerPtr);
   const UOSESenseConfig_HearingContinuous* SenseConfig = Cast<const UOSESenseConfig_HearingContinuous>(NewListenerPtr->GetSenseConfig(GetSenseID()));
   check(SenseConfig);
   const FDigestedProperties PropertyDigest(*SenseConfig);
   DigestedProperties.Add(NewListener.GetListenerID(), PropertyDigest);

   const FDigestedProperties* PropertiesDigest = GetDigestedProperties(NewListener.GetListenerID());
   GenerateQueriesForListener(NewListener, PropertiesDigest);
}


void UOSESense_HearingContinuous::GenerateQueriesForListener(const FOSEPerceptionListener& Listener, const FDigestedProperties* PropertyDigest, const TFunction<void(FOSEHearingQuery&)>& OnAddedFunc/*= nullptr */)
{
	bool bNewQueriesAdded = false;
	const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();
	const AActor* Avatar = Listener.GetBodyActor();

	// create Hearing queries with all legal targets
	for (FTargetsContainer::TConstIterator ItTarget(ObservedTargets); ItTarget; ++ItTarget)
	{
		const AActor* TargetActor = ItTarget->Value.GetTargetActor();
		if (TargetActor == NULL || TargetActor == Avatar)
		{
			continue;
		}

		if (FOSESenseAffiliationFilter::ShouldSenseTeam(ListenersTeamAgent, *TargetActor, PropertyDigest->AffiliationFlags))
		{
			// create a Hearing query		
			const float Importance = _CalcQueryImportance(Listener, ItTarget->Value.GetLocationSimple(), PropertyDigest->MaxRadiusSq);
			const bool bInRange = Importance > 0.0f;
			if (!bInRange)
			{
				bHearingQueriesOutOfRangeDirty = true;
			}
			FOSEHearingQuery& AddedQuery = bInRange ? HearingQueriesInRange.AddDefaulted_GetRef() : HearingQueriesOutOfRange.AddDefaulted_GetRef();
			AddedQuery.ObserverId = Listener.GetListenerID();
			AddedQuery.TargetId = ItTarget->Key;
         AddedQuery.Intensity = ItTarget->Value.Intensity;
			AddedQuery.Importance = Importance;

			if (OnAddedFunc)
			{
				OnAddedFunc(AddedQuery);
			}
			bNewQueriesAdded = true;
		}
	}

	// sort Hearing Queries
	if (bNewQueriesAdded)
	{
		RequestImmediateUpdate();
	}
}
void UOSESense_HearingContinuous::OnListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
   _ListenerUpdateImpl(UpdatedListener);
}

void UOSESense_HearingContinuous::_ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_ListenerUpdate);

	// first, naive implementation:
	// 1. remove all queries by this listener
	// 2. proceed as if it was a new listener

	// see if this listener is a Target as well
	const FOSEHearingTarget::FTargetId AsTargetId = UpdatedListener.GetBodyActorUniqueID();
	FOSEHearingTarget* AsTarget = ObservedTargets.Find(AsTargetId);
	if (AsTarget != NULL)
	{
		if (AsTarget->Target.IsValid())
		{
			// if still a valid target then backup list of observers for which the listener was visible to restore in the newly created queries
			TSet<FOSEPerceptionListenerID> LastVisibleObservers;
			RemoveAllQueriesToTarget(AsTargetId, [&LastVisibleObservers](const FOSEHearingQuery& Query)
			{
				if (Query.bLastResult)
				{
					LastVisibleObservers.Add(Query.ObserverId);
				}
			});

			RegisterTarget(*(AsTarget->Target.Get()),AsTarget->Target.Get()->GetActorLocation(), [&LastVisibleObservers](FOSEHearingQuery& Query)
			{
				Query.bLastResult = LastVisibleObservers.Contains(Query.ObserverId);
			});
		}
		else
		{
			RemoveAllQueriesToTarget(AsTargetId);
		}
	}

	const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

	if (UpdatedListener.HasSense(GetSenseID()))
	{
		// if still a valid sense then backup list of targets that were visible by the listener to restore in the newly created queries
		TSet<FOSEHearingTarget::FTargetId> LastVisibleTargets;
		RemoveAllQueriesByListener(UpdatedListener, [&LastVisibleTargets](const FOSEHearingQuery& Query)
		{
			if (Query.bLastResult)
			{
				LastVisibleTargets.Add(Query.TargetId);
			}			
		});

		const FDigestedProperties* PropertiesDigest = GetDigestedProperties(ListenerID);

		GenerateQueriesForListener(UpdatedListener, PropertiesDigest, [&LastVisibleTargets](FOSEHearingQuery& Query)
		{
			Query.bLastResult = LastVisibleTargets.Contains(Query.TargetId);
		});
	}
	else
	{
		// remove all queries
		RemoveAllQueriesByListener(UpdatedListener);
	}
}

void UOSESense_HearingContinuous::OnListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   _ListenerRemovedImpl(RemovedListener);
}

void UOSESense_HearingContinuous::_ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   RemoveAllQueriesByListener(RemovedListener);
}

void UOSESense_HearingContinuous::RemoveAllQueriesByListener(const FOSEPerceptionListener& Listener, const TFunction<void(const FOSEHearingQuery&)>& OnRemoveFunc/*= nullptr */)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_RemoveByListener);

	if ((HearingQueriesInRange.Num() + HearingQueriesOutOfRange.Num()) == 0)
	{
		return;
	}

	const uint32 ListenerId = Listener.GetListenerID();
	
	auto RemoveQuery = [&ListenerId, &OnRemoveFunc](TArray<FOSEHearingQuery>& HearingQueries, const int32 QueryIndex)->EReverseForEachResult
	{
		const FOSEHearingQuery& HearingQuery = HearingQueries[QueryIndex];

		if (HearingQuery.ObserverId == ListenerId)
		{
			if (OnRemoveFunc)
			{
				OnRemoveFunc(HearingQuery);
			}
			HearingQueries.RemoveAtSwap(QueryIndex, 1, EAllowShrinking::No);
			return EReverseForEachResult::Modified;
		}
		return EReverseForEachResult::UnTouched;
	};
	ReverseForEach(HearingQueriesInRange, RemoveQuery);
	if(ReverseForEach(HearingQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
	{
		bHearingQueriesOutOfRangeDirty = true;
	}
}

void UOSESense_HearingContinuous::RemoveAllQueriesToTarget(const FOSEHearingTarget::FTargetId& TargetId, const TFunction<void(const FOSEHearingQuery&)>& OnRemoveFunc/*= nullptr */)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_HearingContinuous_RemoveToTarget);

	auto RemoveQuery = [&TargetId, &OnRemoveFunc](TArray<FOSEHearingQuery>& HearingQueries, const int32 QueryIndex)->EReverseForEachResult
	{
		const FOSEHearingQuery& HearingQuery = HearingQueries[QueryIndex];

		if (HearingQuery.TargetId == TargetId)
		{
			if (OnRemoveFunc)
			{
				OnRemoveFunc(HearingQuery);
			}
			HearingQueries.RemoveAtSwap(QueryIndex, 1, EAllowShrinking::No);
			return EReverseForEachResult::Modified;
		}
		return EReverseForEachResult::UnTouched;
	};
	ReverseForEach(HearingQueriesInRange, RemoveQuery);
	if (ReverseForEach(HearingQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
	{
		bHearingQueriesOutOfRangeDirty = true;
	}
}

void UOSESense_HearingContinuous::OnListenerForgetsActor(const FOSEPerceptionListener& Listener, AActor& ActorToForget)
{
	const uint32 ListenerId = Listener.GetListenerID();
	const uint32 TargetId = ActorToForget.GetUniqueID();
	
	auto ForgetPreviousResult = [&ListenerId, &TargetId](FOSEHearingQuery& HearingQuery)->EForEachResult
	{
		if (HearingQuery.ObserverId == ListenerId && HearingQuery.TargetId == TargetId)
		{
			// assuming one query per observer-target pair
			HearingQuery.ForgetPreviousResult();
			return EForEachResult::Break;
		}
		return EForEachResult::Continue;
	};

	if (ForEach(HearingQueriesInRange, ForgetPreviousResult) == EForEachResult::Continue)
	{
		ForEach(HearingQueriesOutOfRange, ForgetPreviousResult);
	}
}

void UOSESense_HearingContinuous::OnListenerForgetsAll(const FOSEPerceptionListener& Listener)
{
	const uint32 ListenerId = Listener.GetListenerID();

	auto ForgetPreviousResult = [&ListenerId](FOSEHearingQuery& HearingQuery)->EForEachResult
	{
		if (HearingQuery.ObserverId == ListenerId)
		{
			HearingQuery.ForgetPreviousResult();
		}
		return EForEachResult::Continue;
	};

	ForEach(HearingQueriesInRange, ForgetPreviousResult);
	ForEach(HearingQueriesOutOfRange, ForgetPreviousResult);
}
