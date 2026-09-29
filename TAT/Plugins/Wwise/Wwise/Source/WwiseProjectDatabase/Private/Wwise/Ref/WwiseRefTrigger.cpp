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

#include "Wwise/Ref/WwiseRefTrigger.h"

#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Metadata/WwiseMetadataTrigger.h"

const WwiseDBString WwiseRefTrigger::NAME = "Trigger"_wwise_db;

const WwiseMetadataTrigger* WwiseRefTrigger::GetTrigger() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& Triggers = SoundBank->Triggers;
	if (Triggers.IsValidIndex(TriggerIndex))
	{
		return &Triggers[TriggerIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Trigger index #%zu", TriggerIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefTrigger::TriggerId() const
{
	const auto* Trigger = GetTrigger();
	if (!Trigger) [[unlikely]]
	{
		return 0;
	}
	return Trigger->Id;
}

WwiseDBGuid WwiseRefTrigger::TriggerGuid() const
{
	const auto* Trigger = GetTrigger();
	if (!Trigger) [[unlikely]]
	{
		return {};
	}
	return Trigger->GUID;
}

const WwiseDBString* WwiseRefTrigger::TriggerName() const
{
	const auto* Trigger = GetTrigger();
	if (!Trigger) [[unlikely]]
	{
		return &emptyString;
	}
	return &Trigger->Name;
}

const WwiseDBString* WwiseRefTrigger::TriggerObjectPath() const
{
	const auto* Trigger = GetTrigger();
	if (!Trigger) [[unlikely]]
	{
		return &emptyString;
	}
	return &Trigger->ObjectPath;
}

WwiseDBShortId WwiseRefTrigger::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(TriggerIndex));
	return Result;
}
