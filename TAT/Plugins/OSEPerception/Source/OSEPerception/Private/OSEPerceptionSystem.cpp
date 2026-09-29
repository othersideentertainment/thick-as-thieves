// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSEPerceptionSystem.h"
#include "EngineGlobals.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "AISystem.h"
#include "OSESense_Hearing.h"
#include "OSESenseConfig.h"
#include "OSEPerceptionComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "OSESenseEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPerceptionSystem)

DECLARE_CYCLE_STAT(TEXT("Perception System"),STAT_AI_PerceptionSys,STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception System - Process Stim"),STAT_AI_Perception_ProcessStim,STATGROUP_AI);

DEFINE_LOG_CATEGORY(LogOSEPerception);

//----------------------------------------------------------------------//
// UOSESenseConfig
//----------------------------------------------------------------------//
FOSESenseID UOSESenseConfig::GetSenseID() const
{
	TSubclassOf<UOSESense> SenseClass = GetSenseImplementation();
	return UOSESense::GetSenseID(SenseClass);
}

//----------------------------------------------------------------------//
// UOSEPerceptionSystem
//----------------------------------------------------------------------//
UOSEPerceptionSystem::UOSEPerceptionSystem()
	: PerceptionAgingRate(0.3f)
	, bHandlePawnNotification(false)
	, NextStimuliAgingTick(0.f)
	, CurrentTime(0.f)
{
	StimuliSourceEndPlayDelegate.BindDynamic(this, &UOSEPerceptionSystem::OnPerceptionStimuliSourceEndPlay);
}

FOSESenseID UOSEPerceptionSystem::RegisterSenseClass(TSubclassOf<UOSESense> SenseClass)
{
	check(SenseClass);
	FOSESenseID SenseID = UOSESense::GetSenseID(SenseClass);
	if (SenseID.IsValid() == false)
	{
		UOSESense* SenseCDO = GetMutableDefault<UOSESense>(SenseClass);
		SenseID = SenseCDO->UpdateSenseID();

		if (SenseID.IsValid() == false)
		{
			// @todo log a message here
			return FOSESenseID::InvalidID();
		}
	}

	if (SenseID.Index >= Senses.Num())
	{
		const int32 ItemsToAdd = SenseID.Index - Senses.Num() + 1;
		Senses.AddZeroed(ItemsToAdd);
	}

	if (Senses[SenseID] == nullptr)
	{
		Senses[SenseID] = NewObject<UOSESense>(this, SenseClass);
		check(Senses[SenseID]);
		bHandlePawnNotification |= Senses[SenseID]->ShouldAutoRegisterAllPawnsAsSources() || Senses[SenseID]->WantsNewPawnNotification();

		if (Senses[SenseID]->ShouldAutoRegisterAllPawnsAsSources())
		{
			UWorld* World = GetWorld();
			if (World->HasBegunPlay())
			{
				// this @hack is required due to UOSEPerceptionSystem::RegisterSenseClass
				// being potentially called from UOSEPerceptionComponent::OnRegister
				// and at that point UWorld might not have registered 
				// the pawn related to given UOSEPerceptionComponent.
				World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UOSEPerceptionSystem::RegisterAllPawnsAsSourcesForSense, SenseID));
			}
			// otherwise it will get called in StartPlay()
		}

		// make senses v-log to perception system's log
		REDIRECT_OBJECT_TO_VLOG(Senses[SenseID], this);
		UE_VLOG(this, LogOSEPerception, Log, TEXT("Registering sense %s"), *Senses[SenseID]->GetName());
	}

	return SenseID;
}

TStatId UOSEPerceptionSystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOSEPerceptionSystem, STATGROUP_Tickables);
}

void UOSEPerceptionSystem::RegisterSource(FOSESenseID SenseID, AActor& SourceActor)
{
	ensure(IsSenseInstantiated(SenseID));
	SourcesToRegister.AddUnique(FPerceptionSourceRegistration(SenseID, &SourceActor));
}

