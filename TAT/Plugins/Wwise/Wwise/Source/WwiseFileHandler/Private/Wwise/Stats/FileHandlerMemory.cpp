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

#include "Wwise/Stats/FileHandlerMemory.h"
#include "Wwise/Stats/Memory.h"

DEFINE_STAT(STAT_WwiseMemoryExtSrc);
#if AK_SUPPORT_DEVICE_MEMORY
DEFINE_STAT(STAT_WwiseMemoryExtSrcDevice);
#endif

DEFINE_STAT(STAT_WwiseMemoryExtSrcPrefetch);
#if AK_SUPPORT_DEVICE_MEMORY
DEFINE_STAT(STAT_WwiseMemoryExtSrcPrefetchDevice);
#endif

DEFINE_STAT(STAT_WwiseMemoryMedia);
#if AK_SUPPORT_DEVICE_MEMORY
DEFINE_STAT(STAT_WwiseMemoryMediaDevice);
#endif

DEFINE_STAT(STAT_WwiseMemoryMediaPrefetch);
#if AK_SUPPORT_DEVICE_MEMORY
DEFINE_STAT(STAT_WwiseMemoryMediaPrefetchDevice);
#endif

DEFINE_STAT(STAT_WwiseMemorySoundBank);
#if AK_SUPPORT_DEVICE_MEMORY
DEFINE_STAT(STAT_WwiseMemorySoundBankDevice);
#endif

LLM_DEFINE_TAG(Audio_Wwise_FileHandler, TEXT("Wwise FileHandler"));
LLM_DEFINE_TAG(Audio_Wwise_FileHandler_ExternalSources, TEXT("Wwise External Source File Handles"));
LLM_DEFINE_TAG(Audio_Wwise_FileHandler_Media, TEXT("Wwise Media File Handles"));
LLM_DEFINE_TAG(Audio_Wwise_FileHandler_SoundBanks, TEXT("Wwise SoundBank File Handles"));

#if WITH_EDITORONLY_DATA
DEFINE_STAT(STAT_WwiseMemoryCooking);
LLM_DEFINE_TAG(Audio_Wwise_FileHandler_Cooking, TEXT("Cooking"));
#endif
