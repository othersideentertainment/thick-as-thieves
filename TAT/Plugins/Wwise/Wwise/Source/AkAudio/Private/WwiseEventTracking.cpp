/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#include "WwiseEventTracking.h"
#include "AkAudioEvent.h"

void FWwiseEventTracker::PostEventCallbackHandler(AkCallbackType in_eType, AkCallbackInfo * in_pCallbackInfo)
{
	if (in_pCallbackInfo == nullptr)
		return;

	auto Tracker = (FWwiseEventTracker*)in_pCallbackInfo->pCookie;
	if (Tracker == nullptr)
		return;

	/* Event end */
	if (in_eType == AkCallbackType::AK_EndOfEvent)
	{
		const auto CBInfo = (AkEventCallbackInfo*)in_pCallbackInfo;
		const auto IDToStop = CBInfo->playingID;
		Tracker->RemovePlayingID(IDToStop);
		Tracker->RemoveScheduledStop(IDToStop);
	}/* Received close to the beginning of the event */
	else if (in_eType == AkCallbackType::AK_Duration)
	{
		const auto CBInfo = (AkDurationCallbackInfo*)in_pCallbackInfo;
		Tracker->CurrentDurationEstimation = (CBInfo->fEstimatedDuration * Tracker->CurrentDurationProportionRemaining) / 1000.0f;
	}
}

void FWwiseEventTracker::RemoveScheduledStop(AkPlayingID InID)
{
	FScopeLock autoLock(&ScheduledStopsLock);

	for (auto PlayingID : ScheduledStops)
	{
		if (PlayingID == InID)

			{
			ScheduledStops.Remove(PlayingID);
			break;
		}
	}
}

void FWwiseEventTracker::RemovePlayingID(AkPlayingID InID)
{
	FScopeLock autoLock(&PlayingIDsLock);
	PlayingIDs.Remove(InID);
}

void FWwiseEventTracker::TryAddPlayingID(const AkPlayingID & PlayingID)
{
	if (PlayingID != AK_INVALID_PLAYING_ID)
	{
		FScopeLock autoLock(&PlayingIDsLock);
		PlayingIDs.Add(PlayingID);
	}
}

void FWwiseEventTracker::EmptyPlayingIDs()
{
	FScopeLock autoLock(&PlayingIDsLock);
	PlayingIDs.Empty();
}

void FWwiseEventTracker::EmptyScheduledStops()
{
	FScopeLock autoLock(&ScheduledStopsLock);
	ScheduledStops.Empty();
}

bool FWwiseEventTracker::PlayingIDHasScheduledStop(AkPlayingID InID)
{
	FScopeLock autoLock(&ScheduledStopsLock);

	for (auto PlayingID : ScheduledStops)
	{
		if (PlayingID == InID)
		{
			return true;
		}
	}

	return false;
}

void FWwiseEventTracker::AddScheduledStop(AkPlayingID InID)
{
	FScopeLock autoLock(&ScheduledStopsLock);
	ScheduledStops.Add(InID);
}

namespace WwiseEventTriggering
{
	TArray<AkPlayingID> GetPlayingIds(FWwiseEventTracker& EventTracker)
	{
		FScopeLock autoLock(&EventTracker.PlayingIDsLock);
		return EventTracker.PlayingIDs;
	}

	void LogDirtyPlaybackWarning()
	{
		UE_LOG(LogAkAudio, Warning, TEXT("Playback occurred from sequencer section with new changes. You may need to save your diry work units and re-generate your soundbanks."));
	}

