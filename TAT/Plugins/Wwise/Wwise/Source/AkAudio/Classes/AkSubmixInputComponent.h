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
AkSubmixInputComponent.h:
=============================================================================*/

#pragma once

#include "AkInclude.h"
#include "AudioDevice.h"
#include "AkAudioInputComponent.h"
#include "ISubmixBufferListener.h"
#include "AkSubmixInputComponent.generated.h"

struct FAkSubmixBufferListener : public ISubmixBufferListener
{
	int32 NumChannels = 0;
	int32 SampleRate = 0;
	int32 BufferLength = 0;
	Audio::TCircularAudioBuffer<float> SampleBuffer;
	TArray<float> PoppedSamples;
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock) override;
	virtual ~FAkSubmixBufferListener() {}
};

/*------------------------------------------------------------------------------------
UAkSubmixInputComponent
------------------------------------------------------------------------------------*/
UCLASS(Deprecated,
	DisplayName="AkSubmixInput ( DEPRECATED )",
	ClassGroup = Audiokinetic, BlueprintType,
	hidecategories = (Transform, Rendering, Mobility, LOD, Component, Activation),
	meta = (BlueprintSpawnableComponent, Tooltip = "(DEPRECATED) See AudioLink: https://www.audiokinetic.com/en/library/edge/?source=UE4&id=using_audio_link.html)"))
class AKAUDIO_API UDEPRECATED_UAkSubmixInputComponent
	: public UAkAudioInputComponent
{
    GENERATED_BODY()
public:
	UDEPRECATED_UAkSubmixInputComponent(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SubmixInput")
	TObjectPtr<USoundSubmix> SubmixToRecord = nullptr;

	virtual int32 PostAssociatedAudioInputEvent();
	virtual void Stop();
protected:
	/** The audio callback. This will be called continuously by the Wwise sound engine,
	  * and is used to provide the sound engine with audio samples. If this function returns false, the audio
	  * input event will be stopped and the function will stop being called.
	  */
	virtual bool FillSamplesBuffer(uint32 NumChannels, uint32 NumSamples, float** BufferToFill) override;
	/** This callback is used to provide the Wwise sound engine with the required audio format. */
	virtual void GetChannelConfig(AkAudioFormat& AudioFormat) override;

private:

	TSharedRef<FAkSubmixBufferListener> SubmixListener;
	Audio::FMixerDevice* GetAudioMixerDevice();
	int32 PlayingID = AK_INVALID_PLAYING_ID;
};