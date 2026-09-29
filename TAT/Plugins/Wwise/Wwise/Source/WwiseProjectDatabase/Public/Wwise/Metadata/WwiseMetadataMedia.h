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

#include "Wwise/Metadata/WwiseMetadataLoadable.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/WwiseDatabaseIdentifiers.h"

struct WwiseMetadataMediaReference : public WwiseMetadataLoadable
{
	WwiseDBShortId Id;

	WwiseMetadataMediaReference(WwiseMetadataLoader& Loader);
};

inline WwiseDBShortId GetTypeHash(const WwiseMetadataMediaReference& Media)
{
	return GetTypeHash(Media.Id);
}
inline bool operator ==(const WwiseMetadataMediaReference& Lhs, const WwiseMetadataMediaReference& Rhs)
{
	return Lhs.Id == Rhs.Id;
}
inline bool operator <(const WwiseMetadataMediaReference& Lhs, const WwiseMetadataMediaReference& Rhs)
{
	return Lhs.Id < Rhs.Id;
}

enum class WwiseMetadataMediaLocation : WwiseDBShortId
{
	Memory,
	Loose,
	OtherBank,

	Unknown = (WwiseDBShortId)-1
};

struct WwiseMetadataMediaAttributes : public WwiseMetadataMediaReference
{
	WwiseDBString Language;
	bool bStreaming;
	WwiseMetadataMediaLocation Location;
	bool bUsingReferenceLanguage;
	WwiseDBShortId Align;
	bool bDeviceMemory;

	WwiseMetadataMediaAttributes(WwiseMetadataLoader& Loader);

private:
	static WwiseMetadataMediaLocation LocationFromString(const WwiseDBString& LocationString);
};

struct WwiseMetadataMedia : public WwiseMetadataMediaAttributes
{
	WwiseDBString ShortName;
	WwiseDBString Path;
	WwiseDBString CachePath;
	WwiseDBShortId PrefetchSize;

	WwiseMetadataMedia(WwiseMetadataLoader& Loader);
};
