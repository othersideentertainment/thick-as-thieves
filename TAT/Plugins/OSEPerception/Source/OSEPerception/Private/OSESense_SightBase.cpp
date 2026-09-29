// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_SightBase.h"

//OSE
#include "OSESightTargetInterface.h"
#include "OSESenseConfig_Sight.h"
#include "OSEPerceptionComponent.h"

//UE
#include "CollisionQueryParams.h"
#include "AIHelpers.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_SightBase)

#define AISENSE_SIGHT_TIMESLICING_DEBUG 0
#define DO_SIGHT_VLOGGING (0 && ENABLE_VISUAL_LOG)

#if DO_SIGHT_VLOGGING
	#define SIGHT_LOG_SEGMENT(LogOwner, SegmentStart, SegmentEnd, Color, Format, ...) UE_VLOG_SEGMENT(LogOwner, LogOSEPerception, Verbose, SegmentStart, SegmentEnd, Color, Format, ##__VA_ARGS__)
	#define SIGHT_LOG_LOCATION(LogOwner, Location, Radius, Color, Format, ...) UE_VLOG_LOCATION(LogOwner, LogOSEPerception, Verbose, Location, Radius, Color, Format, ##__VA_ARGS__)
#else
	#define SIGHT_LOG_SEGMENT(...)
	#define SIGHT_LOG_LOCATION(...)
#endif // DO_SIGHT_VLOGGING

DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight"),STAT_OSE_Sense_SightBase,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Update Sort"),STAT_OSE_Sense_SightBase_UpdateSort,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Compute visibility"),STAT_OSE_Sense_SightBase_ComputeVisibility,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Query operations"),STAT_OSE_Sense_SightBase_QueryOperations,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Listener Update"), STAT_OSE_Sense_SightBase_ListenerUpdate, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Register Target"), STAT_OSE_Sense_SightBase_RegisterTarget, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Remove By Listener"), STAT_OSE_Sense_SightBase_RemoveByListener, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Remove To Target"), STAT_OSE_Sense_SightBase_RemoveToTarget, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Process pending result"), STAT_OSE_Sense_SightBase_ProcessPendingQuery, STATGROUP_AI);


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
// FOSESightTarget
//----------------------------------------------------------------------//
const FOSESightTarget::FTargetId FOSESightTarget::InvalidTargetId = FAISystem::InvalidUnsignedID;

FOSESightTarget::FOSESightTarget(AActor* InTarget, FGenericTeamId InTeamId)
	: Target(InTarget), SightTargetInterface(NULL), TeamId(InTeamId)
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

UOSESense_SightBase::FDigestedProperties::FDigestedProperties()
   : AffiliationFlags(-1)
   , MaxRadiusSq(-1.f)
{

}

//----------------------------------------------------------------------//
// UOSESense_SightBase
//----------------------------------------------------------------------//
UOSESense_SightBase::UOSESense_SightBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MaxTracesPerTick(DefaultMaxTracesPerTick)
	, MinQueriesPerTimeSliceCheck(DefaultMinQueriesPerTimeSliceCheck)
	, MaxTimeSlicePerTick(0.005) // 5ms
	, HighImportanceQueryDistanceThreshold(300.f)
	, MaxQueryImportance(60.f)
	, SightLimitQueryImportance(10.f)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		OnNewListenerDelegate.BindUObject(this, &UOSESense_SightBase::OnNewListenerImpl);
		OnListenerUpdateDelegate.BindUObject(this, &UOSESense_SightBase::OnListenerUpdateImpl);
		OnListenerRemovedDelegate.BindUObject(this, &UOSESense_SightBase::OnListenerRemovedImpl);
	}

	NotifyType = EOSESenseNotifyType::OnPerceptionChange;
	
	bNeedsForgettingNotification = true;

	DefaultSightCollisionChannel = GET_AI_CONFIG_VAR(DefaultSightCollisionChannel);
}

