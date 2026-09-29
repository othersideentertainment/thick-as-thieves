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

#include "Platforms/AkPlatform_PS5/AkPS5InitializationSettings.h"
#include "Wwise/Stats/AkAudioMemory.h"
#include "AkAudioDevice.h"
#include "WwiseDefines.h"

// This macro controls whether or not a separate heap should be used for device-specific memory for Ajm decoding.
// This can be disabled to slightly improve memory utilization and memory tracking if the memory provided by
// Unreal via the non-device memory hooks provides ACP_RW as a part of its virtual-memory protection flags.
// e.g. if FPS5PlatformMemory::FPlatformVirtualMemoryBlock::CommitWithOS is modified to provide memory with protection flags of...
//		SCE_KERNEL_PROT_CPU_RW | SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_AMPR_ALL | SCE_KERNEL_PROT_ACP_RW
#define WWISE_PS5_ENABLE_DEVICE_MEMORY 1

#if defined(AK_PS5) && WWISE_PS5_ENABLE_DEVICE_MEMORY
namespace AkInitializationSettings_Helpers
{
#if WWISE_2024_1_OR_LATER
	// Hooks to track Device memory reservation
	void* AkPs5MemAllocSpanDevice(size_t size, size_t* extra)
	{
		ASYNC_INC_MEMORY_STAT_BY(STAT_WwiseMemorySoundEngineDevice, size);
		LLM_SCOPE_BYTAG(Audio_Wwise_SoundEngine);
		return AKPLATFORM::AllocDeviceSpan(size, extra);
	}
	void AkPs5MemFreeSpanDevice(void* address, size_t size, size_t extra)
	{
		AKPLATFORM::FreeDeviceSpan(address, size, extra);
		ASYNC_DEC_MEMORY_STAT_BY(STAT_WwiseMemorySoundEngineDevice, size);
	}
#else
	// forward device calls to stock Wwise allocdevice calls
	void* AkPs5MemAllocDevice(size_t size, size_t* extra)
	{
		ASYNC_INC_MEMORY_STAT_BY(STAT_WwiseMemorySoundEngineDevice, size);
		return AKPLATFORM::AllocDevice(size, extra);
	}

	void AkPs5MemFreeDevice(void* address, size_t size, size_t extra, size_t release)
	{
		if (release)
		{
			AKPLATFORM::FreeDevice(address, size, extra, release);
			ASYNC_DEC_MEMORY_STAT_BY(STAT_WwiseMemorySoundEngineDevice, release);
		}
	}
#endif // WWISE_2024_1_OR_LATER
}
#endif

///////////////////////////////////////////////////////////////////////////
// UAkPS5InitializationSettings

void FAkPS5AdvancedInitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{
	Super::FillInitializationStructure(InitializationStructure);
	
#ifdef AK_PS5
	InitializationStructure.PlatformInitSettings.bHwCodecLowLatencyMode = UseHardwareCodecLowLatencyMode;
	InitializationStructure.PlatformInitSettings.bVorbisHwAcceleration = bVorbisHwAcceleration;
	InitializationStructure.PlatformInitSettings.bEnable3DAudioSync = bEnable3DAudioSync;
	InitializationStructure.PlatformInitSettings.uNumOperationsForHwMixing = uNumOperationsForHwMixing;
	InitializationStructure.PlatformInitSettings.bPlotQueueLevel = bPlotQueueLevel;
	InitializationStructure.PlatformInitSettings.uNumAudioOut2Ports = NumAudioOut2Ports;
	InitializationStructure.PlatformInitSettings.uNumAudioOut2ObjectPorts = NumAudioOut2ObjectPorts;
	
#if WWISE_PS5_ENABLE_DEVICE_MEMORY
	#if WWISE_2024_1_OR_LATER
		InitializationStructure.MemSettings.memoryArenaSettings[AkMemoryMgrArena_Device].fnMemAllocSpan = AkInitializationSettings_Helpers::AkPs5MemAllocSpanDevice;
		InitializationStructure.MemSettings.memoryArenaSettings[AkMemoryMgrArena_Device].fnMemFreeSpan = AkInitializationSettings_Helpers::AkPs5MemFreeSpanDevice;
	#else
		InitializationStructure.MemSettings.pfAllocDevice = AkInitializationSettings_Helpers::AkPs5MemAllocDevice;
		InitializationStructure.MemSettings.pfFreeDevice = AkInitializationSettings_Helpers::AkPs5MemFreeDevice;
	#endif // WWISE_2024_1_OR_LATER
#else
	#if WWISE_2024_1_OR_LATER
		InitializationStructure.MemSettings.memoryArenaSettings[AkMemoryMgrArena_Device].fnMemAllocSpan = nullptr;
		InitializationStructure.MemSettings.memoryArenaSettings[AkMemoryMgrArena_Device].fnMemFreeSpan = nullptr;
	#elif WWISE_2023_1_OR_LATER
		InitializationStructure.MemSettings.pfAllocDevice = nullptr;
		InitializationStructure.MemSettings.pfFreeDevice = nullptr;
		InitializationStructure.MemSettings.bEnableSeparateDeviceHeap = false; // normal Alloc/Free VM calls handle all allocations
	#else
		// on pre-23.1, we "disable" device memory by setting bUseDeviceMemAlways to true,
		// and direct the "device" alloc hook to the generic alloc hook
		InitializationStructure.MemSettings.pfAllocDevice = InitializationStructure.MemSettings.pfAllocVM;
		InitializationStructure.MemSettings.pfFreeDevice = InitializationStructure.MemSettings.pfFreeVM;
		InitializationStructure.MemSettings.bUseDeviceMemAlways = true;
	#endif // WWISE_2023_1_OR_LATER
#endif // WWISE_PS5_ENABLE_DEVICE_MEMORY

#endif // AK_PS5
}


//////////////////////////////////////////////////////////////////////////
// UAkPS5InitializationSettings

UAkPS5InitializationSettings::UAkPS5InitializationSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAkPS5InitializationSettings::FillInitializationStructure(FAkInitializationStructure& InitializationStructure) const
{
	InitializationStructure.SetPluginDllPath("PS5");

	CommonSettings.FillInitializationStructure(InitializationStructure);
	CommunicationSettings.FillInitializationStructure(InitializationStructure);
	AdvancedSettings.FillInitializationStructure(InitializationStructure);
}
