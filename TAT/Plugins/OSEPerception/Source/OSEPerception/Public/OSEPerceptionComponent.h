// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectKey.h"
#include "Templates/SubclassOf.h"
#include "Components/ActorComponent.h"
#include "EngineDefines.h"
#include "GenericTeamAgentInterface.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.h"
#include "OSEPerceptionSystem.h"
#include "OSEPerceptionComponent.generated.h"

class AAIController;
class FGameplayDebuggerCategory;
class UOSESenseConfig;
struct FVisualLogEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEPerceptionUpdatedDelegate, const TArray<AActor*>&, UpdatedActors);
DECLARE_MULTICAST_DELEGATE_TwoParams(FActorPerceptionUpdatedDelegate, AActor*, const FOSEStimulus&);

USTRUCT(BlueprintType, meta = (DisplayName = "Sensed Actor's Update Data"))
struct FOSEPerceptionUpdateInfo
{
	GENERATED_USTRUCT_BODY()

	/** Id of to the stimulus source */
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	int32 TargetId = -1;

	/** Actor associated to the stimulus (can be null) */
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	TWeakObjectPtr<AActor> Target;

	/** Updated stimulus */
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	FOSEStimulus Stimulus;

	FOSEPerceptionUpdateInfo() = default;
	FOSEPerceptionUpdateInfo(const int32 TargetId, const TWeakObjectPtr<AActor>& Target, const FOSEStimulus& Stimulus);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEActorPerceptionInfoUpdatedDelegate, const FOSEPerceptionUpdateInfo&, UpdateInfo);

struct OSEPERCEPTION_API FOSEActorPerceptionInfo
{
	TWeakObjectPtr<AActor> Target;

	TArray<FOSEStimulus> LastSensedStimuli;

	/** if != MAX indicates the sense that takes precedense over other senses when it comes
		to determining last stimulus location */
	FOSESenseID DominantSense;

	/** indicates whether this Actor is hostile to perception holder */
	uint32 bIsHostile : 1;
	
	FOSEActorPerceptionInfo(AActor* InTarget = NULL)
		: Target(InTarget), DominantSense(FOSESenseID::InvalidID())
	{
		LastSensedStimuli.AddDefaulted(FOSESenseID::GetSize());
	}

	/** Retrieves last known location. Active (last reported as "successful")
	 *	stimuli are preferred. */
	FVector GetLastStimulusLocation(float* OptionalAge = NULL) const 
	{
		FVector Location(FAISystem::InvalidLocation);
		float BestAge = FLT_MAX;
		bool bBestWasSuccessfullySensed = false;
		for (int32 Sense = 0; Sense < LastSensedStimuli.Num(); ++Sense)
		{
			const float Age = LastSensedStimuli[Sense].GetAge();
			const bool bWasSuccessfullySensed = LastSensedStimuli[Sense].WasSuccessfullySensed();

			if (Age >= 0 && (Age < BestAge 
				|| (bBestWasSuccessfullySensed == false && bWasSuccessfullySensed)
				|| (Sense == DominantSense && bWasSuccessfullySensed)))
			{
				BestAge = Age;
				Location = LastSensedStimuli[Sense].StimulusLocation;
				bBestWasSuccessfullySensed = bWasSuccessfullySensed;

				if (Sense == DominantSense && bWasSuccessfullySensed)
				{
					// if dominant sense is active we don't want to look any further 
					break;
				}
			}
		}

		if (OptionalAge)
		{
			*OptionalAge = BestAge;
		}

		return Location;
	}

	/** it includes both currently live (visible) stimulus, as well as "remembered" ones */
	bool HasAnyKnownStimulus() const
	{
		for (const FOSEStimulus& Stimulus : LastSensedStimuli)
		{
			// not that WasSuccessfullySensed will return 'false' for expired stimuli
			if (Stimulus.IsValid() && (Stimulus.WasSuccessfullySensed() == true || Stimulus.IsExpired() == false))
			{
				return true;
			}
		}

		return false;
	}

