// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_Team.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_Team)

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
FOSETeamStimulusEvent::FOSETeamStimulusEvent(AActor* InBroadcaster, AActor* InEnemy, const FVector& InLastKnowLocation, float EventRange, float PassedInfoAge, float InStrength)
	: LastKnowLocation(InLastKnowLocation), RangeSq(FMath::Square(EventRange)), InformationAge(PassedInfoAge), Strength(InStrength), Broadcaster(InBroadcaster), Enemy(InEnemy)
{
	CacheBroadcastLocation();

	TeamIdentifier = FGenericTeamId::GetTeamIdentifier(InBroadcaster);
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
UOSESense_Team::UOSESense_Team(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
}

float UOSESense_Team::Update()
{
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();
	
	for (OSEPerception::FListenerMap::TIterator ListenerIt(ListenersMap); ListenerIt; ++ListenerIt)
	{
		FOSEPerceptionListener& Listener = ListenerIt->Value;

		if (Listener.HasSense(GetSenseID()) == false)
		{
			// skip listeners not interested in this sense
			continue;
		}

		for (const FOSETeamStimulusEvent& Event : RegisteredEvents)
		{
			// @todo implement some kind of TeamIdentifierType that would supply comparison operator 
			if (Listener.TeamIdentifier != Event.TeamIdentifier 
				|| FVector::DistSquared(Event.GetBroadcastLocation(), Listener.CachedLocation) > Event.RangeSq)
			{
				continue;
			}
			
			Listener.RegisterStimulus(Event.Enemy, FOSEStimulus(*this, Event.Strength, Event.LastKnowLocation, Event.GetBroadcastLocation(), FOSEStimulus::SensingSucceeded).SetStimulusAge(Event.InformationAge));
		}
	}

	RegisteredEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UOSESense_Team::RegisterEvent(const FOSETeamStimulusEvent& Event)
{
	RegisteredEvents.Add(Event);

	RequestImmediateUpdate();
}

