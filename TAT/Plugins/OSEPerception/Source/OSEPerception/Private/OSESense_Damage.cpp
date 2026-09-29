// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_Damage.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "OSEPerceptionListenerInterface.h"
#include "OSEPerceptionSystem.h"
#include "OSEPerceptionComponent.h"
#include "OSESenseEvent_Damage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_Damage)

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
FOSEDamageEvent::FOSEDamageEvent()
	: Amount(1.f), Location(FAISystem::InvalidLocation), HitLocation(FAISystem::InvalidLocation)
	, DamagedActor(nullptr), Instigator(nullptr), Tag(NAME_None)
{}

FOSEDamageEvent::FOSEDamageEvent(AActor* InDamagedActor, AActor* InInstigator, float DamageAmount, const FVector& EventLocation, const FVector& InHitLocation/* = FAISystem::InvalidLocation*/, FName InTag/* = NAME_None*/)
	: Amount(DamageAmount), Location(EventLocation), HitLocation(InHitLocation), DamagedActor(InDamagedActor), Instigator(InInstigator), Tag(InTag)
{
	Compile();
}

void FOSEDamageEvent::Compile()
{
	if (DamagedActor == nullptr)
	{
		// nothing to do here, this event is invalid
		return;
	}

	const bool bHitLocationValid = FAISystem::IsValidLocation(HitLocation);
	const bool bEventLocationValid = FAISystem::IsValidLocation(Location);

	if (bHitLocationValid != bEventLocationValid)
	{
		if (bHitLocationValid)
		{
			HitLocation = Location;
		}
		else
		{
			Location = HitLocation;
		}
	}
	// both invalid
	else if ((bHitLocationValid || bEventLocationValid) == false)
	{
		HitLocation = Location = DamagedActor->GetActorLocation();
	}

}

IOSEPerceptionListenerInterface* FOSEDamageEvent::GetDamagedActorAsPerceptionListener() const
{
	IOSEPerceptionListenerInterface* Listener = nullptr;
	if (DamagedActor)
	{
		Listener = Cast<IOSEPerceptionListenerInterface>(DamagedActor);
		if (Listener == nullptr)
		{
			APawn* ListenerAsPawn = Cast<APawn>(DamagedActor);
			if (ListenerAsPawn)
			{
				Listener = Cast<IOSEPerceptionListenerInterface>(ListenerAsPawn->GetController());
			}
		}
	}
	return Listener;
}
//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
UOSESense_Damage::UOSESense_Damage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NotifyType = EOSESenseNotifyType::OnPerceptionChangeAndExpire;
}

float UOSESense_Damage::Update()
{
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();

	for (const FOSEDamageEvent& Event : RegisteredEvents)
	{
		IOSEPerceptionListenerInterface* PerceptionListener = Event.GetDamagedActorAsPerceptionListener();
		if (PerceptionListener != nullptr)
		{
			UOSEPerceptionComponent* PerceptionComponent = PerceptionListener->GetOSEPerceptionComponent();
			if (PerceptionComponent != nullptr && PerceptionComponent->GetListenerId().IsValid())
			{
				// this has to succeed, will assert a failure
				FOSEPerceptionListener& Listener = ListenersMap[PerceptionComponent->GetListenerId()];

				if (Listener.HasSense(GetSenseID()))
				{
					Listener.RegisterStimulus(Event.Instigator, FOSEStimulus(*this, Event.Amount, Event.Location, Event.HitLocation, FOSEStimulus::SensingSucceeded, Event.Tag));
				}
			}
		}
	}

	RegisteredEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UOSESense_Damage::RegisterEvent(const FOSEDamageEvent& Event)
{
	if (Event.IsValid())
	{
		RegisteredEvents.Add(Event);

		RequestImmediateUpdate();
	}
}

void UOSESense_Damage::RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent)
{
	UOSESenseEvent_Damage* DamageEvent = Cast<UOSESenseEvent_Damage>(&PerceptionEvent);
	ensure(DamageEvent);
	if (DamageEvent)
	{
		RegisterEvent(DamageEvent->GetDamageEvent());
	}
}

void UOSESense_Damage::ReportDamageEvent(UObject* WorldContextObject, AActor* DamagedActor, AActor* Instigator, float DamageAmount, FVector EventLocation, FVector HitLocation, FName Tag/* = NAME_None*/)
{
	UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(WorldContextObject);
	if (PerceptionSystem)
	{
		FOSEDamageEvent Event(DamagedActor, Instigator, DamageAmount, EventLocation, HitLocation, Tag);
		PerceptionSystem->OnEvent(Event);
	}
}

