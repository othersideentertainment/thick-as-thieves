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

#include "Wwise/Metadata/WwiseMetadataBus.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"

WwiseMetadataBusReference::WwiseMetadataBusReference(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader)
{
	Loader.LogParsed("BusReference"_wwise_db, Id, Name);
}

WwiseMetadataBus::WwiseMetadataBus(WwiseMetadataLoader& Loader) :
	WwiseMetadataBusReference(Loader),
	PluginRefs(Loader.GetObjectPtr<WwiseMetadataPluginReferenceGroup>(this, "PluginRefs"_wwise_db)),
	AuxBusRefs(Loader.GetArray<WwiseMetadataBusReference>(this, "AuxBusRefs"_wwise_db)),
	MaxAttenuation(Loader.GetFloat(this, "MaxAttenuation"_wwise_db, WwiseRequiredMetadata::Optional))
{
	Loader.LogParsed("Bus"_wwise_db, Id, Name);
}
