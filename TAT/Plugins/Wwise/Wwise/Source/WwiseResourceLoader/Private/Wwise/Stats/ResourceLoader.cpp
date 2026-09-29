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

#include "Wwise/Stats/ResourceLoader.h"
#include "Wwise/Stats/Memory.h"

DEFINE_STAT(STAT_WwiseResourceLoaderAssetLibraries);
DEFINE_STAT(STAT_WwiseResourceLoaderAuxBusses);
DEFINE_STAT(STAT_WwiseResourceLoaderEvents);
DEFINE_STAT(STAT_WwiseResourceLoaderExternalSources);
DEFINE_STAT(STAT_WwiseResourceLoaderGroupValues);
DEFINE_STAT(STAT_WwiseResourceLoaderInitBanks);
DEFINE_STAT(STAT_WwiseResourceLoaderMedia);
DEFINE_STAT(STAT_WwiseResourceLoaderShareSets);
DEFINE_STAT(STAT_WwiseResourceLoaderSoundBanks);
DEFINE_STAT(STAT_WwiseResourceLoaderSwitchContainerCombinations);

DEFINE_STAT(STAT_WwiseResourceLoaderTiming);

DEFINE_LOG_CATEGORY(LogWwiseResourceLoader);

#define WWISERESOURCELOADER_ASYNC_NAME(Text) TEXT("WwiseResourceLoader ") TEXT(Text)

LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader, TEXT("Wwise Resource Loader"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_AuxBusses, TEXT("Wwise Aux Busses"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_Events, TEXT("Wwise Events"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_ExternalSources, TEXT("Wwise External Sources"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_GroupValues, TEXT("Wwise Group Values"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_InitBanks, TEXT("Wwise Init Bank"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_Media, TEXT("Wwise Media"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_ShareSets, TEXT("Wwise ShareSets"));
LLM_DEFINE_TAG(Audio_Wwise_ResourceLoader_SoundBanks, TEXT("Wwise SoundBanks"));
