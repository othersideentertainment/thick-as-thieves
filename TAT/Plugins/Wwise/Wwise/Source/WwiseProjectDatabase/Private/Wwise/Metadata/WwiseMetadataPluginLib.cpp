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

#include "Wwise/Metadata/WwiseMetadataPluginLib.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataPluginLibAttributes::WwiseMetadataPluginLibAttributes(WwiseMetadataLoader& Loader) :
	LibName(Loader.GetString(this, "LibName"_wwise_db)),
	LibId(Loader.GetWwiseShortId(this, "LibId"_wwise_db)),
	Type(TypeFromString(Loader.GetString(this, "Type"_wwise_db))),
	DLL(Loader.GetString(this, "DLL"_wwise_db, WwiseRequiredMetadata::Optional)),
	StaticLib(Loader.GetString(this, "StaticLib"_wwise_db, WwiseRequiredMetadata::Optional))
{
	Loader.LogParsed("PluginLibAttributes"_wwise_db, LibId, LibName);
}

WwiseMetadataPluginLibType WwiseMetadataPluginLibAttributes::TypeFromString(const WwiseDBString& TypeString)
{
	if (TypeString == "Effect"_wwise_db)
	{
		return WwiseMetadataPluginLibType::Effect;
	}
	else if (TypeString == "Source"_wwise_db)
	{
		return WwiseMetadataPluginLibType::Source;
	}
	else if (TypeString == "AudioDevice"_wwise_db)
	{
		return WwiseMetadataPluginLibType::AudioDevice;
	}
	else if (TypeString == "Metadata"_wwise_db)
	{
		return WwiseMetadataPluginLibType::Metadata;
	}
	else
	{
		WWISE_DB_LOG(Warning, "WwiseMetadataPluginLibAttributes: Unknown Type: %s", *TypeString);
		return WwiseMetadataPluginLibType::Unknown;
	}
}

WwiseMetadataPluginLib::WwiseMetadataPluginLib(WwiseMetadataLoader& Loader) :
	WwiseMetadataPluginLibAttributes(Loader)
{
	Loader.LogParsed("PluginLib"_wwise_db, LibId, LibName);
}
