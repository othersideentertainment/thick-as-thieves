// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GenericTeamAgentInterface.h"
#include "OSESense.h"
#include "OSESense_SightBase.generated.h"

class IOSESightTargetInterface;
class UOSESense_SightBase;
//class UOSESenseConfig_Sight;

enum class EOSESightPerceptionEvents : uint8
{
	Undefined,
	GainedSight,
	LostSight
};

USTRUCT()
struct OSEPERCEPTION_API FOSESightEvent
{
	GENERATED_USTRUCT_BODY()

	typedef UOSESense_SightBase FSenseClass;

	float Age;
	EOSESightPerceptionEvents EventType;	

	UPROPERTY()
	TObjectPtr<AActor> SeenActor;

	UPROPERTY()
	TObjectPtr<AActor> Observer;

	FOSESightEvent() : SeenActor(nullptr), Observer(nullptr) {}

	FOSESightEvent(AActor* InSeenActor, AActor* InObserver, EOSESightPerceptionEvents InEventType)
		: Age(0.f), EventType(InEventType), SeenActor(InSeenActor), Observer(InObserver)
	{
	}
};

struct FOSESightTarget
{
	typedef uint32 FTargetId;
	static const FTargetId InvalidTargetId;

	TWeakObjectPtr<AActor> Target;
	IOSESightTargetInterface* SightTargetInterface;
	FGenericTeamId TeamId;
	FTargetId TargetId;

	FOSESightTarget(AActor* InTarget = NULL, FGenericTeamId InTeamId = FGenericTeamId::NoTeam);

	FORCEINLINE FVector GetLocationSimple() const
	{
		const AActor* TargetPtr = Target.Get();
		return TargetPtr ? TargetPtr->GetActorLocation() : FVector::ZeroVector;
	}

	FORCEINLINE const AActor* GetTargetActor() const { return Target.Get(); }
};

struct FOSESightQuery
{
	FOSEPerceptionListenerID ObserverId;
	FOSESightTarget::FTargetId TargetId;

	float Score;
	float Importance;

	FVector LastSeenLocation;

	/** User data that can be used inside the IOSESightTargetInterface::CanBeSeenFrom method to store a persistence state */ 
	mutable int32 UserData; 

	uint64 bLastResult:1;
	uint64 LastProcessedFrameNumber :63;

	FOSESightQuery(FOSEPerceptionListenerID ListenerId = FOSEPerceptionListenerID::InvalidID(), FOSESightTarget::FTargetId Target = FOSESightTarget::InvalidTargetId)
		: ObserverId(ListenerId), TargetId(Target), Score(0), Importance(0), LastSeenLocation(FAISystem::InvalidLocation), UserData(0), bLastResult(false), LastProcessedFrameNumber(GFrameCounter)
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

		bool operator()(const FOSESightQuery& A, const FOSESightQuery& B) const
		{
			return A.Score > B.Score;
		}
	};
};

UCLASS(ClassGroup=AI, config=Game)
class OSEPERCEPTION_API UOSESense_SightBase : public UOSESense
{
	GENERATED_UCLASS_BODY()

public:
   struct FDigestedProperties
   {
      uint8 AffiliationFlags;
      float MaxRadiusSq;

      FDigestedProperties();
   };
	typedef TMap<FOSESightTarget::FTargetId, FOSESightTarget> FTargetsContainer;
	FTargetsContainer ObservedTargets;

	/** The SightQueries are a n^2 problem and to reduce the sort time, they are now split between in range and out of range */
	/** Since the out of range queries only age as the distance component of the score is always 0, there is few need to sort them */
	/** In the majority of the cases most of the queries are out of range, so the sort time is greatly reduced as we only sort the in range queries */
	int32 NextOutOfRangeIndex = 0;
	bool bSightQueriesOutOfRangeDirty = true;
	TArray<FOSESightQuery> SightQueriesOutOfRange;
	TArray<FOSESightQuery> SightQueriesInRange;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception", config)
	int32 MaxTracesPerTick;

	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception", config)
	int32 MinQueriesPerTimeSliceCheck;

	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception", config)
	double MaxTimeSlicePerTick;

	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception", config)
	float HighImportanceQueryDistanceThreshold;

	float HighImportanceDistanceSquare;

	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception", config)
	float MaxQueryImportance;

	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception", config)
	float SightLimitQueryImportance;

	ECollisionChannel DefaultSightCollisionChannel;

public:

	virtual void PostInitProperties() override;
	
	void RegisterEvent(const FOSESightEvent& Event);	

	virtual void RegisterSource(AActor& SourceActors) override;
	virtual void UnregisterSource(AActor& SourceActor) override;
	
	virtual void OnListenerForgetsActor(const FOSEPerceptionListener& Listener, AActor& ActorToForget) override;
	virtual void OnListenerForgetsAll(const FOSEPerceptionListener& Listener) override;

   virtual bool IsTargetInSightVolume(FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSESightTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, bool bLastResult) const;
   virtual const FDigestedProperties* GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const;
protected:
	virtual float Update() override;

   
   virtual bool ComputeVisibility(const UWorld* World, FOSESightQuery& SightQuery, FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSESightTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, float& OutStimulusStrength, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed) const;
   
	virtual bool _ShouldAutomaticallySeeTarget(const FDigestedProperties* PropDigest, FOSESightQuery* SightQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const;
   virtual bool _ShouldNeverSeeTarget(const FDigestedProperties* propDigest, FOSESightQuery* sightQuery, FOSEPerceptionListener& listener, AActor* targetActor) const;
   virtual bool _ShouldAutoSucceedLOS(const FDigestedProperties* propDigest, const AActor* ListenerActor, AActor* targetActor) const;
	void UpdateQueryVisibilityStatus(FOSESightQuery& SightQuery, FOSEPerceptionListener& Listener, const bool bIsVisible, const FVector& SeenLocation, const float StimulusStrength, AActor* TargetActor, const FVector& TargetLocation) const;

	void OnNewListenerImpl(const FOSEPerceptionListener& NewListener);
   void OnListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener);
   void OnListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener);
	
   virtual void _NewListenerImpl(const FOSEPerceptionListener& NewListener);
   virtual void _ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener);
   virtual void _ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener);

	
	void GenerateQueriesForListener(const FOSEPerceptionListener& Listener, const FDigestedProperties* PropertyDigest, const TFunction<void(FOSESightQuery&)>& OnAddedFunc = nullptr);

	void RemoveAllQueriesByListener(const FOSEPerceptionListener& Listener, const TFunction<void(const FOSESightQuery&)>& OnRemoveFunc = nullptr);
	void RemoveAllQueriesToTarget(const FOSESightTarget::FTargetId& TargetId, const TFunction<void(const FOSESightQuery&)>& OnRemoveFunc = nullptr);

	/** returns information whether new LoS queries have been added */
	bool RegisterTarget(AActor& TargetActor, const TFunction<void(FOSESightQuery&)>& OnAddedFunc = nullptr);

	float _CalcQueryImportance(const FOSEPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq) const;

};