FORCEINLINE_DEBUGGABLE float UOSESense_SightBase::_CalcQueryImportance(const FOSEPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq) const
{
	const float DistanceSq = FVector::DistSquared(Listener.CachedLocation, TargetLocation);
	return DistanceSq <= HighImportanceDistanceSquare ? MaxQueryImportance
		: FMath::Clamp((SightLimitQueryImportance - MaxQueryImportance) / SightRadiusSq * DistanceSq + MaxQueryImportance, 0.f, MaxQueryImportance);
}

void UOSESense_SightBase::PostInitProperties()
{
	Super::PostInitProperties();
	HighImportanceDistanceSquare = FMath::Square(HighImportanceQueryDistanceThreshold);
}

bool UOSESense_SightBase::_ShouldAutomaticallySeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const
{
	return false;
}
bool UOSESense_SightBase::_ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& Listener, AActor* TargetActor) const
{
   return false;
}

bool UOSESense_SightBase::_ShouldAutoSucceedLOS(const FDigestedProperties* propDigest, const AActor* ListenerActor, AActor* targetActor) const
{
   return false;
}

#if AISENSE_SIGHT_TIMESLICING_DEBUG
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
#endif // AISENSE_SIGHT_TIMESLICING_DEBUG

float UOSESense_SightBase::Update()
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase);

	const UWorld* World = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter(), EGetWorldErrorMode::LogAndReturnNull);

	if (World == nullptr)
	{
		return SuspendNextUpdate;
	}

	// sort Sight Queries
	{
		auto RecalcScore = [](FOSESightQuery& sightQuery)->EForEachResult
		{
			sightQuery.RecalcScore();
			return EForEachResult::Continue;
		};

		SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_UpdateSort);
        // Sort out of range queries
    	if (bSightQueriesOutOfRangeDirty)
		{
			ForEach(SightQueriesOutOfRange, RecalcScore);
			SightQueriesOutOfRange.Sort(FOSESightQuery::FSortPredicate());
			NextOutOfRangeIndex = 0;
			bSightQueriesOutOfRangeDirty = false;
		}

        // Sort in range queries
		ForEach(SightQueriesInRange, RecalcScore);
		SightQueriesInRange.Sort(FOSESightQuery::FSortPredicate());
	}

	int32 TracesCount = 0;
	int32 NumQueriesProcessed = 0;
	double TimeSliceEnd = FPlatformTime::Seconds() + MaxTimeSlicePerTick;
	bool bHitTimeSliceLimit = false;
#if AISENSE_SIGHT_TIMESLICING_DEBUG
	FTimingSlicingInfo SlicingInfo;
#endif // AISENSE_SIGHT_TIMESLICING_DEBUG
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
	TArray<FOSESightTarget::FTargetId> InvalidTargets;
	QueryOperations.Reserve(InitialInvalidItemsSize);
	InvalidTargets.Reserve(InitialInvalidItemsSize);

	OSEPerception::FListenerMap& ListenersMap = *GetListeners();

	int32 InRangeItr = 0;
	int32 OutOfRangeItr = 0;
	for (int32 QueryIndex = 0; QueryIndex < SightQueriesInRange.Num() + SightQueriesOutOfRange.Num(); ++QueryIndex)
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
		int32 InRangeIndex = SightQueriesInRange.IsValidIndex(InRangeItr) ? InRangeItr : INDEX_NONE;
		FOSESightQuery* InRangeQuery = InRangeIndex != INDEX_NONE ? &SightQueriesInRange[InRangeIndex] : nullptr;

		// Calculate next out of range query
		int32 OutOfRangeIndex = SightQueriesOutOfRange.IsValidIndex(OutOfRangeItr) ? (NextOutOfRangeIndex + OutOfRangeItr) % SightQueriesOutOfRange.Num() : INDEX_NONE;
		FOSESightQuery* OutOfRangeQuery = OutOfRangeIndex != INDEX_NONE ? &SightQueriesOutOfRange[OutOfRangeIndex] : nullptr;
		if (OutOfRangeQuery)
		{
			OutOfRangeQuery->RecalcScore();
		}

		// Compare to real find next query
		const bool bIsInRangeQuery = (InRangeQuery && OutOfRangeQuery) ? FOSESightQuery::FSortPredicate()(*InRangeQuery,*OutOfRangeQuery) : !OutOfRangeQuery;
		FOSESightQuery* sightQuery = bIsInRangeQuery ? InRangeQuery : OutOfRangeQuery;
		ensure(sightQuery);

