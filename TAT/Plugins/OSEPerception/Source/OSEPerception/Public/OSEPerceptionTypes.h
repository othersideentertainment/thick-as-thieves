// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AITypes.h"
#include "GenericTeamAgentInterface.h"
#include "OSEPerceptionTypes.generated.h"

class UOSEPerceptionComponent;
class UOSESense;

/**
 *	TCounter needs to supply following functions:
 *		default constructor
 *		typedef X Type; where X is an integer type to be used as ID's internal type
 *		TCounter::Type GetNextAvailableID() - returns next available ID and advances the internal counter
 *		uint32 GetSize() const - returns number of unique IDs created so far
 *		OnIndexForced(TCounter::Type Index) - called when given Index has been force-used. Counter may need to update "next available ID"
 */

template<typename TCounter>
struct FOSENamedID
{
   const typename TCounter::Type Index;
   const FName Name;
private:
   static OSEPERCEPTION_API TCounter Counter;
protected:
   static TCounter& GetCounter()
   {
      return Counter;
   }

   // back-door for forcing IDs
   FOSENamedID(const FName& InName, typename TCounter::Type InIndex)
      : Index(InIndex), Name(InName)
   {
      GetCounter().OnIndexForced(InIndex);
   }

public:
   FOSENamedID(const FName& InName)
      : Index(GetCounter().GetNextAvailableID()), Name(InName)
   {}

   FOSENamedID(const FOSENamedID& Other)
      : Index(Other.Index), Name(Other.Name)
   {}

   FOSENamedID& operator=(const FOSENamedID& Other)
   {
      new(this) FOSENamedID(Other);
      return *this;
   }

   FOSENamedID()
      : Index(typename TCounter::Type(-1)), Name(TEXT("Invalid"))
   {}

   operator typename TCounter::Type() const { return Index; }
   bool IsValid() const { return Index != InvalidID().Index; }

   static uint32 GetSize() { return GetCounter().GetSize(); }

   static FOSENamedID<TCounter> InvalidID()
   {
      static const FOSENamedID<TCounter> InvalidIDInstance;
      return InvalidIDInstance;
   }
};

template<typename TCounter>
struct FOSEGenericID
{
   const typename TCounter::Type Index;
private:
   static OSEPERCEPTION_API TCounter Counter;
protected:
   static TCounter& GetCounter()
   {
      return Counter;
   }

   FOSEGenericID(typename TCounter::Type InIndex)
      : Index(InIndex)
   {}

public:
   FOSEGenericID(const FOSEGenericID& Other)
      : Index(Other.Index)
   {}

   FOSEGenericID& operator=(const FOSEGenericID& Other)
   {
      new(this) FOSEGenericID(Other);
      return *this;
   }

   FOSEGenericID()
      : Index(typename TCounter::Type(-1))
   {}

   static FOSEGenericID GetNextID() { return FOSEGenericID(GetCounter().GetNextAvailableID()); }

   operator typename TCounter::Type() const { return Index; }
   bool IsValid() const { return Index != InvalidID().Index; }

   static uint32 GetSize() { return GetCounter().GetSize(); }

   static FOSEGenericID<TCounter> InvalidID()
   {
      static const FOSEGenericID<TCounter> InvalidIDInstance;
      return InvalidIDInstance;
   }

   friend FORCEINLINE uint32 GetTypeHash(const FOSEGenericID& ID)
   {
      return GetTypeHash(ID.Index);
   }
};

template<typename TCounterType>
struct FOSEBasicCounter
{
   typedef TCounterType Type;
protected:
   Type NextAvailableID;
public:
   FOSEBasicCounter() : NextAvailableID(Type(0)) {}
   Type GetNextAvailableID() { return NextAvailableID++; }
   uint32 GetSize() const { return uint32(NextAvailableID); }
   void OnIndexForced(Type ForcedIndex) { NextAvailableID = FMath::Max<Type>(ForcedIndex + 1, NextAvailableID); }
};