void UOSEPerceptionSystem::PerformSourceRegistration()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

	for (const FPerceptionSourceRegistration& PercSource : SourcesToRegister)
	{
		AActor* SourceActor = PercSource.Source.Get();
		if (SourceActor != nullptr && SourceActor->IsPendingKillPending() == false && Senses[PercSource.SenseID] != nullptr)
		{
			Senses[PercSource.SenseID]->RegisterSource(*SourceActor);

			// hook into notification about actor's EndPlay to remove it as a source 
			SourceActor->OnEndPlay.AddUnique(StimuliSourceEndPlayDelegate);

			// store information we have this actor as given sense's source
			FOSEPerceptionStimuliSource& StimuliSource = RegisteredStimuliSources.FindOrAdd(SourceActor);
			StimuliSource.SourceActor = SourceActor;
			StimuliSource.RelevantSenses.AcceptChannel(PercSource.SenseID);
		}
	}

	SourcesToRegister.Reset();
}

void UOSEPerceptionSystem::OnNewListener(const FOSEPerceptionListener& NewListener)
{
	for (UOSESense* const SenseInstance : Senses)
	{
		// @todo filter out the ones that do not declare using this sense
		if (SenseInstance != nullptr && NewListener.HasSense(SenseInstance->GetSenseID()))
		{
			SenseInstance->OnNewListener(NewListener);
		}
	}
}

void UOSEPerceptionSystem::OnListenerUpdate(const FOSEPerceptionListener& UpdatedListener)
{
	for (UOSESense* const SenseInstance : Senses)
	{
		if (SenseInstance != nullptr)
		{
			SenseInstance->OnListenerUpdate(UpdatedListener);
		}
	}
}

void UOSEPerceptionSystem::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);
	SCOPE_CYCLE_COUNTER(STAT_AI_Overall);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(OSEPerception);

	// if no new stimuli
	// and it's not time to remove stimuli from "know events"

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(GetOuter());
	check(World);

	if (World->bPlayersOnly == false)
	{
		// cache it
		CurrentTime = World->GetTimeSeconds();
		
		if (SourcesToRegister.Num() > 0)
		{
			PerformSourceRegistration();
		}

		bool bSomeListenersNeedUpdateDueToStimuliAging = false;
		if (NextStimuliAgingTick <= CurrentTime)
		{
			bSomeListenersNeedUpdateDueToStimuliAging = AgeStimuli(PerceptionAgingRate + (CurrentTime - NextStimuliAgingTick));
			NextStimuliAgingTick = CurrentTime + PerceptionAgingRate;
		}

		bool bNeedsUpdate = false;
		for (UOSESense* const SenseInstance : Senses)
		{
			bNeedsUpdate |= SenseInstance != nullptr && SenseInstance->ProgressTime(DeltaSeconds);
		}

		if (bNeedsUpdate)
		{
			// first update cached location of all listener, and remove invalid listeners
			for (OSEPerception::FListenerMap::TIterator ListenerIt(ListenerContainer); ListenerIt; ++ListenerIt)
			{
				if (ListenerIt->Value.Listener.IsValid())
				{
					ListenerIt->Value.CacheLocation();
				}
				else
				{
					OnListenerRemoved(ListenerIt->Value);
					ListenerIt.RemoveCurrent();
				}
			}

			for (UOSESense* const SenseInstance : Senses)
			{
				if (SenseInstance != nullptr)
				{
					SenseInstance->Tick();
				}
			}
		}
		{
			SCOPE_CYCLE_COUNTER(STAT_AI_Perception_ProcessStim);
			/** no point in sorting if no new stimuli was processed */
			const bool bStimuliDelivered = DeliverDelayedStimuli(bNeedsUpdate ? RequiresSorting : NoNeedToSort);

			if (bNeedsUpdate || bStimuliDelivered || bSomeListenersNeedUpdateDueToStimuliAging)
			{
				for (OSEPerception::FListenerMap::TIterator ListenerIt(ListenerContainer); ListenerIt; ++ListenerIt)
				{
					check(ListenerIt->Value.Listener.IsValid());

					if (ListenerIt->Value.HasAnyNewStimuli())
					{
						ListenerIt->Value.ProcessStimuli();
					}
				}
			}
		}
	}
}

