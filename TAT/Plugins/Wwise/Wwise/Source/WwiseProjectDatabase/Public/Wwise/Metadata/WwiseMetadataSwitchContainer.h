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

#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#include "Wwise/Metadata/WwiseMetadataExternalSource.h"
#include "Wwise/Metadata/WwiseMetadataSwitchValue.h"
#include "Wwise/Metadata/WwiseMetadataLoadable.h"
#include "Wwise/Metadata/WwiseMetadataMedia.h"
#include "Wwise/Metadata/WwiseMetadataPlugin.h"


struct WwiseMetadataSwitchContainer : public WwiseMetadataLoadable
{
	WwiseMetadataSwitchValue SwitchValue;
	WwiseDBArray<WwiseMetadataMediaReference> MediaRefs;
	WwiseDBArray<WwiseMetadataExternalSourceReference> ExternalSourceRefs;
	WwiseMetadataPluginReferenceGroup* PluginRefs;
	WwiseDBArray<WwiseMetadataSwitchContainer> Children;

	WwiseMetadataSwitchContainer(WwiseMetadataLoader& Loader);
	WwiseDBSet<WwiseMetadataMediaReference> GetAllMedia() const;
	WwiseDBSet<WwiseMetadataExternalSourceReference> GetAllExternalSources() const;
	WwiseDBSet<WwiseMetadataPluginReference> GetAllCustomPlugins() const;
	WwiseDBSet<WwiseMetadataPluginReference> GetAllPluginShareSets() const;
	WwiseDBSet<WwiseMetadataPluginReference> GetAllAudioDevices() const;
};
