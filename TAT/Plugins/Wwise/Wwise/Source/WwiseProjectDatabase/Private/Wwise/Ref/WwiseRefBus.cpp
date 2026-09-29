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

#include "Wwise/Ref/WwiseRefBus.h"

#include "Wwise/Metadata/WwiseMetadataBus.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

const WwiseDBString WwiseRefBus::NAME = "Bus"_wwise_db;

const WwiseMetadataBus* WwiseRefBus::GetBus() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& Busses = SoundBank->Busses;
	if (Busses.IsValidIndex(BusIndex))
	{
		return &Busses[BusIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Bus index #%zu", BusIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefBus::BusId() const
{
	const auto* Bus = GetBus();
	if (!Bus) [[unlikely]]
	{
		return 0;
	}
	return Bus->Id;
}

WwiseDBGuid WwiseRefBus::BusGuid() const
{
	const auto* Bus = GetBus();
	if (!Bus) [[unlikely]]
	{
		return {};
	}
	return Bus->GUID;
}

const WwiseDBString* WwiseRefBus::BusName() const
{
	const auto* Bus = GetBus();
	if (!Bus) [[unlikely]]
	{
		return &emptyString;
	}
	return &Bus->Name;
}

const WwiseDBString* WwiseRefBus::BusObjectPath() const
{
	const auto* Bus = GetBus();
	if (!Bus) [[unlikely]]
	{
		return &emptyString;
	}
	return &Bus->ObjectPath;
}

WwiseDBShortId WwiseRefBus::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(BusIndex));
	return Result;
}