bool UOSEPerceptionSystem::AgeStimuli(const float Amount)
{
	ensure(Amount >= 0.f);
	bool bTagged = false;

	for (OSEPerception::FListenerMap::TIterator ListenerIt(ListenerContainer); ListenerIt; ++ListenerIt)
	{
		FOSEPerceptionListener& Listener = ListenerIt->Value;
		if (Listener.Listener.IsValid())
		{
			// AgeStimuli will return true if this listener requires an update after stimuli aging
			if (Listener.Listener->AgeStimuli(Amount))
			{
				Listener.MarkForStimulusProcessing();
				bTagged = true;
			}
		}
	}
	return bTagged;
}

UOSEPerceptionSystem* UOSEPerceptionSystem::GetCurrent(UObject* WorldContextObject)
{
	UWorld* World = Cast<UWorld>(WorldContextObject);
	if (World == nullptr && WorldContextObject != nullptr)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	}

   return UWorld::GetSubsystem<UOSEPerceptionSystem>(World);
}

UOSEPerceptionSystem* UOSEPerceptionSystem::GetCurrent(UWorld& World)
{
   return World.GetSubsystem<UOSEPerceptionSystem>();
}

void UOSEPerceptionSystem::UpdateListener(UOSEPerceptionComponent& Listener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

	if (!IsValid(&Listener))
	{
		UnregisterListener(Listener);
		return;
	}

	const FOSEPerceptionListenerID ListenerId = Listener.GetListenerId();

	if (ListenerId != FOSEPerceptionListenerID::InvalidID())
	{
		FOSEPerceptionListener& ListenerEntry = ListenerContainer[ListenerId];
		ListenerEntry.UpdateListenerProperties(Listener);
		OnListenerUpdate(ListenerEntry);
	}
	else
	{			
		const FOSEPerceptionListenerID NewListenerId = FOSEPerceptionListenerID::GetNextID();
		Listener.StoreListenerId(NewListenerId);
		FOSEPerceptionListener& ListenerEntry = ListenerContainer.Add(NewListenerId, FOSEPerceptionListener(Listener));
		ListenerEntry.CacheLocation();
				
		OnNewListener(ListenerContainer[NewListenerId]);
	}
}

void UOSEPerceptionSystem::OnListenerConfigUpdated(FOSESenseID SenseID, const UOSEPerceptionComponent& Listener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

	if (!IsSenseInstantiated(SenseID))
	{
		UE_LOG(LogOSEPerception, Warning, TEXT("Sense must exist to update its sense config"));
		return;
	}

	const FOSEPerceptionListenerID ListenerId = Listener.GetListenerId();
	if (ListenerId == FOSEPerceptionListenerID::InvalidID() || !ListenerContainer.Contains(ListenerId))
	{
		UE_LOG(LogOSEPerception, Warning, TEXT("Listener must have a valid id to update its sense config"));
		return;
	}

	FOSEPerceptionListener& ListenerEntry = ListenerContainer[ListenerId];
	check(ListenerEntry.Listener.IsValid() && ListenerEntry.Listener.Get() == &Listener);

	Senses[SenseID]->OnListenerConfigUpdated(ListenerEntry);
}

void UOSEPerceptionSystem::UnregisterListener(UOSEPerceptionComponent& Listener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

	const FOSEPerceptionListenerID ListenerId = Listener.GetListenerId();

	// can already be removed from ListenerContainer as part of cleaning up 
	// listeners with invalid WeakObjectPtr to UOSEPerceptionComponent
	if (ListenerId != FOSEPerceptionListenerID::InvalidID() && ListenerContainer.Contains(ListenerId))
	{
		check(ListenerContainer[ListenerId].Listener.IsValid() == false
			|| ListenerContainer[ListenerId].Listener.Get() == &Listener);
		OnListenerRemoved(ListenerContainer[ListenerId]);
		ListenerContainer.Remove(ListenerId);

		// mark it as unregistered
		Listener.StoreListenerId(FOSEPerceptionListenerID::InvalidID());
	}
}

