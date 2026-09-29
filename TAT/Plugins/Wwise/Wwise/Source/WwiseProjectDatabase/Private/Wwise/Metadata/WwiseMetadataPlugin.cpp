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

#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataPluginReference::WwiseMetadataPluginReference(WwiseMetadataLoader& Loader) :
	Id(Loader.GetWwiseShortId(this, "Id"_wwise_db))
{
	Loader.LogParsed("PluginReference"_wwise_db, Id);
}

WwiseMetadataPluginAttributes::WwiseMetadataPluginAttributes(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader),
	LibName(Loader.GetString(this, "LibName"_wwise_db)),
	LibId(Loader.GetWwiseShortId(this, "LibId"_wwise_db))
{
	Loader.LogParsed("PluginAttributes"_wwise_db, Id, Name);
}

WwiseMetadataPlugin::WwiseMetadataPlugin(WwiseMetadataLoader& Loader) :
	WwiseMetadataPluginAttributes(Loader),
	MediaRefs(Loader.GetArray<WwiseMetadataMediaReference>(this, "MediaRefs"_wwise_db)),
	PluginRefs(Loader.GetObjectPtr<WwiseMetadataPluginReferenceGroup>(this, "PluginRefs"_wwise_db))
{
	Loader.LogParsed("Plugin"_wwise_db, Id, Name);
}
