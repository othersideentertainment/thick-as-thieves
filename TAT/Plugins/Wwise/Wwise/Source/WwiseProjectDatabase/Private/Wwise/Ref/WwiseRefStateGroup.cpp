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

#include "Wwise/Ref/WwiseRefStateGroup.h"

#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

#include "Wwise/Metadata/WwiseMetadataStateGroup.h"

const WwiseDBString WwiseRefStateGroup::NAME = "StateGroup"_wwise_db;

const WwiseMetadataStateGroup* WwiseRefStateGroup::GetStateGroup() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& StateGroups = SoundBank->StateGroups;
	if (StateGroups.IsValidIndex(StateGroupIndex))
	{
		return &StateGroups[StateGroupIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get State Group index #%zu", StateGroupIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefStateGroup::StateGroupId() const
{
	const auto* StateGroup = GetStateGroup();
	if (!StateGroup) [[unlikely]]
	{
		return 0;
	}
	return StateGroup->Id;
}

WwiseDBGuid WwiseRefStateGroup::StateGroupGuid() const
{
	const auto* StateGroup = GetStateGroup();
	if (!StateGroup) [[unlikely]]
	{
		return {};
	}
	return StateGroup->GUID;
}

const WwiseDBString* WwiseRefStateGroup::StateGroupName() const
{
	const auto* StateGroup = GetStateGroup();
	if (!StateGroup) [[unlikely]]
	{
		return &emptyString;
	}
	return &StateGroup->Name;
}

const WwiseDBString* WwiseRefStateGroup::StateGroupObjectPath() const
{
	const auto* StateGroup = GetStateGroup();
	if (!StateGroup) [[unlikely]]
	{
		return &emptyString;
	}
	return &StateGroup->ObjectPath;
}

WwiseDBShortId WwiseRefStateGroup::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(StateGroupIndex));
	return Result;
}