void UOSEPerceptionSystem::UnregisterSource(AActor& SourceActor, const TSubclassOf<UOSESense> Sense)
{
	// Log a message if it turns out the source actor was not registered nor pending registration
	bool bSourceWasKnown = false;
	
	FOSEPerceptionStimuliSource* StimuliSource = RegisteredStimuliSources.Find(&SourceActor);
	if (StimuliSource)
	{
		// Source actor was registered
		bSourceWasKnown = true;

		// A single sense can be targeted, or Sense == null for all senses
		if (Sense)
		{
			// Unregister the source actor from a single sense
			const FOSESenseID SenseID = UOSESense::GetSenseID(Sense);
			if (Senses[SenseID] != nullptr && StimuliSource->RelevantSenses.ShouldRespondToChannel(Senses[SenseID]->GetSenseID()))
			{
				Senses[SenseID]->UnregisterSource(SourceActor);
				StimuliSource->RelevantSenses.FilterOutChannel(SenseID);
			}
		}
		else
		{
			// Unregister the source actor from all senses
			for (UOSESense* const SenseInstance : Senses)
			{
				if (SenseInstance != nullptr && StimuliSource->RelevantSenses.ShouldRespondToChannel(SenseInstance->GetSenseID()))
				{
					SenseInstance->UnregisterSource(SourceActor);
				}
			}
			StimuliSource->RelevantSenses.Clear();
		}

		// If the source actor is no longer relevant for any senses, we can remove its stimuli source entry
		if (StimuliSource->RelevantSenses.IsEmpty())
		{
			SourceActor.OnEndPlay.Remove(StimuliSourceEndPlayDelegate);
			RegisteredStimuliSources.Remove(&SourceActor);
		}
	}
	
	// Remove this from any pending adds (add/remove same frame)
	for (int32 RemoveIndex = SourcesToRegister.Num() - 1; RemoveIndex >= 0; RemoveIndex--)
	{
		if (SourcesToRegister[RemoveIndex].Source == &SourceActor)
		{
			// Source actor was pending registration
			bSourceWasKnown = true;

			// A single sense can be targeted, or Sense == null for all senses
			if (!Sense || SourcesToRegister[RemoveIndex].SenseID == UOSESense::GetSenseID(Sense))
			{
				SourcesToRegister.RemoveAt(RemoveIndex, 1, EAllowShrinking::No);
			}
		}
	}

	// Log if SourceActor was not registered or pending registration for any sense
	if (!bSourceWasKnown)
	{
		UE_VLOG(this, LogOSEPerception, Log, TEXT("UnregisterSource called for %s but it doesn't seem to be registered as a source"), *SourceActor.GetName());
	}
}

void UOSEPerceptionSystem::OnListenerRemoved(const FOSEPerceptionListener& NewListener)
{
	for (UOSESense* const SenseInstance : Senses)
	{
		if (SenseInstance != nullptr && NewListener.HasSense(SenseInstance->GetSenseID()))
		{
			SenseInstance->OnListenerRemoved(NewListener);
		}
	}
}

void UOSEPerceptionSystem::OnListenerForgetsActor(const UOSEPerceptionComponent& Listener, AActor& ActorToForget)
{
	const FOSEPerceptionListenerID ListenerId = Listener.GetListenerId();

	if (ListenerId != FOSEPerceptionListenerID::InvalidID())
	{
		FOSEPerceptionListener& ListenerEntry = ListenerContainer[ListenerId];
		
		for (UOSESense* Sense : Senses)
		{
			if (Sense != nullptr && Sense->NeedsNotificationOnForgetting() && ListenerEntry.HasSense(Sense->GetSenseID()))
			{
				Sense->OnListenerForgetsActor(ListenerEntry, ActorToForget);
			}
		}
	}
}

void UOSEPerceptionSystem::OnListenerForgetsAll(const UOSEPerceptionComponent& Listener)
{
	const FOSEPerceptionListenerID ListenerId = Listener.GetListenerId();

	if (ListenerId != FOSEPerceptionListenerID::InvalidID())
	{
		FOSEPerceptionListener& ListenerEntry = ListenerContainer[ListenerId];

		for (UOSESense* Sense : Senses)
		{
			if (Sense != nullptr && Sense->NeedsNotificationOnForgetting() && ListenerEntry.HasSense(Sense->GetSenseID()))
			{
				Sense->OnListenerForgetsAll(ListenerEntry);
			}
		}
	}
}

