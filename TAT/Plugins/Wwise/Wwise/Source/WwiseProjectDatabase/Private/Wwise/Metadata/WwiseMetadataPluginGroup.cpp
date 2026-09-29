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

#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataPluginReferenceGroup::WwiseMetadataPluginReferenceGroup(WwiseMetadataLoader& Loader) :
	Custom(Loader.GetArray<WwiseMetadataPluginReference>(this, "Custom"_wwise_db)),
	ShareSets(Loader.GetArray<WwiseMetadataPluginReference>(this, "ShareSets"_wwise_db)),
	AudioDevices(Loader.GetArray<WwiseMetadataPluginReference>(this, "AudioDevices"_wwise_db))
{
	Loader.LogParsed("PluginReferenceGroup"_wwise_db);
}

WwiseMetadataPluginGroup::WwiseMetadataPluginGroup(WwiseMetadataLoader& Loader) :
	Custom(Loader.GetArray<WwiseMetadataPlugin>(this, "Custom"_wwise_db)),
	ShareSets(Loader.GetArray<WwiseMetadataPlugin>(this, "ShareSets"_wwise_db)),
	AudioDevices(Loader.GetArray<WwiseMetadataPlugin>(this, "AudioDevices"_wwise_db))
{
	Loader.LogParsed("PluginGroup"_wwise_db);
}
