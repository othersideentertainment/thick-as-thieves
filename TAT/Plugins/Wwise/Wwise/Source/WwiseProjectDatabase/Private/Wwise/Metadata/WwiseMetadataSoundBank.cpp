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

#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Metadata/WwiseMetadataCollections.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataStateGroup.h"
#include "Wwise/Metadata/WwiseMetadataSwitchGroup.h"

WwiseMetadataSoundBankReference::WwiseMetadataSoundBankReference(WwiseMetadataLoader& Loader) :
	Id(Loader.GetWwiseShortId(this, "Id"_wwise_db)),
	GUID(Loader.GetGuid(this, "GUID"_wwise_db)),
	Language(Loader.GetString(this, "Language"_wwise_db))
{
	Loader.LogParsed("SoundBankReference"_wwise_db, Id);
}

WwiseMetadataSoundBankAttributes::WwiseMetadataSoundBankAttributes(WwiseMetadataLoader& Loader) :
	WwiseMetadataSoundBankReference(Loader),
	Align(Loader.GetWwiseShortId(this, "Align"_wwise_db, WwiseRequiredMetadata::Optional)),
	bDeviceMemory(Loader.GetBool(this, "DeviceMemory"_wwise_db, WwiseRequiredMetadata::Optional)),
	Hash(Loader.GetGuid(this, "Hash"_wwise_db)),
	Type(TypeFromString(Loader.GetString(this, "Type"_wwise_db)))
{
	IncLoadedSize(sizeof(EMetadataSoundBankType));
	Loader.LogParsed("SoundBankAttributes"_wwise_db, Id);
}

EMetadataSoundBankType WwiseMetadataSoundBankAttributes::TypeFromString(const WwiseDBString& TypeString)
{
	if (TypeString == "User"_wwise_db)
	{
		return EMetadataSoundBankType::User;
	}
	else if (TypeString == "Event"_wwise_db)
	{
		return EMetadataSoundBankType::Event;
	}
	else if (TypeString == "Bus"_wwise_db)
	{
		return EMetadataSoundBankType::Bus;
	}
	else
	{
		WWISE_DB_LOG(Warning, "Wwise/Metadata/WwiseMetadataSoundBankAttributes: Unknown Type: %s", *TypeString);
		return EMetadataSoundBankType::Unknown;
	}
}

WwiseMetadataSoundBank::WwiseMetadataSoundBank(WwiseMetadataLoader& Loader) :
	WwiseMetadataSoundBankAttributes(Loader),
	ObjectPath(Loader.GetString(this, "ObjectPath"_wwise_db)),
	Color(Loader.GetInt(this, "Color"_wwise_db, WwiseRequiredMetadata::Optional)),
	ShortName(Loader.GetString(this, "ShortName"_wwise_db)),
	Path(Loader.GetString(this, "Path"_wwise_db)),
	Media(Loader.GetArray<WwiseMetadataMedia>(this, "Media"_wwise_db)),
	Plugins(Loader.GetObjectPtr<WwiseMetadataPluginGroup>(this, "Plugins"_wwise_db)),
	Events(Loader.GetArray<WwiseMetadataEvent>(this, "Events"_wwise_db)),
	DialogueEvents(Loader.GetArray<WwiseMetadataDialogueEvent>(this, "DialogueEvents"_wwise_db)),
	Busses(Loader.GetArray<WwiseMetadataBus>(this, "Busses"_wwise_db)),
	AuxBusses(Loader.GetArray<WwiseMetadataBus>(this, "AuxBusses"_wwise_db)),
	GameParameters(Loader.GetArray<WwiseMetadataGameParameter>(this, "GameParameters"_wwise_db)),
	StateGroups(Loader.GetArray<WwiseMetadataStateGroup>(this, "StateGroups"_wwise_db)),
	SwitchGroups(Loader.GetArray<WwiseMetadataSwitchGroup>(this, "SwitchGroups"_wwise_db)),
	Triggers(Loader.GetArray<WwiseMetadataTrigger>(this, "Triggers"_wwise_db)),
	ExternalSources(Loader.GetArray<WwiseMetadataExternalSource>(this, "ExternalSources"_wwise_db)),
	AcousticTextures(Loader.GetArray<WwiseMetadataAcousticTexture>(this, "AcousticTextures"_wwise_db))
{
	bIsInitBank = ShortName == "Init"_wwise_db;
	Loader.LogParsed("SoundBank"_wwise_db, Id);
}

WwiseDBSet<WwiseMetadataDialogueArgument> WwiseMetadataSoundBank::GetAllDialogueArguments() const
{
	WwiseDBSet<WwiseMetadataDialogueArgument> Result;
	for (const auto& DialogueEvent : DialogueEvents)
	{
		Result.Append(DialogueEvent.Arguments);
	}
	return Result;
}

WwiseDBSet<WwiseMetadataStateWithGroup> WwiseMetadataSoundBank::GetAllStates() const
{
	WwiseDBSet<WwiseMetadataStateWithGroup> Result;
	for (const auto& StateGroup : StateGroups)
	{
		for (const auto& State : StateGroup.States)
		{
			Result.Emplace(WwiseMetadataStateWithGroup(StateGroup, State), nullptr);
		}
	}
	return Result;
}

WwiseDBSet<WwiseMetadataSwitchWithGroup> WwiseMetadataSoundBank::GetAllSwitches() const
{
	WwiseDBSet<WwiseMetadataSwitchWithGroup> Result;
	for (const auto& SwitchGroup : SwitchGroups)
	{
		for (const auto& Switch : SwitchGroup.Switches)
		{
			Result.Emplace(WwiseMetadataSwitchWithGroup(SwitchGroup, Switch), nullptr);
		}
	}
	return Result;
}