#if AISENSE_SIGHT_TIMESLICING_DEBUG
		SlicingInfo.PushQueryInfo(bIsInRangeQuery, sightQuery->GetAge());
#endif //AISENSE_SIGHT_TIMESLICING_DEBUG

		bIsInRangeQuery ? ++InRangeItr : ++OutOfRangeItr;

		FOSEPerceptionListener& listener = ListenersMap[sightQuery->ObserverId];
		FOSESightTarget& Target = ObservedTargets[sightQuery->TargetId];

		AActor* targetActor = Target.Target.Get();
		UOSEPerceptionComponent* ListenerPtr = listener.Listener.Get();
		ensure(ListenerPtr);

		// @todo figure out what should we do if not valid
		if (targetActor && ListenerPtr)
		{
         const FDigestedProperties* PropDigest = GetDigestedProperties(sightQuery->ObserverId);
         const AActor* ListenerBodyActor = ListenerPtr->GetBodyActor();
         float StimulusStrength = 1.f;
         FVector SeenLocation(0.f);
         int32 NumberOfLoSChecksPerformed = 0;

         const bool bIsVisible = ComputeVisibility(World, *sightQuery, listener, ListenerBodyActor, Target, targetActor, PropDigest, StimulusStrength, SeenLocation, NumberOfLoSChecksPerformed);

         TracesCount += NumberOfLoSChecksPerformed;

         const bool bWasVisible = sightQuery->bLastResult;
         const FVector TargetLocation = targetActor->GetActorLocation();
         UpdateQueryVisibilityStatus(*sightQuery, listener, bIsVisible, SeenLocation, StimulusStrength, targetActor, TargetLocation);

         sightQuery->Importance = _CalcQueryImportance(listener, TargetLocation, PropDigest->MaxRadiusSq);
         const bool bShouldBeInRange = sightQuery->Importance > 0.0f;
         if (bIsInRangeQuery != bShouldBeInRange)
         {
            QueryOperations.Add(FQueryOperation(bIsInRangeQuery, EOperationType::SwapList, bIsInRangeQuery ? InRangeIndex : OutOfRangeIndex));
         }

         // restart query
         sightQuery->OnProcessed();
      }
      else
      {
         // put this index to "to be removed" array
         QueryOperations.Add(FQueryOperation(bIsInRangeQuery, EOperationType::Remove, bIsInRangeQuery ? InRangeIndex : OutOfRangeIndex));
         if (targetActor == nullptr)
         {
            InvalidTargets.AddUnique(sightQuery->TargetId);
         }
      }
	}
	NextOutOfRangeIndex = SightQueriesOutOfRange.Num() > 0 ? (NextOutOfRangeIndex + OutOfRangeItr) % SightQueriesOutOfRange.Num() : 0;

#if AISENSE_SIGHT_TIMESLICING_DEBUG
	SlicingInfo.Stop();
	UE_LOG(LogOSEPerception, VeryVerbose, TEXT("UOSESense_SightBase::Update processed %d sources %s [time slice limited? %d]"), NumQueriesProcessed, *SlicingInfo.ToString(), bHitTimeSliceLimit ? 1 : 0);
#else
	UE_LOG(LogOSEPerception, VeryVerbose, TEXT("UOSESense_SightBase::Update processed %d sources [time slice limited? %d]"), NumQueriesProcessed, bHitTimeSliceLimit ? 1 : 0);
