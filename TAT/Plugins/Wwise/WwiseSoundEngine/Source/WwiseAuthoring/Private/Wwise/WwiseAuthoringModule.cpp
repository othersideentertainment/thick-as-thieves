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

#include "Wwise/WwiseAuthoringModule.h"
#include "Wwise/API/WAAPI.h"
#include "Wwise/API/WAAPI_Bridge.h"
#include "Wwise/API/WAAPI_Null.h"
#include "Wwise/Stats/SoundEngine.h"

#include "Misc/ScopeLock.h"
#include "Wwise/WwiseAssertHook.h"

IMPLEMENT_MODULE(FWwiseAuthoringModule, WwiseAuthoring)

IWAAPI* IWwiseAuthoringModule::WAAPI = nullptr;

void FWwiseAuthoringModule::StartupModule()
{
	UE_LOG(LogWwiseSoundEngine, Display, TEXT("Loading WwiseAuthoring"));

#if AK_SUPPORT_WAAPI
#if AK_USE_NULL_SOUNDENGINE
	WAAPI = new FWAAPI_Null;
#else
	WAAPI = new FWAAPI_Bridge;
#endif
#endif
}

void FWwiseAuthoringModule::ShutdownModule()
{
	DeleteInterface();
}

void FWwiseAuthoringModule::DeleteInterface()
{
	UE_LOG(LogWwiseSoundEngine, Display, TEXT("Unloading WwiseAuthoring"));
	delete WAAPI; WAAPI = nullptr;
}