	/** Indicates currently live (visible) stimulus from any sense */
	bool HasAnyCurrentStimulus() const
	{
		for (const FOSEStimulus& Stimulus : LastSensedStimuli)
		{
			// not that WasSuccessfullySensed will return 'false' for expired stimuli
			if (Stimulus.IsValid() && Stimulus.WasSuccessfullySensed() == true && Stimulus.IsExpired() == false)
			{
				return true;
			}
		}

		return false;
	}

	/** Retrieves location of the last sensed stimuli for a given sense
	* @param Sense	The AISenseID of the sense
	*
	* @return Location of the last sensed stimuli or FAISystem::InvalidLocation if given sense has never registered related Target actor or if last stimuli has expired.
	*/
	FORCEINLINE FVector GetStimulusLocation(FOSESenseID Sense) const
	{
		return LastSensedStimuli.IsValidIndex(Sense) && (LastSensedStimuli[Sense].IsValid() && (LastSensedStimuli[Sense].IsExpired() == false)) ? LastSensedStimuli[Sense].StimulusLocation : FAISystem::InvalidLocation;
	}

	/** Retrieves receiver location of the last sense stimuli for a given sense
	* @param Sense	The AISenseID of the sense
	*
	* @return Location of the receiver for the last sensed stimuli or FAISystem::InvalidLocation if given sense has never registered related Target actor or last stimuli has expired.
	*/
	FORCEINLINE FVector GetReceiverLocation(FOSESenseID Sense) const
	{
		return LastSensedStimuli.IsValidIndex(Sense) && (LastSensedStimuli[Sense].IsValid() && (LastSensedStimuli[Sense].IsExpired() == false)) ? LastSensedStimuli[Sense].ReceiverLocation : FAISystem::InvalidLocation;
	}

	/** Indicates a currently active or "remembered" stimuli for a given sense
	* @param Sense	The AISenseID of the sense
	*
	* @return True if a target has been registered (even if not currently sensed) for the given sense and the stimuli is not expired.
	*/
	FORCEINLINE bool HasKnownStimulusOfSense(FOSESenseID Sense) const
	{
		return LastSensedStimuli.IsValidIndex(Sense) && (LastSensedStimuli[Sense].IsValid() && (LastSensedStimuli[Sense].IsExpired() == false));
	}

	/** Indicates a currently active stimuli for a given sense
	* @param Sense	The AISenseID of the sense
	*
	* @return True if a target is still sensed for the given sense and the stimuli is not expired.
	*/
	FORCEINLINE bool IsSenseActive(FOSESenseID Sense) const
	{
		return LastSensedStimuli.IsValidIndex(Sense) && LastSensedStimuli[Sense].IsActive();
	}
	
	/** takes all "newer" info from Other and absorbs it */
	void Merge(const FOSEActorPerceptionInfo& Other);
};

USTRUCT(BlueprintType, meta = (DisplayName = "Sensed Actor's Data"))
struct FOSEPerceptionBlueprintInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	TObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	TArray<FOSEStimulus> LastSensedStimuli;

	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	uint32 bIsHostile : 1;

	FOSEPerceptionBlueprintInfo() : Target(NULL), bIsHostile(false)
	{}
	FOSEPerceptionBlueprintInfo(const FOSEActorPerceptionInfo& Info);
};

/**
 *	OSEPerceptionComponent is used to register as stimuli listener in OSEPerceptionSystem
 *	and gathers registered stimuli. UpdatePerception is called when component gets new stimuli (batched)
 */