//////////////////////////////////////////////////////////////////////////
struct OSEPERCEPTION_API FOSESenseCounter : FOSEBasicCounter<uint8>
{};
typedef FOSENamedID<FOSESenseCounter> FOSESenseID;

//////////////////////////////////////////////////////////////////////////
struct OSEPERCEPTION_API FOSEPerceptionListenerCounter : FOSEBasicCounter<uint32>
{};
typedef FOSEGenericID<FOSEPerceptionListenerCounter> FOSEPerceptionListenerID;

//////////////////////////////////////////////////////////////////////////

UENUM()
enum class EOSESenseNotifyType : uint8
{
	/** Continuous update whenever target is perceived. */
	OnEveryPerception,
	/** From "visible" to "not visible" or vice versa. */
	OnPerceptionChange,
	/** From "visible" to "not visible" or vice versa, and when it expires. */
	OnPerceptionChangeAndExpire,

};

struct FOSEPerceptionChannelAllowList
{
	typedef int32 FFlagsContainer;

	FFlagsContainer AcceptedChannelsMask;

	// by default accept all
	FOSEPerceptionChannelAllowList() : AcceptedChannelsMask()
	{}

	void Clear()
	{
		AcceptedChannelsMask = 0;
	}

	bool IsEmpty() const
	{
		return (AcceptedChannelsMask == 0);
	}

	FORCEINLINE FOSEPerceptionChannelAllowList& FilterOutChannel(FOSESenseID Channel)
	{
		AcceptedChannelsMask &= ~(1 << Channel);
		return *this;
	}

	FORCEINLINE_DEBUGGABLE FOSEPerceptionChannelAllowList& AcceptChannel(FOSESenseID Channel)
	{
		AcceptedChannelsMask |= (1 << Channel);
		return *this;
	}

	FORCEINLINE bool ShouldRespondToChannel(FOSESenseID Channel) const
	{
		return (AcceptedChannelsMask & (1 << Channel)) != 0;
	}

	FORCEINLINE FOSEPerceptionChannelAllowList& MergeFilterIn(const FOSEPerceptionChannelAllowList& OtherFilter)
	{
		AcceptedChannelsMask |= OtherFilter.AcceptedChannelsMask;
		return *this;
	}

	FORCEINLINE FFlagsContainer GetAcceptedChannelsMask() const 
	{ 
		return AcceptedChannelsMask;
	}

	struct FConstIterator
	{
	private:
		FFlagsContainer RemainingChannelsToTest;
		const FOSEPerceptionChannelAllowList& AllowList;
		int32 CurrentIndex;

	public:
		FConstIterator(const FOSEPerceptionChannelAllowList& InAllowList)
			: RemainingChannelsToTest((FFlagsContainer)-1)
			, AllowList(InAllowList)
			, CurrentIndex(INDEX_NONE)
		{
			FindNextAcceptedChannel();
		}

		FORCEINLINE void FindNextAcceptedChannel()
		{
			const FFlagsContainer& Flags = AllowList.GetAcceptedChannelsMask();

			while ((RemainingChannelsToTest & Flags) != 0 && ((1 << ++CurrentIndex) | Flags) == 0)
			{
				RemainingChannelsToTest &= ~(1 << CurrentIndex);
			}
		}

		FORCEINLINE explicit operator bool() const
		{
			return (RemainingChannelsToTest & AllowList.GetAcceptedChannelsMask()) != 0;
		}

		FORCEINLINE int32 operator*() const
		{
			return CurrentIndex;
		}

		FORCEINLINE void operator++()
		{
			// mark "old" index as already used
			RemainingChannelsToTest &= ~(1 << CurrentIndex);
			FindNextAcceptedChannel();
		}
	};
};

USTRUCT(BlueprintType)
struct OSEPERCEPTION_API FOSEStimulus
{
	GENERATED_USTRUCT_BODY()