void UOSEPerceptionSystem::RegisterDelayedStimulus(FOSEPerceptionListenerID ListenerId, float Delay, AActor* Instigator, const FOSEStimulus& Stimulus)
{
	FDelayedStimulus DelayedStimulus;
	DelayedStimulus.DeliveryTimestamp = CurrentTime + Delay;
	DelayedStimulus.ListenerId = ListenerId;
	DelayedStimulus.Instigator = Instigator;
	DelayedStimulus.Stimulus = Stimulus;
	DelayedStimuli.Add(DelayedStimulus);
}

bool UOSEPerceptionSystem::DeliverDelayedStimuli(UOSEPerceptionSystem::EDelayedStimulusSorting Sorting)
{
	struct FTimestampSort
	{ 
		bool operator()(const FDelayedStimulus& A, const FDelayedStimulus& B) const
		{
			return A.DeliveryTimestamp < B.DeliveryTimestamp;
		}
	};

	if (DelayedStimuli.Num() <= 0)
	{
		return false;
	}

	if (Sorting == RequiresSorting)
	{
		DelayedStimuli.Sort(FTimestampSort());
	}

	int Index = 0;
	while (Index < DelayedStimuli.Num() && DelayedStimuli[Index].DeliveryTimestamp < CurrentTime)
	{
		FDelayedStimulus& DelayedStimulus = DelayedStimuli[Index];
		
		if (DelayedStimulus.ListenerId != FOSEPerceptionListenerID::InvalidID() && ListenerContainer.Contains(DelayedStimulus.ListenerId))
		{
			FOSEPerceptionListener& ListenerEntry = ListenerContainer[DelayedStimulus.ListenerId];
			// this has been already checked during tick, so if it's no longer the case then it's a bug
			check(ListenerEntry.Listener.IsValid());

			// deliver
			ListenerEntry.RegisterStimulus(DelayedStimulus.Instigator.Get(), DelayedStimulus.Stimulus);
		}

		++Index;
	}

	DelayedStimuli.RemoveAt(0, Index, EAllowShrinking::No);

	return Index > 0;
}

void UOSEPerceptionSystem::MakeNoiseImpl(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation, float MaxRange, FName Tag)
{
	UE_CLOG(NoiseMaker == nullptr && NoiseInstigator == nullptr, LogOSEPerception, Warning
		, TEXT("UOSEPerceptionSystem::MakeNoiseImpl called with both NoiseMaker and NoiseInstigator being null. Unable to resolve UWorld context!"));
	
	UWorld* World = NoiseMaker ? NoiseMaker->GetWorld() : (NoiseInstigator ? NoiseInstigator->GetWorld() : nullptr);

	if (World)
	{
		UOSEPerceptionSystem::OnEvent(World, FOSENoiseEvent(NoiseInstigator ? NoiseInstigator : NoiseMaker
			, NoiseLocation
			, Loudness
			, MaxRange
			, Tag));
	}
}

void UOSEPerceptionSystem::OnActorSpawned(AActor* SpawnedActor)
{
   APawn* AsPawn = Cast<APawn>(SpawnedActor);
   if (AsPawn)
   {
      OnNewPawn(*AsPawn);
   }
}

void UOSEPerceptionSystem::OnNewPawn(APawn& Pawn)
{
	if (bHandlePawnNotification == false)
	{
		return;
	}

	for (UOSESense* Sense : Senses)
	{
		if (Sense == nullptr)
		{
			continue;
		}

		if (Sense->WantsNewPawnNotification())
		{
			Sense->OnNewPawn(Pawn);
		}

		if (Sense->ShouldAutoRegisterAllPawnsAsSources())
		{
			FOSESenseID SenseID = Sense->GetSenseID();
			check(IsSenseInstantiated(SenseID));
			RegisterSource(SenseID, Pawn);
		}
	}
}

void UOSEPerceptionSystem::RegisterSource(AActor& SourceActor)
{
	for (UOSESense* Sense : Senses)
	{
		if (Sense == nullptr)
		{
			continue;
		}

		const FOSESenseID SenseID = Sense->GetSenseID();
		if (IsSenseInstantiated(SenseID))
		{
			RegisterSource(SenseID, SourceActor);
		}
	}
}

