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

#include "Wwise/AudioLink/WwiseAudioLinkSynchronizer.h"

#include "AkAudioDevice.h"
#include "Wwise/WwiseGlobalCallbacks.h"
#include "Wwise/WwiseSoundEngineModule.h"
#include "Wwise/API/WwiseSoundEngineAPI.h"
#include "Wwise/Stats/AudioLink.h"

FWwiseAudioLinkSynchronizer::FWwiseAudioLinkSynchronizer()
{}

FWwiseAudioLinkSynchronizer::~FWwiseAudioLinkSynchronizer()
{
	Unbind();
}

void FWwiseAudioLinkSynchronizer::ExecuteBeginRender(AK::IAkGlobalPluginContext* InContext)
{
	FOnRenderParams Params;
	Params.BufferTickID = InContext->GetBufferTick();
	Params.NumFrames = InContext->GetMaxBufferLength();
	OnBeginRender.Broadcast(Params);
}

void FWwiseAudioLinkSynchronizer::ExecuteEndRender(AK::IAkGlobalPluginContext* InContext)
{
	FOnRenderParams Params;
	Params.BufferTickID = InContext->GetBufferTick();
	Params.NumFrames = InContext->GetMaxBufferLength();
	OnEndRender.Broadcast(Params);
}

bool FWwiseAudioLinkSynchronizer::ExecuteOpenStream()
{
	UE_LOG(LogWwiseAudioLink, Verbose, TEXT("FWwiseAudioLinkSynchronizer::ExecuteOpenStream: Opening stream between Unreal and Wwise."));

	auto* SoundEngine = IWwiseSoundEngineModule::SoundEngine;
	if (UNLIKELY(!SoundEngine))
	{
		UE_LOG(LogWwiseAudioLink, Error, TEXT("FWwiseAudioLinkSynchronizer::ExecuteOpenStream: No Sound Engine."));
		return false;
	}

	AkAudioSettings Settings;
	SoundEngine->GetAudioSettings(Settings);					
	const AkChannelConfig SinkConfig = SoundEngine->GetSpeakerConfiguration();

	FOnOpenStreamParams Params;
	if (!SinkConfig.IsValid())
	{
		UE_LOG(LogWwiseAudioLink, Warning, TEXT("FWwiseAudioLinkSynchronizer::ExecuteOpenStream: Cannot get SpeakerConfiguration. Is Sound Engine Loaded / are SoundBanks generated and available?."));
		Params.NumChannels = INDEX_NONE;
		Params.SampleRate = INDEX_NONE;
		Params.NumFrames = INDEX_NONE;
		Params.Name = TEXT("AudioLink for Wwise (Invalid)");
		return false;
	}

	Params.NumChannels = SinkConfig.uNumChannels;
	Params.SampleRate = Settings.uNumSamplesPerSecond;
	Params.NumFrames = Settings.uNumSamplesPerFrame;
	Params.Name = TEXT("AudioLink for Wwise");

	OnOpenStream.Broadcast(Params);

	return true;
}

bool FWwiseAudioLinkSynchronizer::ExecuteCloseStream()
{
	UE_LOG(LogWwiseAudioLink, Verbose, TEXT("FWwiseAudioLinkSynchronizer::ExecuteCloseStream: Closing stream between Unreal and Wwise."));

	OnCloseStream.Broadcast();

	return true;
}

bool FWwiseAudioLinkSynchronizer::Bind()
{

	if (UNLIKELY(BindingState == ESynchronizerBindingState::Bound))
	{
		UE_LOG(LogWwiseAudioLink, Error, TEXT("FWwiseAudioLinkSynchronizer::Bind: Binding an already bound SoundEngine."));
		return false;
	}
	
	BindingState = ESynchronizerBindingState::Binding;

	auto* Callbacks = FWwiseGlobalCallbacks::Get();
	if (UNLIKELY(!Callbacks))
	{
		UE_LOG(LogWwiseAudioLink, Error, TEXT("FWwiseAudioLinkSynchronizer::Bind: No Callbacks."));
		BindingState = ESynchronizerBindingState::Unbound;
		return false;
	}

	auto* SoundEngine = IWwiseSoundEngineModule::SoundEngine;
	if (UNLIKELY(!SoundEngine))
	{
		UE_LOG(LogWwiseAudioLink, Error, TEXT("FWwiseAudioLinkSynchronizer::ExecuteOpenStream: No Sound Engine."));
		BindingState = ESynchronizerBindingState::Unbound;
		return false;
	}

	UE_LOG(LogWwiseAudioLink, Verbose, TEXT("FWwiseAudioLinkSynchronizer::Bind: Binding SoundEngine."));
	Callbacks->BeginRenderSync([WeakThis = AsWeak()](AK::IAkGlobalPluginContext* InContext) mutable
	{
		auto This = WeakThis.Pin();
		if (UNLIKELY(!This.IsValid() || This->BindingState == ESynchronizerBindingState::Unbound))
		{
			return EWwiseDeferredAsyncResult::Done;
		}

		This->ExecuteBeginRender(InContext);
		return EWwiseDeferredAsyncResult::KeepRunning;
	});

	Callbacks->EndRenderSync([WeakThis = AsWeak()](AK::IAkGlobalPluginContext* InContext) mutable
	{
		auto This = WeakThis.Pin();
		if (UNLIKELY(!This.IsValid() || This->BindingState == ESynchronizerBindingState::Unbound))
		{
			return EWwiseDeferredAsyncResult::Done;
		}

		This->ExecuteEndRender(InContext);
		return EWwiseDeferredAsyncResult::KeepRunning;
	});

	if (SoundEngine->IsInitialized())
	{
		if (!ExecuteOpenStream())
		{
			BindingState = ESynchronizerBindingState::Unbound;
			return false;
		}
	}
	Callbacks->InitAsync([WeakThis = AsWeak()]() mutable
	{
		auto This = WeakThis.Pin();
		if (UNLIKELY(!This.IsValid() || This->BindingState == ESynchronizerBindingState::Unbound))
		{
			return EWwiseDeferredAsyncResult::Done;
		}

		This->ExecuteOpenStream();
		return EWwiseDeferredAsyncResult::KeepRunning;
	});

	Callbacks->TermAsync([WeakThis = AsWeak()]() mutable
	{
		auto This = WeakThis.Pin();
		if (UNLIKELY(!This.IsValid() || This->BindingState == ESynchronizerBindingState::Unbound))
		{
			return EWwiseDeferredAsyncResult::Done;
		}

		This->ExecuteCloseStream();
		return EWwiseDeferredAsyncResult::KeepRunning;
	});
	BindingState = ESynchronizerBindingState::Bound;
	return true;
}

bool FWwiseAudioLinkSynchronizer::Unbind()
{
	if (UNLIKELY(BindingState == ESynchronizerBindingState::Unbound))
	{
		return false;
	}

	UE_LOG(LogWwiseAudioLink, Verbose, TEXT("FWwiseAudioLinkSynchronizer::Bind: Unbinding SoundEngine."));
	BindingState = ESynchronizerBindingState::Unbound;

	if (!ExecuteCloseStream())
	{
		return false;
	}

	return true;
}
