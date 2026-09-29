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

#include "Wwise/WwiseSwitchContainerLeafGroupValueUsageCount.h"

#include "Wwise/Stats/ResourceLoader.h"

FWwiseSwitchContainerLeafGroupValueUsageCount::FLoadedData::FLoadedData()
{
}

bool FWwiseSwitchContainerLeafGroupValueUsageCount::FLoadedData::IsLoaded() const
{
	return LoadedSoundBanks.Num() > 0 || LoadedExternalSources.Num() > 0 || LoadedMedia.Num() > 0;
}

FWwiseSwitchContainerLeafGroupValueUsageCount::FWwiseSwitchContainerLeafGroupValueUsageCount(
	const FWwiseSwitchContainerLeafCookedData& InKey):
	Key(InKey)
{}

bool FWwiseSwitchContainerLeafGroupValueUsageCount::HaveAllKeys() const
{
	if (UNLIKELY(Key.GroupValueSet.Num() < LoadedGroupValues.Num()))
	{
		UE_LOG(LogWwiseResourceLoader, Error, TEXT("Have more keys loaded (%d) than existing in key (%d) @ %p for key %s"),
			LoadedGroupValues.Num(), Key.GroupValueSet.Num(), &LoadedData, *Key.GetDebugString());
		return true;
	}

	return Key.GroupValueSet.Num() == LoadedGroupValues.Num();
}

