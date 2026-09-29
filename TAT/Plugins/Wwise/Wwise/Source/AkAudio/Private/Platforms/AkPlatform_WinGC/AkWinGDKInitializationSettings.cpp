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

#include "Platforms/AkPlatform_WinGC/AkWinGDKInitializationSettings.h"
#include "AkAudioDevice.h"
#include "IHeadMountedDisplayModule.h"

#include "Wwise/API/WwisePlatformAPI.h"

//////////////////////////////////////////////////////////////////////////
// FAkWinGDKAdvancedInitializationSettings

void FAkWinGDKAdvancedInitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{
	Super::FillInitializationStructure(InitializationStructure);

#ifdef AK_WINDOWSGC
	auto Platform = IWwisePlatformAPI::Get();
	if (UNLIKELY(!Platform))
	{
		return;
	}

	if (UseHeadMountedDisplayAudioDevice && IHeadMountedDisplayModule::IsAvailable())
	{
		FString AudioOutputDevice = IHeadMountedDisplayModule::Get().GetAudioOutputDevice();
		if (!AudioOutputDevice.IsEmpty())
			InitializationStructure.InitSettings.settingsMainOutput.idDevice = Platform->GetDeviceIDFromName((wchar_t*)*AudioOutputDevice);
	}
	InitializationStructure.PlatformInitSettings.uMaxSystemAudioObjects = uMaxSystemAudioObjects;
#endif // AK_WINDOWSGC
}


//////////////////////////////////////////////////////////////////////////
// UAkWinGDKInitializationSettings

UAkWinGDKInitializationSettings::UAkWinGDKInitializationSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAkWinGDKInitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{

#ifdef AK_WINGC_VS_VERSION
	constexpr auto PlatformArchitecture = "WinGC_" AK_WINGC_VS_VERSION;
#else
	constexpr auto PlatformArchitecture = "WinGC_vc160";
#endif

	InitializationStructure.SetPluginDllPath(PlatformArchitecture);

	CommonSettings.FillInitializationStructure(InitializationStructure);
	CommunicationSettings.FillInitializationStructure(InitializationStructure);
	AdvancedSettings.FillInitializationStructure(InitializationStructure);

#ifdef AK_WINDOWSGC
	InitializationStructure.PlatformInitSettings.uSampleRate = CommonSettings.SampleRate;
#endif
}
