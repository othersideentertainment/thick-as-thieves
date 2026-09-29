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

#include "Wwise/Ref/WwiseRefSwitchGroup.h"

#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Metadata/WwiseMetadataSwitchGroup.h"

const WwiseDBString WwiseRefSwitchGroup::NAME = "SwitchGroup"_wwise_db;

const WwiseMetadataSwitchGroup* WwiseRefSwitchGroup::GetSwitchGroup() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& SwitchGroups = SoundBank->SwitchGroups;
	if (SwitchGroups.IsValidIndex(SwitchGroupIndex))
	{
		return &SwitchGroups[SwitchGroupIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Switch Group index #%zu", SwitchGroupIndex);
		return nullptr;
	}
}

bool WwiseRefSwitchGroup::IsControlledByGameParameter() const
{
	const auto* SwitchGroup = GetSwitchGroup();
	if (!SwitchGroup) [[unlikely]]
	{
		return false;
	}

	return SwitchGroup->GameParameterRef != nullptr;
}

WwiseDBShortId WwiseRefSwitchGroup::SwitchGroupId() const
{
	const auto* SwitchGroup = GetSwitchGroup();
	if (!SwitchGroup) [[unlikely]]
	{
		return 0;
	}
	return SwitchGroup->Id;
}

WwiseDBGuid WwiseRefSwitchGroup::SwitchGroupGuid() const
{
	const auto* SwitchGroup = GetSwitchGroup();
	if (!SwitchGroup) [[unlikely]]
	{
		return {};
	}
	return SwitchGroup->GUID;
}

const WwiseDBString* WwiseRefSwitchGroup::SwitchGroupName() const
{
	const auto* SwitchGroup = GetSwitchGroup();
	if (!SwitchGroup) [[unlikely]]
	{
		return &emptyString;
	}
	return &SwitchGroup->Name;
}

const WwiseDBString* WwiseRefSwitchGroup::SwitchGroupObjectPath() const
{
	const auto* SwitchGroup = GetSwitchGroup();
	if (!SwitchGroup) [[unlikely]]
	{
		return &emptyString;
	}
	return &SwitchGroup->ObjectPath;
}

WwiseDBShortId WwiseRefSwitchGroup::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(SwitchGroupIndex));
	return Result;
}
