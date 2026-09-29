// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_Prediction.h"
#include "OSEPerceptionListenerInterface.h"
#include "OSEPerceptionSystem.h"
#include "AIController.h"
#include "OSEPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_Prediction)

UOSESense_Prediction::UOSESense_Prediction(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
}

float UOSESense_Prediction::Update()
{
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();

	for (const FOSEPredictionEvent& Event : RegisteredEvents)
	{
		if (Event.Requestor != NULL && Event.PredictedActor != NULL)
		{
			IOSEPerceptionListenerInterface* PerceptionListener = Cast<IOSEPerceptionListenerInterface>(Event.Requestor);
			if (PerceptionListener != NULL)
			{
				UOSEPerceptionComponent* PerceptionComponent = PerceptionListener->GetOSEPerceptionComponent();
				if (PerceptionComponent != NULL && ListenersMap.Contains(PerceptionComponent->GetListenerId()))
				{
					// this has to succeed, will assert a failure
					FOSEPerceptionListener& Listener = ListenersMap[PerceptionComponent->GetListenerId()];

					if (Listener.HasSense(GetSenseID()))
					{
						// calculate the prediction here:
						const FVector PredictedLocation = Event.PredictedActor->GetActorLocation() + Event.PredictedActor->GetVelocity() * Event.TimeToPredict;

						Listener.RegisterStimulus(Event.PredictedActor, FOSEStimulus(*this, 1.f, PredictedLocation, Listener.CachedLocation));
					}
				}
			}
		}
	}

	RegisteredEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UOSESense_Prediction::RegisterEvent(const FOSEPredictionEvent& Event)
{
	RegisteredEvents.Add(Event);
	RequestImmediateUpdate();
}

void UOSESense_Prediction::RequestControllerPredictionEvent(AAIController* Requestor, AActor* PredictedActor, float PredictionTime)
{
	UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(Requestor);
	if (PerceptionSystem)
	{
		FOSEPredictionEvent Event(Requestor, PredictedActor, PredictionTime);
		PerceptionSystem->OnEvent(Event);
	}
}

void UOSESense_Prediction::RequestPawnPredictionEvent(APawn* Requestor, AActor* PredictedActor, float PredictionTime)
{
	UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(Requestor);
	if (PerceptionSystem && Requestor->GetController())
	{
		FOSEPredictionEvent Event(Requestor->GetController(), PredictedActor, PredictionTime);
		PerceptionSystem->OnEvent(Event);
	}
}

