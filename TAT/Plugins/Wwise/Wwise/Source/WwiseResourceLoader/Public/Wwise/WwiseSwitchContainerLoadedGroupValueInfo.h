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

#include "Wwise/WwiseSharedGroupValueKey.h"
#include "Wwise/WwiseSwitchContainerLeafGroupValueUsageCount.h"

struct WWISERESOURCELOADER_API FWwiseSwitchContainerLoadedGroupValueInfo
{
	/**
	 * @brief GroupValue key this structure represents.
	*/
	FWwiseSharedGroupValueKey Key;

	/**
	 * @brief Number of times this particular GroupValue was loaded in the currently loaded maps.
	 * 
	 * Leaves are required for any value greater than 0.
	*/
	int GroupValueCount;

	/**
	 * @brief Leaves that use this particular GroupValue.
	 * 
	 * @note The ownership of this pointer is uniquely created and discarded during SwitchContainerLeaf loading and unloading.
	*/
	TSet<TSharedRef<FWwiseSwitchContainerLeafGroupValueUsageCount, ESPMode::ThreadSafe>> Leaves;

	FWwiseSwitchContainerLoadedGroupValueInfo(const FWwiseSharedGroupValueKey& InKey) :
		Key(InKey),
		GroupValueCount(0),
		Leaves()
	{}

	bool ResourcesAreLoaded() const
	{
		check(GroupValueCount >= 0);

		return GroupValueCount > 0;
	}

	bool operator ==(const FWwiseSwitchContainerLoadedGroupValueInfo& InRhs) const
	{
		return Key == InRhs.Key;
	}

	bool operator !=(const FWwiseSwitchContainerLoadedGroupValueInfo& InRhs) const
	{
		return Key != InRhs.Key;
	}

	bool operator <(const FWwiseSwitchContainerLoadedGroupValueInfo& InRhs) const
	{
		return Key < InRhs.Key;
	}

	bool operator <=(const FWwiseSwitchContainerLoadedGroupValueInfo& InRhs) const
	{
		return Key <= InRhs.Key;
	}

	bool operator >(const FWwiseSwitchContainerLoadedGroupValueInfo& InRhs) const
	{
		return Key > InRhs.Key;
	}

	bool operator >=(const FWwiseSwitchContainerLoadedGroupValueInfo& InRhs) const
	{
		return Key >= InRhs.Key;
	}

	bool operator ==(const FWwiseSharedGroupValueKey& InRhs) const
	{
		return Key == InRhs;
	}

	bool operator !=(const FWwiseSharedGroupValueKey& InRhs) const
	{
		return Key != InRhs;
	}
};
inline uint32 GetTypeHash(const FWwiseSwitchContainerLoadedGroupValueInfo& InValue)
{
	return GetTypeHash(InValue.Key);
}
