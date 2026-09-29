// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSEPerceptionTypes.h"
#include "OSEPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPerceptionTypes)

template<>
FOSESenseCounter FOSENamedID<FOSESenseCounter>::Counter = FOSESenseCounter();

template<>
FOSEPerceptionListenerCounter FOSEGenericID<FOSEPerceptionListenerCounter>::Counter = FOSEPerceptionListenerCounter();

//----------------------------------------------------------------------//
// FOSEStimulus
//----------------------------------------------------------------------//

// mind that this needs to be > 0 since checks are done if Age < FOSEStimulus::NeverHappenedAge
// @todo maybe should be a function (IsValidAge)
const float FOSEStimulus::NeverHappenedAge = FLT_MAX;

FOSEStimulus::FOSEStimulus(const UOSESense& Sense, float StimulusStrength, const FVector& InStimulusLocation, const FVector& InReceiverLocation, FResult Result, FName InStimulusTag)
	: Age(0.f)
	, ExpirationAge(FOSEStimulus::NeverHappenedAge)
	, Strength(Result == SensingSucceeded ? StimulusStrength : -1.f)
	, StimulusLocation(InStimulusLocation)
	, ReceiverLocation(InReceiverLocation)
	, Tag(InStimulusTag)
	, bWantsToNotifyOnlyOnValueChange(Sense.WantsUpdateOnlyOnPerceptionValueChange())
   , bWantsToNotifyOnExpired(Sense.WantsUpdateOnPerceptionExpire())
	, bSuccessfullySensed(Result == SensingSucceeded)
	, bExpired(false)
{
	Type = Sense.GetSenseID();
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
FString FOSEStimulus::GetDebugDescription() const
{
	if (IsValid() == false)
	{
		return TEXT("Uninitialized");
	}
	
	if (bExpired)
	{
		return FString::Printf(TEXT("%s Expired"), *Type.Name.ToString());
	}

	return FString::Printf(TEXT("%s %s %.1fs ago at %s")
		, *Type.Name.ToString()
		, bSuccessfullySensed ? TEXT("+") : TEXT("-")
		, Age
		, *StimulusLocation.ToString()
	);
}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

//----------------------------------------------------------------------//
// FOSEPerceptionListener
//----------------------------------------------------------------------//
const FOSEPerceptionListener FOSEPerceptionListener::NullListener;

FOSEPerceptionListener::FOSEPerceptionListener()
	: CachedLocation(FVector::ZeroVector)
	, CachedDirection(FVector::UpVector)
	, bHasStimulusToProcess(false)
	, ListenerID(FOSEPerceptionListenerID::InvalidID())
{

}

FOSEPerceptionListener::FOSEPerceptionListener(UOSEPerceptionComponent& InListener) 
	: Listener(&InListener)
	, CachedLocation(FVector::ZeroVector)
	, CachedDirection(FVector::UpVector)
	, bHasStimulusToProcess(false)
	, ListenerID(FOSEPerceptionListenerID::InvalidID())
{
	UpdateListenerProperties(InListener);
	ListenerID = InListener.GetListenerId();
}

void FOSEPerceptionListener::CacheLocation()
{
	if (Listener.IsValid())
	{
		Listener->GetLocationAndDirection(CachedLocation, CachedDirection);
	}
}

void FOSEPerceptionListener::UpdateListenerProperties(UOSEPerceptionComponent& InListener)
{
	verify(&InListener == Listener.Get());

	// using InListener rather then Listener to avoid slight overhead of TWeakObjectPtr
	TeamIdentifier = InListener.GetTeamIdentifier();
	Filter = InListener.GetPerceptionFilter();
}

void FOSEPerceptionListener::RegisterStimulus(AActor* Source, const FOSEStimulus& Stimulus)
{
	bHasStimulusToProcess = true;
	Listener->RegisterStimulus(Source, Stimulus);
}

void FOSEPerceptionListener::ProcessStimuli()
{
	ensure(bHasStimulusToProcess);
	Listener->ProcessStimuli();
	bHasStimulusToProcess = false;
}

FName FOSEPerceptionListener::GetBodyActorName() const 
{
	const AActor* OwnerActor = Listener.IsValid() ? Listener->GetBodyActor() : NULL;
	return OwnerActor ? OwnerActor->GetFName() : NAME_None;
}

uint32 FOSEPerceptionListener::GetBodyActorUniqueID() const
{
	const AActor* OwnerActor = Listener.IsValid() ? Listener->GetBodyActor() : nullptr;
	return OwnerActor ? OwnerActor->GetUniqueID() : FAISystem::InvalidUnsignedID;
}

const AActor* FOSEPerceptionListener::GetBodyActor() const 
{ 
	return Listener.IsValid() ? Listener->GetBodyActor() : NULL; 
}

const IGenericTeamAgentInterface* FOSEPerceptionListener::GetTeamAgent() const
{
	const UOSEPerceptionComponent* PercComponent = Listener.Get();
	if (PercComponent == NULL)
	{	// This could be NULL if the Listener is pending kill; in order to get the pointer ANYWAY, we'd need to use
		// Listener.Get(true) instead.  This issue was hit when using KillPawns cheat at the same moment that pawns
		// were spawning into the world.
		return NULL;
	}

	const AActor* OwnerActor = PercComponent->GetOwner();
	const IGenericTeamAgentInterface* OwnerTeamAgent = Cast<const IGenericTeamAgentInterface>(OwnerActor);
	return OwnerTeamAgent != NULL ? OwnerTeamAgent : Cast<const IGenericTeamAgentInterface>(PercComponent->GetBodyActor());
}