	void PausePlayingID(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker, AkPlayingID InPlayingID)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT("WwiseEventTriggering::PausePlayingID: Event Tracker: %p. Pausing Playing ID: %d."), &EventTracker,
		       InPlayingID)

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (AudioDevice)
		{
			for (auto PlayingID : GetPlayingIds(EventTracker))
			{
				AudioDevice->PausePlayingID(PlayingID, 50, AkCurveInterpolation_Linear);
			}
		}
	}

	void PauseAllPlayingIDs(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker)
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("WwiseEventTriggering::PauseAllPlayingIDs: Event Tracker: %p. Pausing play on all Playing Events"), &EventTracker);
		for (auto PlayingID : GetPlayingIds(EventTracker))
		{
			PausePlayingID(AudioDevice, EventTracker, PlayingID);
		}
	}

	void ResumePlayingID(TObjectPtr<UObject> Object, FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker, AkReal32 ProportionalTime, AkPlayingID InPlayingID)
	{
		ensure(AudioDevice != nullptr);
		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (AudioDevice)
		{
			UE_LOG(LogAkAudio, VeryVerbose, TEXT("WwiseEventTriggering::ResumePlayingID: Event Tracker: %p. Resuming. Playing ID: %d"),
				&EventTracker, InPlayingID)
			AudioDevice->ResumePlayingID(InPlayingID, 50, AkCurveInterpolation_Linear);
			if (ProportionalTime >= 0)
			{
				UE_LOG(LogAkAudio, VeryVerbose,
				       TEXT(
					       "WwiseEventTriggering::ResumePlayingID: Event Tracker: %p. Resuming Play at proportional time %f. Playing ID: %d"
				       ),
				       &EventTracker, ProportionalTime, InPlayingID)
				if (Object)
				{
					SeekOnEvent(Object, AudioDevice, ProportionalTime, EventTracker, InPlayingID);
				}
				else
				{
					SeekOnEventWithDummyObject(AudioDevice, ProportionalTime, EventTracker, InPlayingID);
				}
			}
		}
	}

	void ResumeAllPlayingIDs(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker, AkReal32 ProportionalTime)
	{
		UE_LOG(LogAkAudio, Verbose,
		       TEXT(
			       "WwiseEventTriggering::ResumePlayingID: EventTracker: %p, Resuming Play on all Playing IDs at proportional time %f"
		       ), &EventTracker, ProportionalTime);
		for (auto PlayingID : GetPlayingIds(EventTracker))
		{
			ResumePlayingID(nullptr, AudioDevice, EventTracker, ProportionalTime, PlayingID);
		}
	}

	void StopAllPlayingIDs(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker)
	{
		UE_LOG(LogAkAudio, Verbose,
		       TEXT("WwiseEventTriggering::StopAllPlayingIDs: EventTracker: %p, Stopping play all Playing Events"),
		       &EventTracker);
		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (AudioDevice)
		{
			for (auto PlayingID : GetPlayingIds(EventTracker))
			{
				AudioDevice->StopPlayingID(PlayingID);
			}
		}
	}

	AkPlayingID PostEventOnDummyObject(FAkAudioDevice* AudioDevice, FWwiseEventTracker& EventTracker)
	{
		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (EventTracker.EventName.IsEmpty())
		{
			UE_LOG(LogAkAudio, Warning,
			       TEXT(
				       "WwiseEventTriggering::PostEventOnDummyObect: EventTracker: %p. Attempted to post an AkEvent from an empty Sequencer section."
			       ), &EventTracker);
			return AK_INVALID_PLAYING_ID;
		}

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (AudioDevice)
		{
			AkPlayingID PlayingID = AK_INVALID_PLAYING_ID;
			if (EventTracker.Event)
			{
				PlayingID = EventTracker.Event->PostAmbient(nullptr, &FWwiseEventTracker::PostEventCallbackHandler, &EventTracker,
					(AkCallbackType)(AK_EndOfEvent | AK_Duration), nullptr);
			}
			if (LIKELY(PlayingID != AK_INVALID_PLAYING_ID))
			{
				UE_LOG(LogAkAudio, VeryVerbose,
				       TEXT(
					       "WwiseEventTriggering::PostEventOnDummyObject: EventTracker: %p. Posted dummy event. Playing ID %d"
				       ), &EventTracker, PlayingID);
				EventTracker.TryAddPlayingID(PlayingID);
				if (EventTracker.IsDirty)
					LogDirtyPlaybackWarning();
				return PlayingID;
			}
			UE_LOG(LogAkAudio, Warning,
			       TEXT("WwiseEventTriggering::PostEventOnDummyObject: EventTracker: %p. Failed to post dummy event"),
			        &EventTracker);
		}
		return AK_INVALID_PLAYING_ID;
	}

	AkPlayingID PostEvent(TObjectPtr<UObject> Object, FAkAudioDevice* AudioDevice, FWwiseEventTracker& EventTracker)
	{
		if (EventTracker.EventName.IsEmpty())
		{
			UE_LOG(LogAkAudio, Warning,
			       TEXT(
				       "WwiseEventTriggering::PostEvent: EventTracker: %p. Attempted to post an AkEvent from an empty Sequencer section."
			       ), &EventTracker);
			return AK_INVALID_PLAYING_ID;
		}
		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		checkf(Object, TEXT("No Object available! PostEventOnDummyObject can be used instead"))

		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT("WwiseEventTriggering::PostEvent: EventTracker: %p. Posting Event Play on %s"), &EventTracker,
		       IsValid(Object)? *Object.GetName() : TEXT("Unbound"))

		if (Object && AudioDevice)
		{
			auto AkComponent = Cast<UAkComponent>(Object);

			if (!IsValid(AkComponent))
			{
				auto Actor = CastChecked<AActor>(Object);
				if (IsValid(Actor))
				{
					AkComponent = AudioDevice->GetAkComponent(Actor->GetRootComponent(), FName(), NULL, EAttachLocation::KeepRelativeOffset);
				}
			}

			if (IsValid(AkComponent))
			{
				AkPlayingID PlayingID = AK_INVALID_PLAYING_ID;
				if (EventTracker.Event)
				{
					PlayingID = EventTracker.Event->PostOnComponent(AkComponent, nullptr, &FWwiseEventTracker::PostEventCallbackHandler, &EventTracker, (AkCallbackType)(AK_EndOfEvent | AK_Duration), nullptr, AkComponent->StopWhenOwnerDestroyed);
				}
				EventTracker.TryAddPlayingID(PlayingID);
				if (EventTracker.IsDirty)
					LogDirtyPlaybackWarning();

				UE_LOG(LogAkAudio, VeryVerbose,
				       TEXT("WwiseEventTriggering::PostEvent: EventTracker: %p. Posted Event Play on %s. Playing ID: %d"), &EventTracker,
				       IsValid(Object)? *Object.GetName() : TEXT("Unbound"), PlayingID)
				return PlayingID;
			}
		}
		UE_LOG(LogAkAudio, VeryVerbose,
			   TEXT("WwiseEventTriggering::PostEvent: EventTracker: %p. Failed to psot Event Play on %s"), &EventTracker,
			   IsValid(Object)? *Object.GetName() : TEXT("Unbound"))
		return AK_INVALID_PLAYING_ID;
	}

	void StopEvent(FAkAudioDevice * AudioDevice, AkPlayingID InPlayingID, FWwiseEventTracker * EventTracker)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT("WwiseEventTriggering::StopEvent: EventTracker: %p. Stopping Event on PlayingID %d"), &EventTracker, InPlayingID);

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (AudioDevice)
			AudioDevice->StopPlayingID(InPlayingID);
	}

	void TriggerStopEvent(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker, AkPlayingID InPlayingID)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT("WwiseEventTriggering::TriggerStopEvent: EventTracker: %p. Stopping Event on PlayingID %d"),
		       &EventTracker, InPlayingID);
		AudioDevice->StopPlayingID(InPlayingID, (float)EventTracker.ScrubTailLengthMs, AkCurveInterpolation::AkCurveInterpolation_Log1);
		EventTracker.AddScheduledStop(InPlayingID);
	}

	void ScheduleStopEventsForCurrentlyPlayingIDs(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker)
	{
		UE_LOG(LogAkAudio, Verbose,
		       TEXT(
			       "WwiseEventTriggering::ScheduleStopEventsForCurrentlyPlayingIDs:EventTracker: %p. Stopping currently Playing ID"
		       ), &EventTracker);

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (AudioDevice)
		{
			for (auto PlayingID : GetPlayingIds(EventTracker))
			{
				if (!EventTracker.PlayingIDHasScheduledStop(PlayingID))
				{
					TriggerStopEvent(AudioDevice, EventTracker, PlayingID);
				}
			}
		}
	}

	void TriggerScrubSnippetOnDummyObject(FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker)
	{
		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (EventTracker.EventName.IsEmpty())
		{
			return;
		}

		if (AudioDevice)
		{
			AkPlayingID PlayingID = AK_INVALID_PLAYING_ID;
			if (EventTracker.Event)
			{
				PlayingID = EventTracker.Event->PostAmbient(nullptr, &FWwiseEventTracker::PostEventCallbackHandler, &EventTracker,
					(AkCallbackType)(AK_EndOfEvent | AK_Duration), nullptr);
			}
			if (LIKELY(PlayingID != AK_INVALID_PLAYING_ID))
			{
				EventTracker.TryAddPlayingID(PlayingID);
				if (EventTracker.IsDirty)
					LogDirtyPlaybackWarning();
				TriggerStopEvent(AudioDevice, EventTracker, PlayingID);
			}
		}
	}

	void TriggerScrubSnippet(TObjectPtr<UObject> Object, FAkAudioDevice * AudioDevice, FWwiseEventTracker & EventTracker)
	{

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (EventTracker.EventName.IsEmpty())
		{
			return;
		}

		if (Object && AudioDevice)
		{
			auto AkComponent = Cast<UAkComponent>(Object);

			if (!IsValid(AkComponent))
			{
				auto Actor = CastChecked<AActor>(Object);
				if (IsValid(Actor))
				{
					AkComponent = AudioDevice->GetAkComponent(Actor->GetRootComponent(), FName(), NULL, EAttachLocation::KeepRelativeOffset);
				}
			}

			if (IsValid(AkComponent))
			{
				AkPlayingID PlayingID = AK_INVALID_PLAYING_ID;
				if (EventTracker.Event)
				{
					PlayingID = EventTracker.Event->PostOnComponent(AkComponent, nullptr, &FWwiseEventTracker::PostEventCallbackHandler, &EventTracker,
						(AkCallbackType)(AK_EndOfEvent | AK_Duration), nullptr, AkComponent->StopWhenOwnerDestroyed);
				}
				if (LIKELY(PlayingID != AK_INVALID_PLAYING_ID))
				{
					EventTracker.TryAddPlayingID(PlayingID);
					if (EventTracker.IsDirty)
						LogDirtyPlaybackWarning();
					TriggerStopEvent(AudioDevice, EventTracker, PlayingID);
				}
			}
		}
	}

	void SeekOnEvent(TObjectPtr<UObject> Object, FAkAudioDevice * AudioDevice, AkReal32 in_fPercent, FWwiseEventTracker & EventTracker, AkPlayingID InPlayingID)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT(
			       "WwiseEventTriggering::SeekOnEvent: EventTracker: %p. Seeking to proportional time %f on %s Playing ID: %d"),
		       &EventTracker, in_fPercent, IsValid(Object)? *Object.GetName() : TEXT("Unbound"), InPlayingID)

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if (EventTracker.EventName.IsEmpty())
		{
			return;
		}

		if (Object && AudioDevice)
		{
			auto AkComponent = Cast<UAkComponent>(Object);
			if (!IsValid(AkComponent))
			{
				auto Actor = CastChecked<AActor>(Object);
				if (IsValid(Actor))
				{
					AkComponent = AudioDevice->GetAkComponent(Actor->GetRootComponent(), FName(), NULL,
															  EAttachLocation::KeepRelativeOffset);
				}
			}
			if (IsValid(AkComponent))
			{
				const AkUInt32 ShortID = AudioDevice->GetShortID(EventTracker.Event, EventTracker.EventName);
				AudioDevice->SeekOnEvent(ShortID, AkComponent, in_fPercent, false, InPlayingID);
			}
		}
	}

	void SeekOnEvent(TObjectPtr<UObject> Object, FAkAudioDevice * AudioDevice, AkReal32 ProportionalTime, FWwiseEventTracker & EventTracker)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT(
			       "WwiseEventTriggering::SeekOnEvent: EventTracker: %p. Seeking to proportional time %f on %s on all Playing IDs"
		       ), &EventTracker, ProportionalTime, IsValid(Object)? *Object.GetName() : TEXT("Unbound"))

		for (auto PlayingID : GetPlayingIds(EventTracker))
		{
			SeekOnEvent(Object, AudioDevice, ProportionalTime, EventTracker, PlayingID);
		}
	}

	void SeekOnEventWithDummyObject(FAkAudioDevice * AudioDevice, AkReal32 ProportionalTime, FWwiseEventTracker & EventTracker, AkPlayingID InPlayingID)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT(
			       "WwiseEventTriggering::SeekOnEvent: EventTracker: %p. Seeking to proportional time %f on Playing ID %d"
		       ), &EventTracker, ProportionalTime, InPlayingID)

		checkf(AudioDevice, TEXT("No AudioDevice available!"))
		if ( EventTracker.EventName.IsEmpty())
		{
			return;
		}

		if (AudioDevice)
		{
			if (ProportionalTime < 1.0f && ProportionalTime >= 0.0f)
			{
				AActor* DummyActor = nullptr;
				const AkUInt32 ShortID = AudioDevice->GetShortID(EventTracker.Event, EventTracker.EventName);
				AudioDevice->SeekOnEvent(ShortID, DummyActor, ProportionalTime, false, InPlayingID);
				// Update the duration proportion remaining property of the event tracker, rather than updating the current duration directly here.
				// This way, we ensure that the current duration is updated first by any PostEvent callback,
				// before it is then multiplied by the remaining proportion.
				EventTracker.CurrentDurationProportionRemaining = 1.0f - ProportionalTime;
			}
		}
	}

	void SeekOnEventWithDummyObject(FAkAudioDevice * AudioDevice, AkReal32 ProportionalTime, FWwiseEventTracker & EventTracker)
	{
		UE_LOG(LogAkAudio, VeryVerbose,
		       TEXT("WwiseEventTriggering::SeekOnEvent: EventTracker: %p. Seeking on all Playing IDs"), &EventTracker)

		for (auto PlayingID : GetPlayingIds(EventTracker))
		{
			SeekOnEventWithDummyObject(AudioDevice, ProportionalTime, EventTracker, PlayingID);
		}
	}
}
