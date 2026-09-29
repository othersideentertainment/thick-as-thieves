// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense_Hearing.h"
#include "OSEPerceptionSystem.h"
#include "OSEPerceptionComponent.h"
#include "OSESenseConfig_Hearing.h"
#include "OSESenseEvent_Hearing.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense_Hearing)

//----------------------------------------------------------------------//
// FOSENoiseEvent
//----------------------------------------------------------------------//
FOSENoiseEvent::FOSENoiseEvent()
	: Age(0.f), NoiseLocation(FAISystem::InvalidLocation), Loudness(1.f), MaxRange(0.f)
	, Instigator(nullptr), Tag(NAME_None), TeamIdentifier(FGenericTeamId::NoTeam)
{
}

FOSENoiseEvent::FOSENoiseEvent(AActor* InInstigator, const FVector& InNoiseLocation, float InLoudness, float InMaxRange, FName InTag)
	: Age(0.f), NoiseLocation(InNoiseLocation), Loudness(InLoudness), MaxRange(InMaxRange)
	, Instigator(InInstigator), Tag(InTag), TeamIdentifier(FGenericTeamId::NoTeam)
{
	Compile();
}

void FOSENoiseEvent::Compile()
{
	TeamIdentifier = FGenericTeamId::GetTeamIdentifier(Instigator);
	if (FAISystem::IsValidLocation(NoiseLocation) == false && Instigator != nullptr)
	{
		NoiseLocation = Instigator->GetActorLocation();
	}
}

//----------------------------------------------------------------------//
// FDigestedHearingProperties
//----------------------------------------------------------------------//
UOSESense_Hearing::FDigestedHearingProperties::FDigestedHearingProperties(const UOSESenseConfig_Hearing& SenseConfig)
{
	HearingRangeSq = FMath::Square(SenseConfig.HearingRange);
	LoSHearingRangeSq = FMath::Square(SenseConfig.LoSHearingRange);
	AffiliationFlags = SenseConfig.DetectionByAffiliation.GetAsFlags();
	bUseLoSHearing = SenseConfig.bUseLoSHearing;
}

UOSESense_Hearing::FDigestedHearingProperties::FDigestedHearingProperties()
	: HearingRangeSq(-1.f), LoSHearingRangeSq(-1.f), AffiliationFlags(-1), bUseLoSHearing(false)
{

}

//----------------------------------------------------------------------//
// UOSESense_Hearing
//----------------------------------------------------------------------//
UOSESense_Hearing::UOSESense_Hearing(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		OnNewListenerDelegate.BindUObject(this, &UOSESense_Hearing::OnNewListenerImpl);
		OnListenerUpdateDelegate.BindUObject(this, &UOSESense_Hearing::OnListenerUpdateImpl);
		OnListenerRemovedDelegate.BindUObject(this, &UOSESense_Hearing::OnListenerRemovedImpl);
	}
}

void UOSESense_Hearing::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		RegisterMakeNoiseDelegate();
	}
}

void UOSESense_Hearing::RegisterMakeNoiseDelegate()
{
	AActor::SetMakeNoiseDelegate(FMakeNoiseDelegate::CreateStatic(&UOSEPerceptionSystem::MakeNoiseImpl));
}

void UOSESense_Hearing::ReportNoiseEvent(UObject* WorldContextObject, FVector NoiseLocation, float Loudness, AActor* Instigator, float MaxRange, FName Tag)
{
	UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(WorldContextObject);
	if (PerceptionSystem)
	{
		FOSENoiseEvent Event(Instigator, NoiseLocation, Loudness, MaxRange, Tag);
		PerceptionSystem->OnEvent(Event);
	}
}

void UOSESense_Hearing::OnNewListenerImpl(const FOSEPerceptionListener& NewListener)
{
	UOSEPerceptionComponent* ListenerPtr = NewListener.Listener.Get();
	check(ListenerPtr);
	const UOSESenseConfig_Hearing* SenseConfig = Cast<const UOSESenseConfig_Hearing>(ListenerPtr->GetSenseConfig(GetSenseID()));
	check(SenseConfig);
	const FDigestedHearingProperties PropertyDigest(*SenseConfig);
	DigestedProperties.Add(NewListener.GetListenerID(), PropertyDigest);
}

