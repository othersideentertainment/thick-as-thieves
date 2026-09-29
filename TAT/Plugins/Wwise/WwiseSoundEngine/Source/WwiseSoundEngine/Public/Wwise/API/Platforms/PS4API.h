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

#pragma once

#include "CoreMinimal.h"
#include "AkInclude.h"

#include "Wwise/WwiseSoundEngineModule.h"

#if defined(PLATFORM_PS4) && PLATFORM_PS4

class IWwisePlatformAPI
{
public:
	inline static IWwisePlatformAPI* Get()
	{
		IWwiseSoundEngineModule::ForceLoadModule();
		return IWwiseSoundEngineModule::Platform;
	}

	UE_NONCOPYABLE(IWwisePlatformAPI);

protected:
	IWwisePlatformAPI() = default;

public:
	virtual ~IWwisePlatformAPI() {}

	/// Returns the PS4 output port handle being used by the Wwise SoundEngine for the output matching
	/// the provided OutputDeviceId. This should be called only once the SoundEngine has been successfully
	/// initialized, otherwise the function will return an invalid value (-1). Invalid values will also
	/// be returned if the OutputDeviceId matches a device that is not an output for the Default, DVR,
	/// Communication, Personal, Pad_output, or Aux devices. 
	/// 
	/// Use <tt>AK::SoundEngine::RegisterAudioDeviceStatusCallback()</tt> to get notified when devices are created/destructed.
	///
	/// Note that because this function acquires the global lock in Wwise in order to fetch device state,
	/// it is recommended to call this inside of the AudioDeviceStatusCallback, after a successful AkAudioDeviceEvent_Initialization.
	/// 
	/// \return the current PS4 main output port handle or -1.
	virtual int GetPS4OutputHandle(AkOutputDeviceID deviceId = 0) = 0;

	/// Returns the sceAudio3d port ID being used by the Wwise SoundEngine for the output.
	/// Since there can only be a single sceAudio3d port open at a time, the sceAudio3d port
	/// used by the sound engine should also be used for all other sceAudio3d operations.
	/// The sceAudio3d module will be loaded and initialized by the sound engine at initialization.
	/// sceAudio3dPortAdvance and sceAudio3dPortPush is called after each audio frame.
	///
	/// This should be called only once the SoundEngine has been successfully
	/// initialized, otherwise the function will return an invalid value (-1).
	///
	/// \return the current sceAudio3d port ID or -1.
	virtual int GetSceAudio3dPortId() = 0;
};

#endif
