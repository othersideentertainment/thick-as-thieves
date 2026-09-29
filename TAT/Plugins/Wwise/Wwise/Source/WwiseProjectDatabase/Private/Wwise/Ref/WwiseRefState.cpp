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

#include "Wwise/Ref/WwiseRefState.h"
#include "Wwise/Metadata/WwiseMetadataStateGroup.h"

#include "Wwise/Metadata/WwiseMetadataState.h"

const WwiseDBString WwiseRefState::NAME = "State"_wwise_db;

const WwiseMetadataState* WwiseRefState::GetState() const
{
	const auto* StateGroup = GetStateGroup();
	if (!StateGroup) [[unlikely]]
	{
		return nullptr;
	}
	const auto& States = StateGroup->States;
	if (States.IsValidIndex(StateIndex))
	{
		return &States[StateIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get State index #%zu", StateIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefState::StateId() const
{
	const auto* State = GetState();
	if (!State) [[unlikely]]
	{
		return 0;
	}
	return State->Id;
}

WwiseDBGuid WwiseRefState::StateGuid() const
{
	const auto* State = GetState();
	if (!State) [[unlikely]]
	{
		return {};
	}
	return State->GUID;
}

WwiseDBString WwiseRefState::StateName() const
{
	const auto* State = GetState();
	if (!State) [[unlikely]]
	{
		return {};
	}
	return State->Name;
}

WwiseDBString WwiseRefState::StateObjectPath() const
{
	const auto* State = GetState();
	if (!State) [[unlikely]]
	{
		return {};
	}
	return State->ObjectPath;
}

WwiseDBShortId WwiseRefState::Hash() const
{
	auto Result = WwiseRefStateGroup::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(StateIndex));
	return Result;
}