UCLASS(ClassGroup=AI, HideCategories=(Activation, Collision), meta=(BlueprintSpawnableComponent), config=Game)
class OSEPERCEPTION_API UOSEPerceptionComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
	
	static const int32 InitialStimuliToProcessArraySize;

	typedef TMap<TObjectKey<AActor>, FOSEActorPerceptionInfo> TActorPerceptionContainer;
	typedef TActorPerceptionContainer FActorPerceptionContainer;

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "OSE Perception")
	TArray<TObjectPtr<UOSESenseConfig>> SensesConfig;

	/** Indicated sense that takes precedence over other senses when determining sensed actor's location. 
	 *	Should be set to one of the senses configured in SensesConfig, or None. */
	UPROPERTY(EditDefaultsOnly, Category = "OSE Perception")
	TSubclassOf<UOSESense> DominantSense;
	
	FOSESenseID DominantSenseID;

	UPROPERTY(Transient)
	TObjectPtr<AAIController> AIOwner;

	/** @todo this field is misnamed. It's an allow list. */
	FOSEPerceptionChannelAllowList PerceptionFilter;

private:
	FOSEPerceptionListenerID PerceptionListenerId;
	FActorPerceptionContainer PerceptualData;
		
protected:	
	struct FStimulusToProcess
	{
		TObjectKey<AActor> Source;
		FOSEStimulus Stimulus;

		FStimulusToProcess(AActor* InSource, const FOSEStimulus& InStimulus)
			: Source(InSource), Stimulus(InStimulus)
		{

		}
	};

	TArray<FStimulusToProcess> StimuliToProcess; 
	
	/** max age of stimulus to consider it "active" (e.g. target is visible) */
	TArray<float> MaxActiveAge;

private:

	/** Determines whether all knowledge of previously sensed actors will be removed or not when they become stale.
		That is, when they are no longer perceived and have exceeded the max age of the sense. */
	uint32 bForgetStaleActors : 1;

	uint32 bCleanedUp : 1;

public:

	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	UFUNCTION()
	void OnOwnerEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
	
	void GetLocationAndDirection(FVector& Location, FVector& Direction) const;
	const AActor* GetBodyActor() const;
	AActor* GetMutableBodyActor();

	FORCEINLINE const FOSEPerceptionChannelAllowList GetPerceptionFilter() const { return PerceptionFilter; }

	FGenericTeamId GetTeamIdentifier() const;
	FORCEINLINE FOSEPerceptionListenerID GetListenerId() const { return PerceptionListenerId; }

	FVector GetActorLocation(const AActor& Actor) const;
	FORCEINLINE const FOSEActorPerceptionInfo* GetActorInfo(const AActor& Actor) const { return PerceptualData.Find(&Actor); }
	FORCEINLINE FActorPerceptionContainer::TIterator GetPerceptualDataIterator() { return FActorPerceptionContainer::TIterator(PerceptualData); }
	FORCEINLINE FActorPerceptionContainer::TConstIterator GetPerceptualDataConstIterator() const { return FActorPerceptionContainer::TConstIterator(PerceptualData); }

	virtual void GetHostileActors(TArray<AActor*>& OutActors) const;
	
	void GetHostileActorsBySense(TSubclassOf<UOSESense> SenseToFilterBy, TArray<AActor*>& OutActors) const;

	/**	Retrieves all actors in PerceptualData matching the predicate.
	 *	@return whether dead data (invalid actors) have been found while iterating over PerceptualData
	 */
	bool GetFilteredActors(TFunctionRef<bool(const FOSEActorPerceptionInfo&)> Predicate, TArray<AActor*>& OutActors) const;

	// @note Will stop on first age 0 stimulus
	const FOSEActorPerceptionInfo* GetFreshestTrace(const FOSESenseID Sense) const;
	
	void SetDominantSense(TSubclassOf<UOSESense> InDominantSense);
	FORCEINLINE FOSESenseID GetDominantSenseID() const { return DominantSenseID; }
	FORCEINLINE TSubclassOf<UOSESense> GetDominantSense() const { return DominantSense; }
	UOSESenseConfig* GetSenseConfig(const FOSESenseID& SenseID);
	const UOSESenseConfig* GetSenseConfig(const FOSESenseID& SenseID) const;
	void ConfigureSense(UOSESenseConfig& SenseConfig);

	typedef TArray<UOSESenseConfig*>::TConstIterator TAISenseConfigConstIterator;
	TAISenseConfigConstIterator GetSensesConfigIterator() const;

	/** Notifies OSEPerceptionSystem to update properties for this "stimuli listener" */
	UFUNCTION(BlueprintCallable, Category="OSE|Perception")
	void RequestStimuliListenerUpdate();

	/** Allows toggling senses on and off */
	void UpdatePerceptionAllowList(const FOSESenseID Channel, const bool bNewValue);

	void RegisterStimulus(AActor* Source, const FOSEStimulus& Stimulus);
	void ProcessStimuli();
	/** Returns true if, as result of stimuli aging, this listener needs an update (like if some stimuli expired) */
	bool AgeStimuli(const float ConstPerceptionAgingRate);
	void ForgetActor(AActor* ActorToForget);

	/** basically cleans up PerceptualData, resulting in loss of all previous perception */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void ForgetAll();

	float GetYoungestStimulusAge(const AActor& Source) const;
	bool HasAnyActiveStimulus(const AActor& Source) const;
	bool HasAnyCurrentStimulus(const AActor& Source) const;
	bool HasActiveStimulus(const AActor& Source, FOSESenseID Sense) const;