	static const float NeverHappenedAge;

	enum FResult
	{
		SensingSucceeded,
		SensingFailed
	};

protected:
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	float Age;

	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	float ExpirationAge;
public:
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	float Strength;
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	FVector StimulusLocation;
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	FVector ReceiverLocation;
	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	FName Tag;

	FOSESenseID Type;

protected:
	uint32 bWantsToNotifyOnlyOnValueChange : 1;
   uint32 bWantsToNotifyOnExpired : 1;

	UPROPERTY(BlueprintReadWrite, Category = "OSE|Perception")
	uint32 bSuccessfullySensed:1; // currently used only for marking failed sight tests

	/** this means the stimulus was originally created with a "time limit" and this time has passed. 
	 *	Expiration also results in calling MarkNoLongerSensed */
	uint32 bExpired:1;	
	
public:
	
	/** this is the recommended constructor. Use others if you know what you're doing. */
	FOSEStimulus(const UOSESense& Sense, float StimulusStrength, const FVector& InStimulusLocation, const FVector& InReceiverLocation, FResult Result = SensingSucceeded, FName InStimulusTag = NAME_None);

	// default constructor
	FOSEStimulus()
		: Age(NeverHappenedAge), ExpirationAge(NeverHappenedAge), Strength(-1.f), StimulusLocation(FAISystem::InvalidLocation)
		, ReceiverLocation(FAISystem::InvalidLocation)
      , Tag(NAME_None)
      , Type(FOSESenseID::InvalidID())
      , bWantsToNotifyOnlyOnValueChange(false)
      , bWantsToNotifyOnExpired(false)
		, bSuccessfullySensed(false)
      , bExpired(false)
	{}

	FOSEStimulus& SetExpirationAge(float InExpirationAge) { ExpirationAge = InExpirationAge; return *this; }
	FOSEStimulus& SetStimulusAge(float StimulusAge) { Age = StimulusAge; return *this; }
	FOSEStimulus& SetWantsNotifyOnlyOnValueChange(bool InEnable) { bWantsToNotifyOnlyOnValueChange = InEnable; return *this; }
	
	FORCEINLINE float GetAge() const { return Strength > 0 ? Age : NeverHappenedAge; }
	/** @return false when this stimulus is no longer valid, when it is Expired */
	FORCEINLINE bool AgeStimulus(float ConstPerceptionAgingRate) 
	{ 
		Age += ConstPerceptionAgingRate; 
		return Age < ExpirationAge;
	}
	FORCEINLINE bool WasSuccessfullySensed() const { return bSuccessfullySensed; }
	FORCEINLINE bool IsExpired() const { return bExpired; }
	FORCEINLINE void MarkNoLongerSensed() { bSuccessfullySensed = false; }
	FORCEINLINE void MarkExpired() { bExpired = true; MarkNoLongerSensed(); }
	FORCEINLINE bool IsActive() const { return WasSuccessfullySensed() == true && IsValid(); }
	FORCEINLINE bool WantsToNotifyOnlyOnPerceptionChange() const { return bWantsToNotifyOnlyOnValueChange; }
   FORCEINLINE bool WantsToNotifyOnExpired() const { return bWantsToNotifyOnExpired; }
	FORCEINLINE bool IsValid() const { return Type != FOSESenseID::InvalidID() && GetAge() < NeverHappenedAge; }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FString GetDebugDescription() const;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
};

USTRUCT(BlueprintType)
struct OSEPERCEPTION_API FOSESenseAffiliationFilter
{
	GENERATED_USTRUCT_BODY()

	FOSESenseAffiliationFilter()
		: bDetectEnemies(false)
		, bDetectNeutrals(false)
		, bDetectFriendlies(false) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sense")
	uint32 bDetectEnemies : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sense")
	uint32 bDetectNeutrals : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sense")
	uint32 bDetectFriendlies : 1;
	