#endif // AISENSE_SIGHT_TIMESLICING_DEBUG

	if (QueryOperations.Num() > 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_QueryOperations);

		// Sort by InRange and by descending Index 
		QueryOperations.Sort([](const FQueryOperation& LHS, const FQueryOperation& RHS)->bool
		{
			if (LHS.bInRange != RHS.bInRange)
				return LHS.bInRange;
			return LHS.Index > RHS.Index;
		});
        // Do all the removes first and save the out of range swaps because we will insert them at the right location to prevent sorting
		TArray<FOSESightQuery> SightQueriesOutOfRangeToInsert;
		for (FQueryOperation& Operation : QueryOperations)
		{
			if (Operation.OpType == EOperationType::SwapList)
			{
				if (Operation.bInRange)
				{
					SightQueriesOutOfRangeToInsert.Push(SightQueriesInRange[Operation.Index]);
				}
				else
				{
					SightQueriesInRange.Add(SightQueriesOutOfRange[Operation.Index]);
				}
			}

			if (Operation.bInRange)
			{
				// In range queries are always sorted at the beginning of the update
				SightQueriesInRange.RemoveAtSwap(Operation.Index, 1, EAllowShrinking::No);
			}
			else
			{
				// Preserve the list ordered
				SightQueriesOutOfRange.RemoveAt(Operation.Index, 1, EAllowShrinking::No);
				if (Operation.Index < NextOutOfRangeIndex)
				{
					NextOutOfRangeIndex--;
				}
			}
		}
        // Reinsert the saved out of range swaps
		if (SightQueriesOutOfRangeToInsert.Num() > 0)
		{
			SightQueriesOutOfRange.Insert(SightQueriesOutOfRangeToInsert.GetData(), SightQueriesOutOfRangeToInsert.Num(), NextOutOfRangeIndex);
			NextOutOfRangeIndex += SightQueriesOutOfRangeToInsert.Num();
		}

		if (InvalidTargets.Num() > 0)
		{
			// this should not be happening since UOSEPerceptionSystem::OnPerceptionStimuliSourceEndPlay introduction
			UE_VLOG(GetPerceptionSystem(), LogOSEPerception, Error, TEXT("Invalid sight targets found during UOSESense_SightBase::Update call"));

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

	//return SightQueries.Num() > 0 ? 1.f/6 : FLT_MAX;
	return 0.f;
}

const UOSESense_SightBase::FDigestedProperties* UOSESense_SightBase::GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const
{
   return nullptr;
}