void UOSESense_Hearing::OnListenerUpdateImpl(const FOSEPerceptionListener& UpdatedListener)
{
	// @todo add updating code here
	const FOSEPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();
	
	if (UpdatedListener.HasSense(GetSenseID()))
	{
		const UOSESenseConfig_Hearing* SenseConfig = Cast<const UOSESenseConfig_Hearing>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
		check(SenseConfig);
		FDigestedHearingProperties& PropertiesDigest = DigestedProperties.FindOrAdd(ListenerID);
		PropertiesDigest = FDigestedHearingProperties(*SenseConfig);
	}
	else
	{
		DigestedProperties.Remove(ListenerID);
	}
}

void UOSESense_Hearing::OnListenerRemovedImpl(const FOSEPerceptionListener& UpdatedListener)
{
	DigestedProperties.FindAndRemoveChecked(UpdatedListener.GetListenerID());
}

float UOSESense_Hearing::Update()
{
	OSEPerception::FListenerMap& ListenersMap = *GetListeners();
	UOSEPerceptionSystem* PerseptionSys = GetPerceptionSystem();
	const float SpeedOfSoundSqScalar = SpeedOfSoundSq > 0.f ? 1.f / SpeedOfSoundSq : 0.f;

	for (OSEPerception::FListenerMap::TIterator ListenerIt(ListenersMap); ListenerIt; ++ListenerIt)
	{
		FOSEPerceptionListener& Listener = ListenerIt->Value;
		
		if (Listener.HasSense(GetSenseID()) == false)
		{
			// skip listeners not interested in this sense
			continue;
		}

		const FDigestedHearingProperties& PropDigest = DigestedProperties[Listener.GetListenerID()];

		for (const FOSENoiseEvent& Event : NoiseEvents)
		{
			const float ClampedLoudness = FMath::Max(0.f, Event.Loudness);
			const float DistToSoundSquared = FVector::DistSquared(Event.NoiseLocation, Listener.CachedLocation);
			
			// Limit by loudness modified squared range (this is the old behavior)
			if (DistToSoundSquared > PropDigest.HearingRangeSq * FMath::Square(ClampedLoudness))
			{
				continue;
			}
			// Limit by max range
			else if (Event.MaxRange > 0.f && DistToSoundSquared > FMath::Square(Event.MaxRange * ClampedLoudness))
			{
				continue;
			}

			if (FOSESenseAffiliationFilter::ShouldSenseTeam(Listener.TeamIdentifier, Event.TeamIdentifier, PropDigest.AffiliationFlags) == false)
			{
				continue;
			}
			// calculate delay and fake it with Age
			const float Delay = FMath::Sqrt(DistToSoundSquared * SpeedOfSoundSqScalar);
			// pass over to listener to process 			
			PerseptionSys->RegisterDelayedStimulus(Listener.GetListenerID(), Delay, Event.Instigator
				, FOSEStimulus(*this, ClampedLoudness, Event.NoiseLocation, Listener.CachedLocation, FOSEStimulus::SensingSucceeded, Event.Tag) );
		}
	}

	NoiseEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UOSESense_Hearing::RegisterEvent(const FOSENoiseEvent& Event)
{
	NoiseEvents.Add(Event);

	RequestImmediateUpdate();
}

void UOSESense_Hearing::RegisterEventsBatch(const TArray<FOSENoiseEvent>& Events)
{
	NoiseEvents.Append(Events);

	RequestImmediateUpdate();
}

void UOSESense_Hearing::RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent)
{
	UOSESenseEvent_Hearing* HearingEvent = Cast<UOSESenseEvent_Hearing>(&PerceptionEvent);
	ensure(HearingEvent);
	if (HearingEvent)
	{
		RegisterEvent(HearingEvent->GetNoiseEvent());
	}
}