#if WITH_GAMEPLAY_DEBUGGER
	virtual void DescribeSelfToGameplayDebugger(FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

	//----------------------------------------------------------------------//
	// blueprint interface
	//----------------------------------------------------------------------//
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void GetPerceivedHostileActors(TArray<AActor*>& OutActors) const;

	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void GetPerceivedHostileActorsBySense(const TSubclassOf<UOSESense> SenseToUse, TArray<AActor*>& OutActors) const;

	/** If SenseToUse is none all actors currently perceived in any way will get fetched */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void GetCurrentlyPerceivedActors(TSubclassOf<UOSESense> SenseToUse, TArray<AActor*>& OutActors) const;

	/** If SenseToUse is none all actors ever perceived in any way (and not forgotten yet) will get fetched */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void GetKnownPerceivedActors(TSubclassOf<UOSESense> SenseToUse, TArray<AActor*>& OutActors) const;
	
	/** Retrieves whatever has been sensed about given actor */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	bool GetActorsPerception(AActor* Actor, FOSEPerceptionBlueprintInfo& Info);

   /// Get the last sensed location for the given actor and sense. If senseToUse is none, all senses
   /// will be used. Returns false if we don't know anything about this actor.
   UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
   bool GetLastSensedActorLocation(AActor* actor, TSubclassOf<UOSESense> senseToUse, FVector& lastSensedLocation) const;


	/** Note that this works only if given sense has been already configured for
	 *	this component instance */
	UFUNCTION(BlueprintCallable, Category = "OSE|Perception")
	void SetSenseEnabled(TSubclassOf<UOSESense> SenseClass, const bool bEnable);

   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnActorPerceptionUpdated;

protected:
	FActorPerceptionContainer& GetPerceptualData() { return PerceptualData; }
	const FActorPerceptionContainer& GetPerceptualData() const { return PerceptualData; }

	/** called to clean up on owner's end play or destruction */
	virtual void CleanUp();

	void RemoveDeadData();

	/** Updates the stimulus entry in StimulusStore, if NewStimulus is more recent or stronger */
	virtual void RefreshStimulus(FOSEStimulus& StimulusStore, const FOSEStimulus& NewStimulus);

	/** @note no need to call super implementation, it's there just for some validity checking */
	virtual void HandleExpiredStimulus(FOSEStimulus& StimulusStore);
	
private:
	friend UOSEPerceptionSystem;

	void RegisterSenseConfig(UOSESenseConfig& SenseConfig, UOSEPerceptionSystem& OSEPerceptionSys);
	void StoreListenerId(FOSEPerceptionListenerID InListenerId) { PerceptionListenerId = InListenerId; }
	void SetMaxStimulusAge(FOSESenseID SenseId, float MaxAge);
};