bool UOSESense_SightBase::ComputeVisibility(const UWorld* World, FOSESightQuery& sightQuery, FOSEPerceptionListener& listener, const AActor* listenerActor, FOSESightTarget& target, AActor* targetActor, const FDigestedProperties* PropDigest, float& OutStimulusStrength, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed) const
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_ComputeVisibility);

   if (_ShouldNeverSeeTarget(PropDigest, &sightQuery, listener, targetActor))
   {
      OutSeenLocation = FAISystem::InvalidLocation;
      return false;
   }

   // @Note that automagical "seeing" does not care about sight range nor vision cone
   if (_ShouldAutomaticallySeeTarget(PropDigest, &sightQuery, listener, targetActor, OutStimulusStrength))
   {
      OutSeenLocation = FAISystem::InvalidLocation;
      return true;
   }

   if (!IsTargetInSightVolume(listener, listenerActor, target, targetActor, PropDigest, sightQuery.bLastResult))
   {
      return false;
   }

   if (_ShouldAutoSucceedLOS(PropDigest, listenerActor, targetActor))
   {
      OutSeenLocation = targetActor->GetActorLocation();
      return true;
   }

   if (target.SightTargetInterface != nullptr)
   {
      const bool bWasVisible = sightQuery.bLastResult;
      const bool bCanBeSeen = target.SightTargetInterface->CanBeSeenFrom(listener.CachedLocation, OutSeenLocation, OutNumberOfLoSChecksPerformed, OutStimulusStrength, listenerActor, &bWasVisible, &sightQuery.UserData);
      return bCanBeSeen;
   }
   else
   {
      FVector TargetLocation;
      FRotator TargetRotation;
      targetActor->GetActorEyesViewPoint(TargetLocation, TargetRotation);

      // we need to do tests ourselves
      FHitResult HitResult;
      const bool bHit = World->LineTraceSingleByChannel(HitResult, listener.CachedLocation, TargetLocation, DefaultSightCollisionChannel, FCollisionQueryParams(SCENE_QUERY_STAT(AILineOfSight), false, listenerActor));

      ++OutNumberOfLoSChecksPerformed;

      auto HitResultActorIsOwnedByTargetActor = [&HitResult, targetActor]()
		{
			AActor* HitResultActor = HitResult.HitObjectHandle.FetchActor();
			return (HitResultActor ? HitResultActor->IsOwnedBy(targetActor) : false);
		};

		if (bHit == false || HitResultActorIsOwnedByTargetActor())
		{
			OutSeenLocation = TargetLocation;
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool UOSESense_SightBase::IsTargetInSightVolume(FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSESightTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, bool bLastResult) const
{
   return false;
}

void UOSESense_SightBase::UpdateQueryVisibilityStatus(FOSESightQuery& sightQuery, FOSEPerceptionListener& Listener, const bool bIsVisible, const FVector& SeenLocation, const float StimulusStrength, AActor* TargetActor, const FVector& TargetLocation) const
{
	if (bIsVisible)
	{
		const bool bHasValidSeenLocation = SeenLocation != FAISystem::InvalidLocation;
		Listener.RegisterStimulus(TargetActor, FOSEStimulus(*this, StimulusStrength, bHasValidSeenLocation ? SeenLocation : sightQuery.LastSeenLocation, Listener.CachedLocation));
		sightQuery.bLastResult = true;
		if (bHasValidSeenLocation)
		{
			sightQuery.LastSeenLocation = SeenLocation;
		}
	}
	// communicate failure only if we've seen given actor before
	else if (sightQuery.bLastResult == true)
	{
		Listener.RegisterStimulus(TargetActor, FOSEStimulus(*this, 0.f, TargetLocation, Listener.CachedLocation, FOSEStimulus::SensingFailed));
		sightQuery.bLastResult = false;
		sightQuery.LastSeenLocation = FAISystem::InvalidLocation;
	}

	SIGHT_LOG_SEGMENT(ListenerPtr->GetOwner(), Listener.CachedLocation, TargetLocation, bIsVisible ? FColor::Green : FColor::Red, TEXT("TargetID %d"), Target.TargetId);
}

void UOSESense_SightBase::RegisterEvent(const FOSESightEvent& Event)
{

}

void UOSESense_SightBase::RegisterSource(AActor& SourceActor)
{
	RegisterTarget(SourceActor);
}

void UOSESense_SightBase::UnregisterSource(AActor& SourceActor)
{
	const FOSESightTarget::FTargetId AsTargetId = SourceActor.GetUniqueID();
	FOSESightTarget AsTarget;
	
	if (ObservedTargets.RemoveAndCopyValue(AsTargetId, AsTarget) 
		&& (SightQueriesInRange.Num() + SightQueriesOutOfRange.Num()) > 0)
	{
		AActor* TargetActor = AsTarget.Target.Get();

		if (TargetActor)
		{
			// notify all interested observers that this source is no longer
			// visible		
			OSEPerception::FListenerMap& ListenersMap = *GetListeners();
			auto RemoveQuery = [this,&ListenersMap,&AsTargetId,&TargetActor](TArray<FOSESightQuery>& SightQueries, const int32 QueryIndex)->EReverseForEachResult
			{
				FOSESightQuery* sightQuery = &SightQueries[QueryIndex];
				if (sightQuery->TargetId == AsTargetId)
				{
					if (sightQuery->bLastResult == true)
					{
						FOSEPerceptionListener& Listener = ListenersMap[sightQuery->ObserverId];
						ensure(Listener.Listener.IsValid());

						Listener.RegisterStimulus(TargetActor, FOSEStimulus(*this, 0.f, sightQuery->LastSeenLocation, Listener.CachedLocation, FOSEStimulus::SensingFailed));
					}

					SightQueries.RemoveAtSwap(QueryIndex, 1, EAllowShrinking::No);
					return EReverseForEachResult::Modified;
				}
				return EReverseForEachResult::UnTouched;
			};
			ReverseForEach(SightQueriesInRange, RemoveQuery);
			if (ReverseForEach(SightQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
			{
				bSightQueriesOutOfRangeDirty = true;
			}
		}
	}
}

bool UOSESense_SightBase::RegisterTarget(AActor& TargetActor, const TFunction<void(FOSESightQuery&)>& OnAddedFunc /*= nullptr*/)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_RegisterTarget);
	
	FOSESightTarget* SightTarget = ObservedTargets.Find(TargetActor.GetUniqueID());
	
	if (SightTarget != nullptr && SightTarget->GetTargetActor() != &TargetActor)
	{
		// this means given unique ID has already been recycled. 
		FOSESightTarget NewSightTarget(&TargetActor);

		SightTarget = &(ObservedTargets.Add(NewSightTarget.TargetId, NewSightTarget));
		SightTarget->SightTargetInterface = Cast<IOSESightTargetInterface>(&TargetActor);
	}
	else if (SightTarget == nullptr)
	{
		FOSESightTarget NewSightTarget(&TargetActor);

		SightTarget = &(ObservedTargets.Add(NewSightTarget.TargetId, NewSightTarget));
		SightTarget->SightTargetInterface = Cast<IOSESightTargetInterface>(&TargetActor);
	}

	// set/update data
	SightTarget->TeamId = FGenericTeamId::GetTeamIdentifier(&TargetActor);
	
	// generate all pairs and add them to current Sight Queries
	bool bNewQueriesAdded = false;
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();
	const FVector TargetLocation = TargetActor.GetActorLocation();

	for (OSEPerception::FListenerMap::TConstIterator ItListener(ListenersMap); ItListener; ++ItListener)
	{
		const FOSEPerceptionListener& Listener = ItListener->Value;
		const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();

		if (Listener.HasSense(GetSenseID()) && Listener.GetBodyActor() != &TargetActor)
		{
			const FDigestedProperties* PropDigest = GetDigestedProperties(Listener.GetListenerID());
			if (FOSESenseAffiliationFilter::ShouldSenseTeam(ListenersTeamAgent, TargetActor, PropDigest->AffiliationFlags))
			{
				// create a sight query		
				const float Importance = _CalcQueryImportance(ItListener->Value, TargetLocation, PropDigest->MaxRadiusSq);
				const bool bInRange = Importance > 0.0f;
				if (!bInRange)
				{
					bSightQueriesOutOfRangeDirty = true;
				}
				FOSESightQuery& AddedQuery = bInRange ? SightQueriesInRange.AddDefaulted_GetRef() : SightQueriesOutOfRange.AddDefaulted_GetRef();
				AddedQuery.ObserverId = ItListener->Key;
				AddedQuery.TargetId = SightTarget->TargetId;
				AddedQuery.Importance = Importance;
				
				if (OnAddedFunc)
				{
					OnAddedFunc(AddedQuery);
				}
				bNewQueriesAdded = true;
			}
		}
	}

	// sort Sight Queries
	if (bNewQueriesAdded)
	{
		RequestImmediateUpdate();
	}

	return bNewQueriesAdded;
}

void UOSESense_SightBase::OnNewListenerImpl(const FOSEPerceptionListener& NewListener)
{
   _NewListenerImpl(NewListener);
}

void UOSESense_SightBase::_NewListenerImpl(const FOSEPerceptionListener& NewListener)
{
   const FDigestedProperties* PropertiesDigest = GetDigestedProperties(NewListener.GetListenerID());
   GenerateQueriesForListener(NewListener, PropertiesDigest);
}


void UOSESense_SightBase::GenerateQueriesForListener(const FOSEPerceptionListener& Listener, const FDigestedProperties* PropertyDigest, const TFunction<void(FOSESightQuery&)>& OnAddedFunc/*= nullptr */)
{
	bool bNewQueriesAdded = false;
	const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();
	const AActor* Avatar = Listener.GetBodyActor();

	// create sight queries with all legal targets
	for (FTargetsContainer::TConstIterator ItTarget(ObservedTargets); ItTarget; ++ItTarget)
	{
		const AActor* TargetActor = ItTarget->Value.GetTargetActor();
		if (TargetActor == NULL || TargetActor == Avatar)
		{
			continue;
		}

		if (FOSESenseAffiliationFilter::ShouldSenseTeam(ListenersTeamAgent, *TargetActor, PropertyDigest->AffiliationFlags))
		{
			// create a sight query		
			const float Importance = _CalcQueryImportance(Listener, ItTarget->Value.GetLocationSimple(), PropertyDigest->MaxRadiusSq);
			const bool bInRange = Importance > 0.0f;
			if (!bInRange)
			{
				bSightQueriesOutOfRangeDirty = true;
			}
			FOSESightQuery& AddedQuery = bInRange ? SightQueriesInRange.AddDefaulted_GetRef() : SightQueriesOutOfRange.AddDefaulted_GetRef();
			AddedQuery.ObserverId = Listener.GetListenerID();
			AddedQuery.TargetId = ItTarget->Key;
			AddedQuery.Importance = Importance;

			if (OnAddedFunc)
			{
				OnAddedFunc(AddedQuery);
			}
			bNewQueriesAdded = true;
		}
	}

	// sort Sight Queries
	if (bNewQueriesAdded)
	{
		RequestImmediateUpdate();
	}
}
void UOSESense_SightBase::OnListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
   _ListenerUpdateImpl(UpdatedListener);
}

void UOSESense_SightBase::_ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_ListenerUpdate);

	// first, naive implementation:
	// 1. remove all queries by this listener
	// 2. proceed as if it was a new listener

	// see if this listener is a Target as well
	const FOSESightTarget::FTargetId AsTargetId = UpdatedListener.GetBodyActorUniqueID();
	FOSESightTarget* AsTarget = ObservedTargets.Find(AsTargetId);
	if (AsTarget != NULL)
	{
		if (AsTarget->Target.IsValid())
		{
			// if still a valid target then backup list of observers for which the listener was visible to restore in the newly created queries
			TSet<FOSEPerceptionListenerID> LastVisibleObservers;
			RemoveAllQueriesToTarget(AsTargetId, [&LastVisibleObservers](const FOSESightQuery& Query)
			{
				if (Query.bLastResult)
				{
					LastVisibleObservers.Add(Query.ObserverId);
				}
			});

			RegisterTarget(*(AsTarget->Target.Get()), [&LastVisibleObservers](FOSESightQuery& Query)
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
		TSet<FOSESightTarget::FTargetId> LastVisibleTargets;
		RemoveAllQueriesByListener(UpdatedListener, [&LastVisibleTargets](const FOSESightQuery& Query)
		{
			if (Query.bLastResult)
			{
				LastVisibleTargets.Add(Query.TargetId);
			}			
		});

		const FDigestedProperties* PropertiesDigest = GetDigestedProperties(ListenerID);

		GenerateQueriesForListener(UpdatedListener, PropertiesDigest, [&LastVisibleTargets](FOSESightQuery& Query)
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

void UOSESense_SightBase::OnListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   _ListenerRemovedImpl(RemovedListener);
}

void UOSESense_SightBase::_ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener)
{
   RemoveAllQueriesByListener(RemovedListener);
}

void UOSESense_SightBase::RemoveAllQueriesByListener(const FOSEPerceptionListener& Listener, const TFunction<void(const FOSESightQuery&)>& OnRemoveFunc/*= nullptr */)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_RemoveByListener);

	if ((SightQueriesInRange.Num() + SightQueriesOutOfRange.Num()) == 0)
	{
		return;
	}

	const uint32 ListenerId = Listener.GetListenerID();
	
	auto RemoveQuery = [&ListenerId, &OnRemoveFunc](TArray<FOSESightQuery>& SightQueries, const int32 QueryIndex)->EReverseForEachResult
	{
		const FOSESightQuery& sightQuery = SightQueries[QueryIndex];

		if (sightQuery.ObserverId == ListenerId)
		{
			if (OnRemoveFunc)
			{
				OnRemoveFunc(sightQuery);
			}
			SightQueries.RemoveAtSwap(QueryIndex, 1, EAllowShrinking::No);
			return EReverseForEachResult::Modified;
		}
		return EReverseForEachResult::UnTouched;
	};
	ReverseForEach(SightQueriesInRange, RemoveQuery);
	if(ReverseForEach(SightQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
	{
		bSightQueriesOutOfRangeDirty = true;
	}
}

void UOSESense_SightBase::RemoveAllQueriesToTarget(const FOSESightTarget::FTargetId& TargetId, const TFunction<void(const FOSESightQuery&)>& OnRemoveFunc/*= nullptr */)
{
	SCOPE_CYCLE_COUNTER(STAT_OSE_Sense_SightBase_RemoveToTarget);

	auto RemoveQuery = [&TargetId, &OnRemoveFunc](TArray<FOSESightQuery>& SightQueries, const int32 QueryIndex)->EReverseForEachResult
	{
		const FOSESightQuery& sightQuery = SightQueries[QueryIndex];

		if (sightQuery.TargetId == TargetId)
		{
			if (OnRemoveFunc)
			{
				OnRemoveFunc(sightQuery);
			}
			SightQueries.RemoveAtSwap(QueryIndex, 1, EAllowShrinking::No);
			return EReverseForEachResult::Modified;
		}
		return EReverseForEachResult::UnTouched;
	};
	ReverseForEach(SightQueriesInRange, RemoveQuery);
	if (ReverseForEach(SightQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
	{
		bSightQueriesOutOfRangeDirty = true;
	}
}

void UOSESense_SightBase::OnListenerForgetsActor(const FOSEPerceptionListener& Listener, AActor& ActorToForget)
{
	const uint32 ListenerId = Listener.GetListenerID();
	const uint32 TargetId = ActorToForget.GetUniqueID();
	
	auto ForgetPreviousResult = [&ListenerId, &TargetId](FOSESightQuery& sightQuery)->EForEachResult
	{
		if (sightQuery.ObserverId == ListenerId && sightQuery.TargetId == TargetId)
		{
			// assuming one query per observer-target pair
			sightQuery.ForgetPreviousResult();
			return EForEachResult::Break;
		}
		return EForEachResult::Continue;
	};

	if (ForEach(SightQueriesInRange, ForgetPreviousResult) == EForEachResult::Continue)
	{
		ForEach(SightQueriesOutOfRange, ForgetPreviousResult);
	}
}

void UOSESense_SightBase::OnListenerForgetsAll(const FOSEPerceptionListener& Listener)
{
	const uint32 ListenerId = Listener.GetListenerID();

	auto ForgetPreviousResult = [&ListenerId](FOSESightQuery& sightQuery)->EForEachResult
	{
		if (sightQuery.ObserverId == ListenerId)
		{
			sightQuery.ForgetPreviousResult();
		}
		return EForEachResult::Continue;
	};

	ForEach(SightQueriesInRange, ForgetPreviousResult);
	ForEach(SightQueriesOutOfRange, ForgetPreviousResult);
}
