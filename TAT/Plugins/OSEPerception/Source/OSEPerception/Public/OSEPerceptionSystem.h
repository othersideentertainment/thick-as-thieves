// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.h"
#include "AISubsystem.h"
#include "OSEPerceptionSystem.generated.h"

class UOSEPerceptionComponent;
class UOSESenseEvent;

OSEPERCEPTION_API DECLARE_LOG_CATEGORY_EXTERN(LogOSEPerception, Warning, All);

class APawn;

/**
 *	By design checks perception between hostile teams
 */
UCLASS(ClassGroup=AI, config=Game, defaultconfig)
class OSEPERCEPTION_API UOSEPerceptionSystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
		
public:

	UOSEPerceptionSystem();
	
	// FTickableGameObject begin
   virtual void Tick(float DeltaTime) override;
   virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	// FTickableGameObject end

protected:	
	OSEPerception::FListenerMap ListenerContainer;

	UPROPERTY()
	TArray<TObjectPtr<UOSESense>> Senses;

	UPROPERTY(config, EditAnywhere, Category = Perception)
	float PerceptionAgingRate;

	FActorEndPlaySignature::FDelegate StimuliSourceEndPlayDelegate;

	// not a UPROPERTY on purpose so that we have a control over when stuff gets removed from the map
	TMap<const AActor*, FOSEPerceptionStimuliSource> RegisteredStimuliSources;

	uint32 bHandlePawnNotification : 1;

	struct FDelayedStimulus
	{
		float DeliveryTimestamp;
		FOSEPerceptionListenerID ListenerId;
		TWeakObjectPtr<AActor> Instigator;
		FOSEStimulus Stimulus;
	};

	TArray<FDelayedStimulus> DelayedStimuli;

	struct FPerceptionSourceRegistration
	{
		FOSESenseID SenseID;
		TWeakObjectPtr<AActor> Source;

		FPerceptionSourceRegistration(FOSESenseID InSenseID, AActor* SourceActor)
			: SenseID(InSenseID), Source(SourceActor)
		{}

		FORCEINLINE bool operator==(const FPerceptionSourceRegistration& Other) const
		{
			return SenseID == Other.SenseID && Source == Other.Source;
		}
	};
	TArray<FPerceptionSourceRegistration> SourcesToRegister;

public:

	FORCEINLINE bool IsSenseInstantiated(const FOSESenseID& SenseID) const { return SenseID.IsValid() && Senses.IsValidIndex(SenseID) && Senses[SenseID] != nullptr; }

	/** Registers listener if not registered */
	void UpdateListener(UOSEPerceptionComponent& Listener);
	void UnregisterListener(UOSEPerceptionComponent& Listener);

	template<typename FEventClass, typename FSenseClass = typename FEventClass::FSenseClass>
	void OnEvent(const FEventClass& Event)
	{
		const FOSESenseID SenseID = UOSESense::GetSenseID<FSenseClass>();
		if (Senses.IsValidIndex(SenseID) && Senses[SenseID] != nullptr)
		{
			((FSenseClass*)Senses[SenseID])->RegisterEvent(Event);
		}
		// otherwise there's no one interested in this event, skip it.
	}

	template<typename FEventClass, typename FSenseClass = typename FEventClass::FSenseClass>
	void OnEventsBatch(const TArray<FEventClass>& Events)
	{
		if (Events.Num() > 0)
		{
			const FOSESenseID SenseID = UOSESense::GetSenseID<FSenseClass>();
			if (Senses.IsValidIndex(SenseID) && Senses[SenseID] != nullptr)
			{
				((FSenseClass*)Senses[SenseID])->RegisterEventsBatch(Events);
			}
		}
		// otherwise there's no one interested in this event, skip it.
	}

	template<typename FEventClass, typename FSenseClass = typename FEventClass::FSenseClass>
	static void OnEvent(UWorld* World, const FEventClass& Event)
	{
		UOSEPerceptionSystem* PerceptionSys = GetCurrent(World);
		if (PerceptionSys != NULL)
		{
			PerceptionSys->OnEvent<FEventClass, FSenseClass>(Event);
		}
	}

	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void ReportEvent(UOSESenseEvent* PerceptionEvent);

	UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext="WorldContextObject"))
	static void ReportPerceptionEvent(UObject* WorldContextObject, UOSESenseEvent* PerceptionEvent);

	/** Registers a source of given sense's stimuli */
	template<typename FSenseClass>
	void RegisterSource(AActor& SourceActor);

	/** Registers given actor as a source for all registered senses */
	void RegisterSource(AActor& SourceActor);

	void RegisterSourceForSenseClass(TSubclassOf<UOSESense> Sense, AActor& Target);

	/** 
	 *	unregisters given actor from the list of active stimuli sources
	 *	@param Sense if null will result in removing SourceActor from all the senses
	 */
	void UnregisterSource(AActor& SourceActor, const TSubclassOf<UOSESense> Sense = nullptr);

	void OnListenerForgetsActor(const UOSEPerceptionComponent& Listener, AActor& ActorToForget);
	void OnListenerForgetsAll(const UOSEPerceptionComponent& Listener);
	void OnListenerConfigUpdated(FOSESenseID SenseID, const UOSEPerceptionComponent& Listener);

	void RegisterDelayedStimulus(FOSEPerceptionListenerID ListenerId, float Delay, AActor* Instigator, const FOSEStimulus& Stimulus);

	static UOSEPerceptionSystem* GetCurrent(UObject* WorldContextObject);
	static UOSEPerceptionSystem* GetCurrent(UWorld& World);

	static void MakeNoiseImpl(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation, float MaxRange, FName Tag);

	UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext="WorldContextObject"))
	static bool RegisterPerceptionStimuliSource(UObject* WorldContextObject, TSubclassOf<UOSESense> Sense, AActor* Target);

	FOSESenseID RegisterSenseClass(TSubclassOf<UOSESense> SenseClass);

	UFUNCTION(BlueprintCallable, Category = "OSE|Perception", meta = (WorldContext="WorldContextObject"))
	static TSubclassOf<UOSESense> GetSenseClassForStimulus(UObject* WorldContextObject, const FOSEStimulus& Stimulus);

   UOSESense* GetSense(FOSESenseID SenseID);
	
