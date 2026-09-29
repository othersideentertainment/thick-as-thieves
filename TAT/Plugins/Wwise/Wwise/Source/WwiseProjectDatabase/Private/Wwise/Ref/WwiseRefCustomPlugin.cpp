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

#include "Wwise/Ref/WwiseRefCustomPlugin.h"

#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Ref/WwiseRefMedia.h"

const WwiseDBString WwiseRefCustomPlugin::NAME = "CustomPlugin"_wwise_db;

const WwiseMetadataPlugin* WwiseRefCustomPlugin::GetPlugin() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank || !SoundBank->Plugins) [[unlikely]]
	{
		return nullptr;
	}

	const auto& Plugins = SoundBank->Plugins->Custom;
	if (Plugins.IsValidIndex(CustomPluginIndex))
	{
		return &Plugins[CustomPluginIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Custom Plugin index #%zu", CustomPluginIndex);
		return nullptr;
	}
}

WwiseMediaIdsMap WwiseRefCustomPlugin::GetPluginMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const
{
	const auto* CustomPlugin = GetPlugin();
	const auto* SoundBank = GetSoundBank();
	if (!CustomPlugin || !SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& Media = CustomPlugin->MediaRefs;

	WwiseMediaIdsMap Result;
	Result.Empty(Media.Size());
	for (const auto& Elem : Media)
	{
		WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);

		const WwiseRefMedia* MediaInGlobalMap = GlobalMap.Find(Id);
		if (MediaInGlobalMap)
		{
			Result.Add(Elem.Id, *MediaInGlobalMap);
		}
	}
	return Result;
}

WwiseDBShortId WwiseRefCustomPlugin::CustomPluginId() const
{
	const auto* CustomPlugin = GetPlugin();
	if (!CustomPlugin) [[unlikely]]
	{
		return 0;
	}
	return CustomPlugin->Id;
}

WwiseDBGuid WwiseRefCustomPlugin::CustomPluginGuid() const
{
	const auto* CustomPlugin = GetPlugin();
	if (!CustomPlugin) [[unlikely]]
	{
		return {};
	}
	return CustomPlugin->GUID;
}

const WwiseDBString* WwiseRefCustomPlugin::CustomPluginName() const
{
	const auto* CustomPlugin = GetPlugin();
	if (!CustomPlugin) [[unlikely]]
	{
		return &emptyString;
	}
	return &CustomPlugin->Name;
}

const WwiseDBString* WwiseRefCustomPlugin::CustomPluginObjectPath() const
{
	const auto* CustomPlugin = GetPlugin();
	if (!CustomPlugin) [[unlikely]]
	{
		return &emptyString;
	}
	return &CustomPlugin->ObjectPath;
}

WwiseDBShortId WwiseRefCustomPlugin::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(CustomPluginIndex));
	return Result;
}
