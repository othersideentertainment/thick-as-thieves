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

#include "Wwise/Metadata/WwiseMetadataBasicReference.h"
#include "Wwise/Metadata/WwiseMetadataMedia.h"

struct WwiseMetadataPluginReference : public WwiseMetadataLoadable
{
	WwiseDBShortId Id;

	WwiseMetadataPluginReference(WwiseMetadataLoader& Loader);
};

inline WwiseDBShortId GetTypeHash(const WwiseMetadataPluginReference& Plugin)
{
	return GetTypeHash(Plugin.Id);
}
inline bool operator ==(const WwiseMetadataPluginReference& Lhs, const WwiseMetadataPluginReference& Rhs)
{
	return Lhs.Id == Rhs.Id;
}
inline bool operator <(const WwiseMetadataPluginReference& Lhs, const WwiseMetadataPluginReference& Rhs)
{
	return Lhs.Id < Rhs.Id;
}

struct WwiseMetadataPluginAttributes : public WwiseMetadataBasicReference
{
	WwiseDBString LibName;
	WwiseDBShortId LibId;

	WwiseMetadataPluginAttributes(WwiseMetadataLoader& Loader);
};

struct WwiseMetadataPlugin : public WwiseMetadataPluginAttributes
{
	WwiseDBArray<WwiseMetadataMediaReference> MediaRefs;
	WwiseMetadataPluginReferenceGroup* PluginRefs;

	WwiseMetadataPlugin(WwiseMetadataLoader& Loader);
};
