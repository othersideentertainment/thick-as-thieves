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

#pragma once

#include "WwiseDefines.h"
#include "Wwise/Stats/Memory.h"

LLM_DECLARE_TAG_API(Audio_Wwise_SoundEngine, AKAUDIO_API);

#if WWISE_2024_1_OR_LATER
DECLARE_MEMORY_STAT_EXTERN(TEXT("SoundEngine Reserved (Primary)"), STAT_WwiseMemorySoundEnginePrimary, STATGROUP_WwiseMemory, AKAUDIO_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("SoundEngine Reserved (Media)"), STAT_WwiseMemorySoundEngineMedia, STATGROUP_WwiseMemory, AKAUDIO_API);
DECLARE_MEMORY_STAT_EXTERN(TEXT("SoundEngine Reserved (Profiler)"), STAT_WwiseMemorySoundEngineProfiler, STATGROUP_WwiseMemory, AKAUDIO_API);
#else
DECLARE_MEMORY_STAT_EXTERN(TEXT("SoundEngine Reserved"), STAT_WwiseMemorySoundEngineVM, STATGROUP_WwiseMemory, AKAUDIO_API);
#endif

#if AK_SUPPORT_DEVICE_MEMORY
DECLARE_MEMORY_STAT_EXTERN(TEXT("SoundBank Reserved (Device)"), STAT_WwiseMemorySoundEngineDevice, STATGROUP_WwiseMemory, AKAUDIO_API);
#endif
