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

#include "AkAudioMixer.h"

#include "AkMixerPlatform.h"
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "AkAudioModule.h"

void FAkAudioMixerModule::StartupModule()
{
	UE_LOG(LogAkAudio, Warning,
	       TEXT(
		       "FAkAudioMixerModule::StartupModule Use of AKAudioMixer is deprecated. Consider using AudioLink instead (https://www.audiokinetic.com/en/library/edge/?source=UE4&id=using_audio_link.html)"
	       ));
}


void FAkAudioMixerModule::ShutdownModule()
{
}

// Implementation of IAudioDeviceModule
bool FAkAudioMixerModule::IsAudioMixerModule() const
{
	return true;
}

Audio::IAudioMixerPlatformInterface* FAkAudioMixerModule::CreateAudioMixerPlatformInterface()
{
	return new FAkMixerPlatform();
}


IMPLEMENT_MODULE( FAkAudioMixerModule, AkAudioMixer )
