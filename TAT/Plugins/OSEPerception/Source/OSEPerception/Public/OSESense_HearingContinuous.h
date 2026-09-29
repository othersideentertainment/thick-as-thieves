// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GenericTeamAgentInterface.h"
#include "OSESense.h"
#include "OSESense_HearingContinuous.generated.h"

class UOSESense_HearingContinuous;
class UOSESenseConfig_HearingContinuous;


USTRUCT(BlueprintType)
struct OSEPERCEPTION_API FOSEHearingEvent
{
	GENERATED_USTRUCT_BODY()

	typedef UOSESense_HearingContinuous FSenseClass;

   //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   //float GameTimeCreation;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	float Duration;

	/** if not set Instigator's location will be used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FVector NoiseLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
   float BaseStimulusStrength;

   /**
	 * Actor triggering the sound.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	TObjectPtr<AActor> Instigator;

	FOSEHearingEvent() 
   : Duration(0.f)
   , NoiseLocation(FVector::ZeroVector)
   , BaseStimulusStrength(0.f)
   , Instigator(nullptr) {}

	FOSEHearingEvent(AActor* InInstigator, const FVector& Location, float StimStrength, float InDuration)
		: Duration(InDuration)
      , NoiseLocation(Location)
      , BaseStimulusStrength(StimStrength)
      , Instigator(InInstigator)
	{
	}
};

struct FOSEHearingTarget
{
	typedef uint32 FTargetId;
	static const FTargetId InvalidTargetId;

	TWeakObjectPtr<AActor> Target;
	FGenericTeamId TeamId;
	FTargetId TargetId;
   float Intensity;

	FOSEHearingTarget(AActor* InTarget = NULL, FGenericTeamId InTeamId = FGenericTeamId::NoTeam, float InIntensity = 0.0f);

	FORCEINLINE FVector GetLocationSimple() const
	{
		const AActor* TargetPtr = Target.Get();
		return TargetPtr ? TargetPtr->GetActorLocation() : FVector::ZeroVector;
	}

	FORCEINLINE const AActor* GetTargetActor() const { return Target.Get(); }
};

struct FOSEHearingQuery
{
	FOSEPerceptionListenerID ObserverId;
	FOSEHearingTarget::FTargetId TargetId;

	float Score;
	float Importance;
   float Intensity;

	FVector LastSeenLocation;

	/** User data that can be used inside the IOSEHearingTargetInterface::CanBeSeenFrom method to store a persistence state */ 
	mutable int32 UserData; 

	uint64 bLastResult : 1;
   uint64 bLastPropagated : 1;
	uint64 LastProcessedFrameNumber :62;

   //Was this query generated from an event
   bool bEvent = false;

   float EventCreationGameTime;
   float EventDuration;
   FVector EventFiredLocation;

	FOSEHearingQuery(FOSEPerceptionListenerID ListenerId = FOSEPerceptionListenerID::InvalidID(), FOSEHearingTarget::FTargetId Target = FOSEHearingTarget::InvalidTargetId)
		: ObserverId(ListenerId), TargetId(Target), Score(0.f), Importance(0.f), Intensity(0.f), LastSeenLocation(FAISystem::InvalidLocation), UserData(0), bLastResult(false), bLastPropagated(false), LastProcessedFrameNumber(GFrameCounter)
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

      
		bool operator()(const FOSEHearingQuery& A, const FOSEHearingQuery& B) const
		{
         return A.Score > B.Score;
		}
	};
};

UCLASS(ClassGroup=AI, config=Game)
class OSEPERCEPTION_API UOSESense_HearingContinuous : public UOSESense
{
	GENERATED_UCLASS_BODY()

public:
   struct FDigestedProperties
   {
      float MaxRadiusSq;
      float HearingRadiusSq;
      float LoseHearingRadiusSq;
      float SoundMaskingPercent;
      float MinErrorRadius;
      float MaxErrorRadius;
      uint8 AffiliationFlags;

      FDigestedProperties();
      FDigestedProperties(const UOSESenseConfig_HearingContinuous& SenseConfig);
   };
	typedef TMap<FOSEHearingTarget::FTargetId, FOSEHearingTarget> FTargetsContainer;
	FTargetsContainer ObservedTargets;

