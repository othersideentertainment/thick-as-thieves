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

/*=============================================================================
AkSubmixInputComponent.cpp:
=============================================================================*/

#include "AkSubmixInputComponent.h"
#include "AkAudioDevice.h"
#include "AudioMixerDevice.h"

#include <inttypes.h>

UDEPRECATED_UAkSubmixInputComponent::UDEPRECATED_UAkSubmixInputComponent(const class FObjectInitializer& ObjectInitializer) :
	UAkAudioInputComponent(ObjectInitializer), SubmixListener(MakeShared<FAkSubmixBufferListener>())
{}

void FAkSubmixBufferListener::OnNewSubmixBuffer(
	const USoundSubmix* InOwningSubmix,
	float* InAudioData,
	int32				InNumSamples,
	int32				InNumChannels,
	const int32			InSampleRate,
	double				InAudioClock)
{
	check(InNumChannels == NumChannels);
	check(InSampleRate == SampleRate);
	SampleBuffer.Push(InAudioData, InNumSamples);
}

Audio::FMixerDevice* UDEPRECATED_UAkSubmixInputComponent::GetAudioMixerDevice()
{
	UWorld* ThisWorld = GetWorld();
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	if (FAudioDevice* AudioDevice = ThisWorld->GetAudioDevice().GetAudioDevice())
	{
		if (!AudioDevice->bAudioMixerModuleLoaded)
		{
			return nullptr;
		}
		else
		{
			return static_cast<Audio::FMixerDevice*>(AudioDevice);
		}
	}
	return nullptr;
}

int32 UDEPRECATED_UAkSubmixInputComponent::PostAssociatedAudioInputEvent()
{
	if (PlayingID == AK_INVALID_PLAYING_ID)
	{
		Audio::FMixerDevice* AudioMixerDevice = GetAudioMixerDevice();
		if (AudioMixerDevice)
		{
			SubmixListener->NumChannels = AudioMixerDevice->GetNumDeviceChannels();
			SubmixListener->SampleRate = AudioMixerDevice->GetDeviceSampleRate();
			SubmixListener->BufferLength = AudioMixerDevice->GetBufferLength();
			SubmixListener->SampleBuffer.SetCapacity(2 * SubmixListener->BufferLength * SubmixListener->NumChannels);
			SubmixListener->PoppedSamples.Empty();
			SubmixListener->PoppedSamples.AddUninitialized(SubmixListener->BufferLength * SubmixListener->NumChannels);
#if UE_5_4_OR_LATER
			if (!SubmixToRecord)
			{
				SubmixToRecord = &AudioMixerDevice->GetMainSubmixObject();
			}
			if(SubmixListener->DoesSharedInstanceExist())
			{
				AudioMixerDevice->RegisterSubmixBufferListener(SubmixListener, *SubmixToRecord);
			}
#else
			AudioMixerDevice->RegisterSubmixBufferListener(&SubmixListener.Get(), SubmixToRecord);
#endif
		}
		else
		{
			UE_LOG(LogAkAudio, Warning, TEXT("AkSubmixInputComponent::PostAssociatedAudioInputEvent (%s): No Audio Mixer Device available, AkSubMixInputComponent cannot work."), *GetOwner()->GetName());
			return PlayingID;
		}

		PlayingID = Super::PostAssociatedAudioInputEvent();
	}
	else
	{
		UE_LOG(LogAkAudio, Log, TEXT("AkSubmixInputComponent (%s): Event was already posted."), *GetOwner()->GetName());
	}
	return PlayingID;
}

void UDEPRECATED_UAkSubmixInputComponent::Stop()
{
	Audio::FMixerDevice* AudioMixerDevice = GetAudioMixerDevice();
	if (AudioMixerDevice)
	{
#if UE_5_4_OR_LATER
		if (UNLIKELY(!SubmixToRecord))
		{
			SubmixToRecord = &AudioMixerDevice->GetMainSubmixObject();
		}
		if(SubmixListener->DoesSharedInstanceExist())
		{
			AudioMixerDevice->UnregisterSubmixBufferListener(SubmixListener, *SubmixToRecord);
		}
#else
		AudioMixerDevice->UnregisterSubmixBufferListener(&SubmixListener.Get(), SubmixToRecord);
#endif
	}
	Super::Stop();
	PlayingID = AK_INVALID_PLAYING_ID;
}

bool UDEPRECATED_UAkSubmixInputComponent::FillSamplesBuffer(uint32 InNumChannels, uint32 InNumSamples, float** InOutBufferToFill)
{
	check(InNumChannels == SubmixListener->NumChannels);
	if (SubmixListener->SampleBuffer.Num() >= (InNumChannels * InNumSamples))
	{
		auto NumPopped = SubmixListener->SampleBuffer.Pop(SubmixListener->PoppedSamples.GetData(), InNumChannels * InNumSamples);
		if (NumPopped == InNumChannels * InNumSamples)
		{
			for (uint32 Channel = 0; Channel < InNumChannels; Channel++)
			{
				for (uint32 Sample = 0; Sample < InNumSamples; Sample++)
				{
					InOutBufferToFill[Channel][Sample] = SubmixListener->PoppedSamples[((SubmixListener->NumChannels * Sample) + Channel)];
				}
			}

			return true;
		}
	}

	for (uint32 Channel = 0; Channel < InNumChannels; Channel++)
	{
		FMemory::Memset(InOutBufferToFill[Channel], 0, InNumSamples * sizeof(float));
	}
	return true;
}

void UDEPRECATED_UAkSubmixInputComponent::GetChannelConfig(AkAudioFormat& AudioFormat)
{
	Audio::FMixerDevice* AudioMixerDevice = GetAudioMixerDevice();
	if (!AudioMixerDevice)
	{
		UE_LOG(LogAkAudio, Error, TEXT("AkSubmixInputComponent::GetChannelConfig (%s): No Audio Mixer Device available, AkSubMixInputComponent cannot work."), *GetOwner()->GetName());
		return;
	}

    AudioFormat.uSampleRate = SubmixListener->SampleRate;
	switch (SubmixListener->NumChannels)
	{
	case 8:
		AudioFormat.channelConfig.SetStandard(AK_SPEAKER_SETUP_7POINT1);
		break;
	case 6:
		AudioFormat.channelConfig.SetStandard(AK_SPEAKER_SETUP_5POINT1);
		break;
	case 2:
		AudioFormat.channelConfig.SetStandard(AK_SPEAKER_SETUP_STEREO);
		break;
	case 1:
		AudioFormat.channelConfig.SetStandard(AK_SPEAKER_SETUP_MONO);
		break;
	default:
		UE_LOG(LogAkAudio, Error, TEXT("AkSubmixInputComponent::GetChannelConfig (%s): Unknown number of channels (%" PRIu32 ")"), *GetOwner()->GetName(), SubmixListener->NumChannels);
	}
}