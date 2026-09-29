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

#include "Wwise/Metadata/WwiseMetadataAcousticTexture.h"
#include "Wwise/Metadata/WwiseMetadataBus.h"
#include "Wwise/Metadata/WwiseMetadataCollections.h"
#include "Wwise/Metadata/WwiseMetadataDialogue.h"
#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Metadata/WwiseMetadataExternalSource.h"
#include "Wwise/Metadata/WwiseMetadataGameParameter.h"
#include "Wwise/Metadata/WwiseMetadataMedia.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataStateGroup.h"
#include "Wwise/Metadata/WwiseMetadataSwitchGroup.h"
#include "Wwise/Metadata/WwiseMetadataTrigger.h"
#include "Wwise/Ref/WwiseRefCollections.h"

struct WwiseMetadataSoundBankReference : public WwiseMetadataLoadable
{
	WwiseDBShortId Id;
	WwiseDBGuid GUID;
	WwiseDBString Language;

	WwiseMetadataSoundBankReference(WwiseMetadataLoader& Loader);
};

enum class EMetadataSoundBankType : WwiseDBShortId
{
	User = 0,
	Event = 30,
	Bus = 31,
	Unknown = (WwiseDBShortId)-1
};

struct WwiseMetadataSoundBankAttributes : public WwiseMetadataSoundBankReference
{
	WwiseDBShortId Align;
	bool bDeviceMemory;
	WwiseDBGuid Hash;
	EMetadataSoundBankType Type;

	WwiseMetadataSoundBankAttributes(WwiseMetadataLoader& Loader);

private:
	static EMetadataSoundBankType TypeFromString(const WwiseDBString& TypeString);
};

struct WwiseMetadataSoundBank : public WwiseMetadataSoundBankAttributes
{
	WwiseDBString ObjectPath;
	int Color;
	WwiseDBString ShortName;
	WwiseDBString Path;

	WwiseDBArray<WwiseMetadataMedia> Media;
	WwiseMetadataPluginGroup* Plugins;
	WwiseDBArray<WwiseMetadataEvent> Events;
	WwiseDBArray<WwiseMetadataDialogueEvent> DialogueEvents;
	WwiseDBArray<WwiseMetadataBus> Busses;
	WwiseDBArray<WwiseMetadataBus> AuxBusses;
	WwiseDBArray<WwiseMetadataGameParameter> GameParameters;
	WwiseDBArray<WwiseMetadataStateGroup> StateGroups;
	WwiseDBArray<WwiseMetadataSwitchGroup> SwitchGroups;
	WwiseDBArray<WwiseMetadataTrigger> Triggers;
	WwiseDBArray<WwiseMetadataExternalSource> ExternalSources;
	WwiseDBArray<WwiseMetadataAcousticTexture> AcousticTextures;

	WwiseMetadataSoundBank(WwiseMetadataLoader& Loader);
	WwiseDBSet<WwiseMetadataDialogueArgument> GetAllDialogueArguments() const;
	WwiseDBSet<WwiseMetadataStateWithGroup> GetAllStates() const;
	WwiseDBSet<WwiseMetadataSwitchWithGroup> GetAllSwitches() const;
	bool IsInitBank() const
	{
		return bIsInitBank;
	}
	bool ContainsMedia() const
	{
		return Media.ContainsByPredicate([](const WwiseMetadataMedia& MediaToTest)
		{
			return MediaToTest.Location == WwiseMetadataMediaLocation::Memory;
		});
	}

protected:
	bool bIsInitBank;

};