	/** The HearingQueries are a n^2 problem and to reduce the sort time, they are now split between in range and out of range */
	/** Since the out of range queries only age as the distance component of the score is always 0, there is few need to sort them */
	/** In the majority of the cases most of the queries are out of range, so the sort time is greatly reduced as we only sort the in range queries */
	int32 NextOutOfRangeIndex = 0;
	bool bHearingQueriesOutOfRangeDirty = true;
	TArray<FOSEHearingQuery> HearingQueriesOutOfRange;
	TArray<FOSEHearingQuery> HearingQueriesInRange;

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
	float HearingLimitQueryImportance;

   TMap<FOSEPerceptionListenerID, FDigestedProperties> DigestedProperties;
public:

	virtual void PostInitProperties() override;
	
	void RegisterEvent(const FOSEHearingEvent& Event);
    virtual void RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent) override;

	virtual void RegisterSource(AActor& SourceActors) override;
	virtual void UnregisterSource(AActor& SourceActor) override;
	
	virtual void OnListenerForgetsActor(const FOSEPerceptionListener& Listener, AActor& ActorToForget) override;
	virtual void OnListenerForgetsAll(const FOSEPerceptionListener& Listener) override;

   virtual bool IsTargetInHearingVolume(FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSEHearingTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, bool bLastResult) const;
   virtual const FDigestedProperties* GetDigestedProperties(FOSEPerceptionListenerID ListenerID) const;

   UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext="WorldContextObject"))
	static void ReportSoundEvent(UObject* WorldContextObject, FVector NoiseLocation, float Loudness = 1.f, AActor* Instigator = nullptr, float Duration = 0.f);

protected:
	virtual float Update() override;

   void SortQueriesForUpdate();

   
   virtual bool ComputeHearibility(const UWorld* World, FOSEHearingQuery& HearingQuery, FOSEPerceptionListener& Listener, const AActor* ListenerActor, FOSEHearingTarget& Target, AActor* TargetActor, const FDigestedProperties* PropDigest, float& OutStimulusStrength, FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed) const;
   
	virtual bool _ShouldAutomaticallyHearTarget(const FDigestedProperties* PropDigest, FOSEHearingQuery* HearingQuery, FOSEPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const;
   virtual bool _ShouldNeverHearTarget(const FDigestedProperties* propDigest, FOSEHearingQuery* HearingQuery, FOSEPerceptionListener& listener, AActor* targetActor) const;
	void UpdateQueryVisibilityStatus(FOSEHearingQuery& HearingQuery, FOSEPerceptionListener& Listener, const bool bIsVisible, const FVector& SeenLocation, const float StimulusStrength, AActor* TargetActor) const;

	void OnNewListenerImpl(const FOSEPerceptionListener& NewListener);
   void OnListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener);
   void OnListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener);
	
   virtual void _NewListenerImpl(const FOSEPerceptionListener& NewListener);
   virtual void _ListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener);
   virtual void _ListenerRemovedImpl(const FOSEPerceptionListener& RemovedListener);

	
	void GenerateQueriesForListener(const FOSEPerceptionListener& Listener, const FDigestedProperties* PropertyDigest, const TFunction<void(FOSEHearingQuery&)>& OnAddedFunc = nullptr);

	void RemoveAllQueriesByListener(const FOSEPerceptionListener& Listener, const TFunction<void(const FOSEHearingQuery&)>& OnRemoveFunc = nullptr);
	void RemoveAllQueriesToTarget(const FOSEHearingTarget::FTargetId& TargetId, const TFunction<void(const FOSEHearingQuery&)>& OnRemoveFunc = nullptr);

	/** returns information whether new LoS queries have been added */
   bool RegisterTarget(AActor& TargetActor, const FVector& TargetLocation, const TFunction<void(FOSEHearingQuery&)>& OnAddedFunc = nullptr, bool bEvent = false, float MaxGameTimeAge = 0.0f, float EventBaseStimulusStrength = 0.0f);

	float _CalcQueryImportance(const FOSEPerceptionListener& Listener, const FVector& TargetLocation, const float HearingRadiusSq) const;

   void _RemoveExpiredQueries(const TArray<FOSEHearingQuery*>& ExpiredEventQueries);

   FVector _CalcStimLocation(const FDigestedProperties* propDigest,const FOSEHearingQuery& HearingQuery, const FVector& TargetLocation);
};
