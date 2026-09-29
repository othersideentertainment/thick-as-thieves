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
// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND LicenseRef-Audiokinetic-Wwise AND MIT


#include "Audio/TATWwiseUtils.h"

// wwise
#include "AkAudioDevice.h"
#include "AkAudioEvent.h"
#include "AkRoomComponent.h"
#include "AkSwitchValue.h"
#include "Wwise/API/WwiseSoundEngineAPI.h"
#include "Wwise/Stats/AkAudio.h"

// std
#include <inttypes.h> // for PRIu64

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWwiseUtils)

AkPlayingID UTATWwiseUtils::_PostEventAtLocationCustom(UAkAudioEvent* event, const FVector& location, const FRotator& orientation, const UObject* worldContext, TFunctionRef<void(IWwiseSoundEngineAPI&, AkGameObjectID)> setup)
{
   // NOTE: Largely adapted from UAkAudioEvent::PostAtLocation
   SCOPED_AKAUDIO_EVENT(TEXT("UTATWwiseUtils::PostEventAtLocationCustom"));

   const UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      return AK_INVALID_PLAYING_ID;
   }

   if (event == nullptr)
   {
      return AK_INVALID_PLAYING_ID;
   }

	FAkAudioDevice* audioDevice = FAkAudioDevice::Get();
	if (UNLIKELY(!audioDevice))
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("Failed to post AkAudioEvent '%s' at a location without an Audio Device."), *GetNameSafe(event));
		return AK_INVALID_PLAYING_ID;
	}

	if (UNLIKELY(!audioDevice->IsInitialized()))
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("Failed to post AkAudioEvent '%s' at a location with the Sound Engine uninitialized."), *GetNameSafe(event));
		return AK_INVALID_PLAYING_ID;
	}

	IWwiseSoundEngineAPI* soundEngine = IWwiseSoundEngineAPI::Get();
	if (UNLIKELY(!soundEngine))
	{
		UE_LOG(LogAkAudio, Warning, TEXT("Failed to post AkAudioEvent '%s' at a location without a Sound Engine."), *GetNameSafe(event));
		return AK_INVALID_PLAYING_ID;
	}

	if (UNLIKELY(!world))
	{
		UE_LOG(LogAkAudio, Log, TEXT("Failed to post AkAudioEvent '%s' at a location without a world world."), *GetNameSafe(event));
		return AK_INVALID_PLAYING_ID;
	}

	if (UNLIKELY(!world->AllowAudioPlayback()))
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("Failed to post AkAudioEvent '%s' with a world '%s' that doesn't allow audio playback."), *GetNameSafe(event), *world->GetName());
		return AK_INVALID_PLAYING_ID;
	}

   // NOTE: c-style cast is verbatim what the wwise original does
	const AkGameObjectID objectID = (AkGameObjectID)event;
   // TODO: bypass to avoid string alloc
	AKRESULT result = audioDevice->RegisterGameObject(objectID, event->GetName());
	if (UNLIKELY(result != AK_Success))
	{
		return AK_INVALID_PLAYING_ID;
	}

	TArray<AkAuxSendValue> akReverbVolumes;
	audioDevice->GetAuxSendValuesAtLocation(location, akReverbVolumes, world);
	result = soundEngine->SetGameObjectAuxSendValues(objectID, akReverbVolumes.GetData(), akReverbVolumes.Num());
	UE_CLOG(UNLIKELY(result != AK_Success), LogAkAudio, Log, TEXT("Could not Set AuxSend Values while PostOnLocation for AkAudioEvent '%s' (ObjId: %" PRIu64 "): (%" PRIu32 ") %s."), *event->GetName(), objectID, result, WwiseUnrealHelper::GetResultString(result));
   
	auto& RoomIndex = audioDevice->GetRoomIndex();
	TArray<UAkRoomComponent*> akRooms = RoomIndex.Query<UAkRoomComponent>(location, world);
	if (LIKELY(akRooms.Num() > 0))
	{
		UE_CLOG(akRooms.Num() > 1, LogAkAudio, Verbose, TEXT("There are %d rooms while PostOnLocation for AkAudioEvent '%s' (ObjId: %" PRIu64 "). Picking the first one."), (int)akRooms.Num(), *event->GetName(), objectID);
		const AkRoomID roomID = akRooms[0]->GetRoomID();
		audioDevice->SetInSpatialAudioRoom(objectID, roomID);
	}
	else
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("No Spatial Audio Room while PostOnLocation AkAudioEvent '%s' (ObjId: %" PRIu64 ")"), *event->GetName(), objectID);
	}

	AkSoundPosition soundPosition;
	FQuat orientationQuat(orientation);
	audioDevice->FVectorsToAKWorldTransform(location, orientationQuat.GetForwardVector(), orientationQuat.GetUpVector(), soundPosition);
	result = soundEngine->SetPosition(objectID, soundPosition);
	UE_CLOG(UNLIKELY(result != AK_Success), LogAkAudio, Log, TEXT("Could not Set Position for AkAudioEvent '%s' (ObjId: %" PRIu64 "): (%" PRIu32 ") %s."), *event->GetName(), objectID, result, WwiseUnrealHelper::GetResultString(result));

   setup(*soundEngine, objectID);

   // eliding callback params until there is a usecase to use them
   const auto playingID = event->PostOnGameObjectID(objectID, nullptr, nullptr, nullptr, {}, nullptr);

	result = soundEngine->UnregisterGameObj(objectID);
	UE_CLOG(UNLIKELY(result != AK_Success), LogAkAudio, Log, TEXT("Could not Unregister GameObject after PostOnLocation for AkAudioEvent '%s' (ObjId: %" PRIu64 "): (%" PRIu32 ") %s."), *event->GetName(), objectID, result, WwiseUnrealHelper::GetResultString(result));

	return playingID;
}



int32 UTATWwiseUtils::PostEventAtLocationWithSwitch(UAkAudioEvent* akEvent, FVector location, FRotator orientation, const UAkSwitchValue* switchValue, const UObject* worldContextObject)
{
   auto setup = [switchValue](IWwiseSoundEngineAPI& soundEngine, AkGameObjectID objectId) {
      if (switchValue)
      {
         soundEngine.SetSwitch(switchValue->GroupValueCookedData.GroupId, switchValue->GroupValueCookedData.Id, objectId);
      }
   };

   return _PostEventAtLocationCustom(akEvent, location, orientation, worldContextObject, setup);
}
