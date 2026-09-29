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

#include "Wwise/Metadata/WwiseMetadataSoundBanksInfo.h"
#include "Wwise/Metadata/WwiseMetadataRootPaths.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataSoundBanksInfoAttributes::WwiseMetadataSoundBanksInfoAttributes(WwiseMetadataLoader& Loader):
	Platform(Loader.GetString(this, "Platform"_wwise_db)),
	BasePlatform(Loader.GetString(this, "BasePlatform"_wwise_db)),
	SchemaVersion(Loader.GetWwiseShortId(this, "SchemaVersion"_wwise_db)),
	SoundBankVersion(Loader.GetWwiseShortId(this, "SoundBankVersion"_wwise_db))
{
	Loader.LogParsed("SoundBanksInfoAttributes"_wwise_db);
}

WwiseMetadataSoundBanksInfo::WwiseMetadataSoundBanksInfo(WwiseMetadataLoader& Loader) :
	WwiseMetadataSoundBanksInfoAttributes(Loader),
	RootPaths(Loader.GetObjectPtr<WwiseMetadataRootPaths>(this, "RootPaths"_wwise_db)),
	DialogueEvents(Loader.GetArray<WwiseMetadataDialogueEvent>(this, "DialogueEvents"_wwise_db)),
	SoundBanks(Loader.GetArray<WwiseMetadataSoundBank>(this, "SoundBanks"_wwise_db)),
	FileHash(Loader.GetGuid(this, "FileHash"_wwise_db))
{
	Loader.LogParsed("SoundBanksInfo"_wwise_db);
}

WwiseMetadataSoundBanksInfo::~WwiseMetadataSoundBanksInfo()
{
	if (RootPaths)
	{
		delete RootPaths;
		RootPaths = nullptr;
	}
}
