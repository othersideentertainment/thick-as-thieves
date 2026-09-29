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

#if defined(PLATFORM_PS5) && PLATFORM_PS5

#include "Platforms/AkPlatform_PS5/AkPS5Platform.h"
#include "AkAudioDevice.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Interfaces/IPluginManager.h"

#include "Wwise/API/WwisePlatformAPI.h"

#define AK_PS5_DSP_PLUGINS_DIR "/app0/prx/"

FString FAkPS5Platform::GetWwiseSoundEnginePluginDirectory()
{
    // Is it not possible to get an absolute path on PS4, so we build it ourselves...
    FString BaseDirectory = TEXT("/app0");
    if (IPluginManager::Get().FindPlugin("Wwise")->GetType() == EPluginType::Engine)
    {
        BaseDirectory /= TEXT("engine");
    }
    else
    {
        BaseDirectory /= FApp::GetProjectName();
    }

    return BaseDirectory / TEXT("Plugins") / TEXT("Wwise");
}

FString FAkPS5Platform::GetDSPPluginsDirectory(const FString& PlatformArchitecture)
{
    return AK_PS5_DSP_PLUGINS_DIR;
}

void FAkPS5Platform::PreInitialize(const FAkInitializationStructure& InitSettings)
{
    // We only need to do this once for the whole application lifetime.
    static bool bVorbisHwAcceleratorLoaded = false;
	auto Platform = IWwisePlatformAPI::Get();
	if (UNLIKELY(!Platform))
	{
		return;
	}
    if (!bVorbisHwAcceleratorLoaded && InitSettings.PlatformInitSettings.bVorbisHwAcceleration)
    {
        AKRESULT eResult = Platform->LoadVorbisHwAcceleratorLibrary("AkVorbisHwAccelerator", AK_PS5_DSP_PLUGINS_DIR);
        switch (eResult)
        {
            case AK_Success:
                bVorbisHwAcceleratorLoaded = true;
                break;
            // A failure here is not critical. We emit the proper error in the log, but we can allow the sound engine to continue to initialize.
            case AK_NotCompatible:
                UE_LOG(LogAkAudio, Error, TEXT("Vorbis acceleration library failed to load due to PS5 SDK mismatch."));
                break;
            case AK_InvalidFile:
                UE_LOG(LogAkAudio, Error, TEXT("Vorbis acceleration library failed to load because the library file could not be found in the game package."));
                break;
            case AK_InsufficientMemory:
                UE_LOG(LogAkAudio, Error, TEXT("Vorbis acceleration library failed to load due to insufficient resources or memory."));
                break;
            default:
                UE_LOG(LogAkAudio, Error, TEXT("Vorbis acceleration library failed to load due an unexpected error."));
                break;
        }
    }
}

#endif