protected:
	
	UFUNCTION()
	void OnPerceptionStimuliSourceEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	
	/** requests registration of a given actor as a perception data source for specified sense */
	void RegisterSource(FOSESenseID SenseID, AActor& SourceActor);

	/** iterates over all pawns and registeres them as a source for sense indicated by SenseID. Note that this will 
	 *  be performed only for senses that request that (see UOSESense.bAutoRegisterAllPawnsAsSources).*/
	virtual void RegisterAllPawnsAsSourcesForSense(FOSESenseID SenseID);

	enum EDelayedStimulusSorting 
	{
		RequiresSorting,
		NoNeedToSort,
	};
	/** sorts DelayedStimuli and delivers all the ones that are no longer "in the future"
	 *	@return true if any stimuli has become "current" stimuli (meaning being no longer in future) */
	bool DeliverDelayedStimuli(EDelayedStimulusSorting Sorting);
	void OnNewListener(const FOSEPerceptionListener& NewListener);
	void OnListenerUpdate(const FOSEPerceptionListener& UpdatedListener);
	void OnListenerRemoved(const FOSEPerceptionListener& UpdatedListener);
	void PerformSourceRegistration();

	/** Returns true if aging resulted in tagging any of the listeners to process 
	 *	its stimuli (@see MarkForStimulusProcessing)*/
	bool AgeStimuli(const float Amount);

	friend class UOSESense;
	FORCEINLINE OSEPerception::FListenerMap& GetListenersMap() { return ListenerContainer; }

   virtual void OnActorSpawned(AActor* SpawnedActor);
	virtual void OnNewPawn(APawn& Pawn);
	virtual void OnWorldBeginPlay(UWorld& InWorld);
	/** Timestamp of the next stimuli aging */
	float NextStimuliAgingTick;
private:
	/** cached world's timestamp */
	float CurrentTime;
   bool _IsAllowedToTick { false };
};

//////////////////////////////////////////////////////////////////////////
template<typename FSenseClass>
void UOSEPerceptionSystem::RegisterSource(AActor& SourceActor)
{
	FOSESenseID SenseID = UOSESense::GetSenseID<FSenseClass>();
	if (IsSenseInstantiated(SenseID) == false)
	{
		RegisterSenseClass(FSenseClass::StaticClass());
		SenseID = UOSESense::GetSenseID<FSenseClass>();
		check(SenseID.IsValid());
	}
	RegisterSource(SenseID, SourceActor);
}
