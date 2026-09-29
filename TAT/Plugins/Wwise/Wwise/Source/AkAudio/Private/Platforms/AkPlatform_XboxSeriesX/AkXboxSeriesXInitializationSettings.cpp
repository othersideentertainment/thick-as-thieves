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

#include "Platforms/AkPlatform_XboxSeriesX/AkXboxSeriesXInitializationSettings.h"
#include "Wwise/Stats/AkAudioMemory.h"
#include "AkAudioDevice.h"
#include "WwiseDefines.h"

//////////////////////////////////////////////////////////////////////////
// FAkXSXAdvancedInitializationSettings

void FAkXSXAdvancedInitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{
	Super::FillInitializationStructure(InitializationStructure);

#ifdef AK_XBOXSERIESX
	InitializationStructure.PlatformInitSettings.bHwCodecLowLatencyMode = UseHardwareCodecLowLatencyMode;
	InitializationStructure.PlatformInitSettings.uMaxOpusVoices = MaximumNumberOfOpusVoices;
	InitializationStructure.PlatformInitSettings.uMaxSystemAudioObjects = uMaxSystemAudioObjects;

#if WWISE_2023_1_OR_LATER 
	// Only set Xdsp properties if XDSP is enabled on Lockhart (Series S) or we're not running on a Lockhart
	// Otherwise, set it to zero, to reclaim the memory
	if (EnableXdspOnLockhart || FPlatformMisc::GetConsoleType() != EXSXConsoleType::Lockhart)
	{
		InitializationStructure.PlatformInitSettings.uMaxXdspStreams = MaxXdspStreams;
		InitializationStructure.PlatformInitSettings.uMaxXdspAggregateStreamLength = MaxXdspAggregateStreamLength;
	}
	else
	{
		InitializationStructure.PlatformInitSettings.uMaxXdspStreams = 0;
		InitializationStructure.PlatformInitSettings.uMaxXdspAggregateStreamLength = 0;
	}
#endif

	// Xbox doesn't use device memory anymore on 2024.1 or later,
	// so leave the device memory hooks to the default of "nullptr"
#if !WWISE_2024_1_OR_LATER
	#if WWISE_2023_1_OR_LATER
		InitializationStructure.MemSettings.pfAllocDevice = nullptr;
		InitializationStructure.MemSettings.pfFreeDevice = nullptr;
		InitializationStructure.MemSettings.bEnableSeparateDeviceHeap = false;
	#else
		// on pre-23.1, we "disable" device memory by setting bUseDeviceMemAlways to true,
		// and direct the "device" alloc hook to the generic alloc hook
		InitializationStructure.MemSettings.pfAllocDevice = InitializationStructure.MemSettings.pfAllocVM;
		InitializationStructure.MemSettings.pfFreeDevice = InitializationStructure.MemSettings.pfFreeVM;
		InitializationStructure.MemSettings.bUseDeviceMemAlways = true;
	#endif // WWISE_2023_1_OR_LATER
#endif // !WWISE_2024_1_OR_LATER

#endif // AK_XBOXSERIESX
}


//////////////////////////////////////////////////////////////////////////
// UAkXboxSeriesXInitializationSettings

UAkXboxSeriesXInitializationSettings::UAkXboxSeriesXInitializationSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CommunicationSettings.DiscoveryBroadcastPort = FAkCommunicationSettings::DefaultDiscoveryBroadcastPort;
	CommunicationSettings.CommandPort = FAkCommunicationSettings::DefaultDiscoveryBroadcastPort + 1;
}

void UAkXboxSeriesXInitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{
#ifdef AK_XBOXSERIESX_VS_VERSION
	constexpr auto PlatformArchitecture = "XboxSeriesX_" AK_XBOXSERIESX_VS_VERSION;
#else
	constexpr auto PlatformArchitecture = "XboxSeriesX_vc160";
#endif

	InitializationStructure.SetPluginDllPath(PlatformArchitecture);

	CommonSettings.FillInitializationStructure(InitializationStructure);
	CommunicationSettings.FillInitializationStructure(InitializationStructure);
	AdvancedSettings.FillInitializationStructure(InitializationStructure);
}