	uint8 GetAsFlags() const { return (bDetectEnemies << ETeamAttitude::Hostile) | (bDetectNeutrals << ETeamAttitude::Neutral) | (bDetectFriendlies << ETeamAttitude::Friendly); }
	FORCEINLINE bool ShouldDetectAll() const { return (bDetectEnemies && bDetectNeutrals && bDetectFriendlies); }

	static FORCEINLINE uint8 DetectAllFlags() { return (1 << ETeamAttitude::Hostile) | (1 << ETeamAttitude::Neutral) | (1 << ETeamAttitude::Friendly); }

	static bool ShouldSenseTeam(FGenericTeamId TeamA, FGenericTeamId TeamB, uint8 AffiliationFlags)
	{
		static const uint8 AllFlags = DetectAllFlags();
		return AffiliationFlags == AllFlags || ((1 << FGenericTeamId::GetAttitude(TeamA, TeamB)) & AffiliationFlags);
	}

	static bool ShouldSenseTeam(const IGenericTeamAgentInterface* TeamAgent, const AActor& TargetActor, uint8 AffiliationFlags)
	{
		static const uint8 AllFlags = DetectAllFlags();
		return AffiliationFlags == AllFlags 
			|| (TeamAgent == nullptr ? (AffiliationFlags & (1 << ETeamAttitude::Neutral)) : ((1 << TeamAgent->GetTeamAttitudeTowards(TargetActor)) & AffiliationFlags));
	}
};

/** Should contain only cached information common to all senses. Sense-specific data needs to be stored by senses themselves */
struct OSEPERCEPTION_API FOSEPerceptionListener
{
	TWeakObjectPtr<UOSEPerceptionComponent> Listener;

	FOSEPerceptionChannelAllowList Filter;

	FVector CachedLocation;
	FVector CachedDirection;

	FGenericTeamId TeamIdentifier;

private:
	uint32 bHasStimulusToProcess : 1;

	FOSEPerceptionListenerID ListenerID;

	FOSEPerceptionListener();
public:
	FOSEPerceptionListener(UOSEPerceptionComponent& InListener);

	void UpdateListenerProperties(UOSEPerceptionComponent& Listener);

	bool operator==(const UOSEPerceptionComponent* Other) const { return Listener.Get() == Other; }
	bool operator==(const FOSEPerceptionListener& Other) const { return Listener == Other.Listener; }

	void CacheLocation();

	void RegisterStimulus(AActor* Source, const FOSEStimulus& Stimulus);

	FORCEINLINE bool HasAnyNewStimuli() const { return bHasStimulusToProcess; }
	void ProcessStimuli();

	FORCEINLINE bool HasSense(FOSESenseID SenseID) const { return Filter.ShouldRespondToChannel(SenseID); }

	// used to remove "dead" listeners
	static const FOSEPerceptionListener NullListener;

	FORCEINLINE FOSEPerceptionListenerID GetListenerID() const { return ListenerID; }

	FName GetBodyActorName() const;
	uint32 GetBodyActorUniqueID() const;

	/** Returns pointer to the actor representing this listener's physical body */
	const AActor* GetBodyActor() const;

	const IGenericTeamAgentInterface* GetTeamAgent() const;

private:
	friend class UOSEPerceptionSystem;
	FORCEINLINE void SetListenerID(FOSEPerceptionListenerID InListenerID) { ListenerID = InListenerID; }
	FORCEINLINE void MarkForStimulusProcessing() { bHasStimulusToProcess = true; }
};

struct OSEPERCEPTION_API FOSEPerceptionStimuliSource
{
	TWeakObjectPtr<AActor> SourceActor;
	FOSEPerceptionChannelAllowList RelevantSenses;
};

namespace OSEPerception
{
	typedef TMap<FOSEPerceptionListenerID, FOSEPerceptionListener> FListenerMap;
}