void UOSEPerceptionSystem::OnWorldBeginPlay(UWorld& InWorld)
{
   for (UOSESense* Sense : Senses)
   {
      if (Sense != nullptr && Sense->ShouldAutoRegisterAllPawnsAsSources())
      {
         FOSESenseID SenseID = Sense->GetSenseID();
         RegisterAllPawnsAsSourcesForSense(SenseID);
      }
   }

   NextStimuliAgingTick = InWorld.GetTimeSeconds();
   if(InWorld.GetAuthGameMode() != nullptr)
   {
      _IsAllowedToTick = true;
      FOnActorSpawned::FDelegate ActorSpawnedDelegate = FOnActorSpawned::FDelegate::CreateUObject(this, &UOSEPerceptionSystem::OnActorSpawned);
      InWorld.AddOnActorSpawnedHandler(ActorSpawnedDelegate);
   }
}

bool UOSEPerceptionSystem::IsTickable() const
{
   return _IsAllowedToTick;
}

void UOSEPerceptionSystem::RegisterAllPawnsAsSourcesForSense(FOSESenseID SenseID)
{
	UWorld* World = GetWorld();
	for (TActorIterator<APawn> PawnIt(World); PawnIt; ++PawnIt)
	{
		RegisterSource(SenseID, **PawnIt);
	}
}

bool UOSEPerceptionSystem::RegisterPerceptionStimuliSource(UObject* WorldContextObject, TSubclassOf<UOSESense> Sense, AActor* Target)
{
	bool bResult = false;
	if (Sense && Target)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		if (World && World->GetAISystem())
		{
			// just a cache
			UOSEPerceptionSystem* PerceptionSys = UOSEPerceptionSystem::GetCurrent(World);
				
			PerceptionSys->RegisterSourceForSenseClass(Sense, *Target);

			bResult = true;
		}
	}

	return bResult;
}

void UOSEPerceptionSystem::RegisterSourceForSenseClass(TSubclassOf<UOSESense> Sense, AActor& Target)	
{
	FOSESenseID SenseID = UOSESense::GetSenseID(Sense);
	if (IsSenseInstantiated(SenseID) == false)
	{
		SenseID = RegisterSenseClass(Sense);
	}

	RegisterSource(SenseID, Target);
}

void UOSEPerceptionSystem::OnPerceptionStimuliSourceEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	UnregisterSource(*Actor);
}

TSubclassOf<UOSESense> UOSEPerceptionSystem::GetSenseClassForStimulus(UObject* WorldContextObject, const FOSEStimulus& Stimulus)
{
	TSubclassOf<UOSESense> Result = nullptr;
	UOSEPerceptionSystem* PercSys = GetCurrent(WorldContextObject);
	if (PercSys && PercSys->Senses.IsValidIndex(Stimulus.Type) && PercSys->Senses[Stimulus.Type] != nullptr)
	{
		Result = PercSys->Senses[Stimulus.Type]->GetClass();
	}

	return Result;
}

UOSESense* UOSEPerceptionSystem::GetSense(FOSESenseID SenseID)
{
   return Senses[SenseID];
}

//----------------------------------------------------------------------//
// Blueprint API
//----------------------------------------------------------------------//
void UOSEPerceptionSystem::ReportEvent(UOSESenseEvent* PerceptionEvent)
{
	if (PerceptionEvent)
	{
		const FOSESenseID SenseID = PerceptionEvent->GetSenseID();
		if (SenseID.IsValid() && Senses.IsValidIndex(SenseID) && Senses[SenseID] != nullptr)
		{
			Senses[SenseID]->RegisterWrappedEvent(*PerceptionEvent);
		}
		else
		{
			UE_VLOG(this, LogOSEPerception, Log, TEXT("Skipping perception event %s since related sense class has not been registered (no listeners)")
				, *PerceptionEvent->GetName());
			PerceptionEvent->DrawToVLog(*this);
		}
	}
}

void UOSEPerceptionSystem::ReportPerceptionEvent(UObject* WorldContextObject, UOSESenseEvent* PerceptionEvent)
{
	UOSEPerceptionSystem* PerceptionSys = GetCurrent(WorldContextObject);
	if (PerceptionSys != nullptr)
	{
		PerceptionSys->ReportEvent(PerceptionEvent);
	}
}

