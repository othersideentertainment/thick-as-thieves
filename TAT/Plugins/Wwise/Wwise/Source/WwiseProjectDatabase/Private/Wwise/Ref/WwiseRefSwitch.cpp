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

#include "Wwise/Ref/WwiseRefSwitch.h"
#include "Wwise/Metadata/WwiseMetadataSwitchGroup.h"

#include "Wwise/Metadata/WwiseMetadataSwitch.h"

const WwiseDBString WwiseRefSwitch::NAME = "Switch"_wwise_db;

const WwiseMetadataSwitch* WwiseRefSwitch::GetSwitch() const
{
	const auto* SwitchGroup = GetSwitchGroup();
	if (!SwitchGroup) [[unlikely]]
	{
		return nullptr;
	}
	const auto& Switches = SwitchGroup->Switches;
	if (Switches.IsValidIndex(SwitchIndex))
	{
		return &Switches[SwitchIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Switch index #%zu", SwitchIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefSwitch::SwitchId() const
{
	const auto* Switch = GetSwitch();
	if (!Switch) [[unlikely]]
	{
		return 0;
	}
	return Switch->Id;
}

WwiseDBGuid WwiseRefSwitch::SwitchGuid() const
{
	const auto* Switch = GetSwitch();
	if (!Switch) [[unlikely]]
	{
		return {};
	}
	return Switch->GUID;
}

WwiseDBString WwiseRefSwitch::SwitchName() const
{
	const auto* Switch = GetSwitch();
	if (!Switch) [[unlikely]]
	{
		return {};
	}
	return Switch->Name;
}

WwiseDBString WwiseRefSwitch::SwitchObjectPath() const
{
	const auto* Switch = GetSwitch();
	if (!Switch) [[unlikely]]
	{
		return {};
	}
	return Switch->ObjectPath;
}

WwiseDBShortId WwiseRefSwitch::Hash() const
{
	auto Result = WwiseRefSwitchGroup::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(SwitchIndex));
	return Result;
}
