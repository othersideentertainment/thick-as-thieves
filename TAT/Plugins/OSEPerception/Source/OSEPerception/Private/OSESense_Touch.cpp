// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_Touch.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "OSEPerceptionListenerInterface.h"
#include "OSEPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_Touch)


IOSEPerceptionListenerInterface* FOSETouchEvent::GetTouchedActorAsPerceptionListener() const
{
	IOSEPerceptionListenerInterface* Listener = nullptr;
	if (TouchReceiver)
	{
		Listener = Cast<IOSEPerceptionListenerInterface>(TouchReceiver);
		if (Listener == nullptr)
		{
			APawn* ListenerAsPawn = Cast<APawn>(TouchReceiver);
			if (ListenerAsPawn)
			{
				Listener = Cast<IOSEPerceptionListenerInterface>(ListenerAsPawn->GetController());
			}
		}
	}
	return Listener;
}

UOSESense_Touch::UOSESense_Touch(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
}

float UOSESense_Touch::Update()
{
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();

	for (const FOSETouchEvent& Event : RegisteredEvents)
	{
		if (Event.TouchReceiver != NULL && Event.OtherActor != NULL)
		{
			IOSEPerceptionListenerInterface* PerceptionListener = Event.GetTouchedActorAsPerceptionListener();

			// OSE BEGIN -- Try to get the listener via the controller. This is inspired
			// by AISense_Damage::GetDamagedActorAsPerceptionListener.
			if (PerceptionListener == nullptr)
			{
				APawn* ListenerAsPawn = Cast<APawn>(Event.TouchReceiver);
				if (ListenerAsPawn)
				{
					AController* controller = ListenerAsPawn->GetController();
					PerceptionListener = Cast<IOSEPerceptionListenerInterface>(controller);
				}
			}
			// OSE END

			if (PerceptionListener != NULL)
			{
				UOSEPerceptionComponent* PerceptionComponent = PerceptionListener->GetOSEPerceptionComponent();
				if (PerceptionComponent != NULL && ListenersMap.Contains(PerceptionComponent->GetListenerId()))
				{
					// this has to succeed, will assert a failure
					FOSEPerceptionListener& Listener = ListenersMap[PerceptionComponent->GetListenerId()];
					if (Listener.HasSense(GetSenseID()))
					{
						Listener.RegisterStimulus(Event.OtherActor, FOSEStimulus(*this, 1.f, Event.Location, Event.Location));
					}
				}
			}
		}
	}

	RegisteredEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UOSESense_Touch::RegisterEvent(const FOSETouchEvent& Event)
{
	RegisteredEvents.Add(Event);

	RequestImmediateUpdate();
}

void UOSESense_Touch::ReportTouchEvent(UObject* WorldContextObject, AActor* TouchReceiver, AActor* OtherActor, FVector Location)
{
	UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(WorldContextObject);
	if (PerceptionSystem)
	{
		FOSETouchEvent Event(TouchReceiver, OtherActor, Location);
		PerceptionSystem->OnEvent(Event);
	}
}

