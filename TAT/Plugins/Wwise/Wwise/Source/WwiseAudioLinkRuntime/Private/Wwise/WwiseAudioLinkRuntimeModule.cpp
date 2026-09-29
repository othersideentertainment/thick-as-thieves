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

#include "Wwise/WwiseAudioLinkRuntimeModule.h"
#include "Wwise/AudioLink/WwiseAudioLinkFactory.h"

IMPLEMENT_MODULE(FWwiseAudioLinkRuntimeModule, WwiseAudioLinkRuntime)

FWwiseAudioLinkRuntimeModule::FWwiseAudioLinkRuntimeModule() {}
FWwiseAudioLinkRuntimeModule::~FWwiseAudioLinkRuntimeModule() {}

#if !UE_SERVER
#include "Misc/ConfigCacheIni.h"
#endif

void FWwiseAudioLinkRuntimeModule::StartupModule()
{
#if !UE_SERVER
	bool bWwiseAudioLinkEnabled = false;
	GConfig->GetBool(TEXT("/Script/AkAudio.AkSettings"), TEXT("bWwiseAudioLinkEnabled"), bWwiseAudioLinkEnabled, GGameIni);
	if (bWwiseAudioLinkEnabled)
	{
		Factory = MakeUnique<FWwiseAudioLinkFactory>();
	}
#endif
}

void FWwiseAudioLinkRuntimeModule::ShutdownModule()
{
	Factory.Reset();
}
