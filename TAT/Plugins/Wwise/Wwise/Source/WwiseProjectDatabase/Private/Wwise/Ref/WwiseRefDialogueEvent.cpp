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

#include "Wwise/Ref/WwiseRefDialogueEvent.h"

#include "Wwise/Ref/WwiseRefCollections.h"
#include "Wwise/Ref/WwiseRefDialogueArgument.h"
#include "Wwise/Metadata/WwiseMetadataDialogue.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

#include <inttypes.h>


const WwiseDBString WwiseRefDialogueEvent::NAME = "DialogueEvent"_wwise_db;

const WwiseMetadataDialogueEvent* WwiseRefDialogueEvent::GetDialogueEvent() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& DialogueEvents = SoundBank->DialogueEvents;
	if (DialogueEvents.IsValidIndex(DialogueEventIndex))
	{
		return &DialogueEvents[DialogueEventIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Dialogue Event index #%zu", DialogueEventIndex);
		return nullptr;
	}
}

WwiseDialogueArgumentIdsMap WwiseRefDialogueEvent::GetDialogueArguments(const WwiseDialogueArgumentGlobalIdsMap& GlobalMap) const
{
	const auto* DialogueEvent = GetDialogueEvent();
	if (!DialogueEvent) [[unlikely]]
	{
		return {};
	}
	const auto& Arguments = DialogueEvent->Arguments;
	WwiseDialogueArgumentIdsMap Result;
	Result.Empty(Arguments.Size());
	for (const auto& Elem : Arguments)
	{
		const auto* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
		else
		{
			WWISE_DB_LOG(Error, "Could not get Dialogue Argument ID %" PRIu32, Elem.Id);
		}
	}

	return Result;
}

WwiseDBShortId WwiseRefDialogueEvent::DialogueEventId() const
{
	const auto* DialogueEvent = GetDialogueEvent();
	if (!DialogueEvent) [[unlikely]]
	{
		return 0;
	}
	return DialogueEvent->Id;
}

WwiseDBGuid WwiseRefDialogueEvent::DialogueEventGuid() const
{
	const auto* DialogueEvent = GetDialogueEvent();
	if (!DialogueEvent) [[unlikely]]
	{
		return {};
	}
	return DialogueEvent->GUID;
}

const WwiseDBString* WwiseRefDialogueEvent::DialogueEventName() const
{
	const auto* DialogueEvent = GetDialogueEvent();
	if (!DialogueEvent) [[unlikely]]
	{
		return &emptyString;
	}
	return &DialogueEvent->Name;
}

const WwiseDBString* WwiseRefDialogueEvent::DialogueEventObjectPath() const
{
	const auto* DialogueEvent = GetDialogueEvent();
	if (!DialogueEvent) [[unlikely]]
	{
		return &emptyString;
	}
	return &DialogueEvent->ObjectPath;
}

WwiseDBShortId WwiseRefDialogueEvent::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(DialogueEventIndex));
	return Result;
}
