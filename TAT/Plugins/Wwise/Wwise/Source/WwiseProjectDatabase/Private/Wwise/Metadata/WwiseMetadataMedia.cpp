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

#include "Wwise/Metadata/WwiseMetadataMedia.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataMediaReference::WwiseMetadataMediaReference(WwiseMetadataLoader& Loader) :
	Id(Loader.GetWwiseShortId(this, "Id"_wwise_db))
{
	Loader.LogParsed("MediaReference"_wwise_db, Id);
}

WwiseMetadataMediaAttributes::WwiseMetadataMediaAttributes(WwiseMetadataLoader& Loader) :
	WwiseMetadataMediaReference(Loader),
	Language(Loader.GetString(this, "Language"_wwise_db)),
	bStreaming(Loader.GetBool(this, "Streaming"_wwise_db)),
	Location(LocationFromString(Loader.GetString(this, "Location"_wwise_db))),
	bUsingReferenceLanguage(Loader.GetBool(this, "UsingReferenceLanguage"_wwise_db, WwiseRequiredMetadata::Optional)),
	Align(Loader.GetWwiseShortId(this, "Align"_wwise_db, WwiseRequiredMetadata::Optional)),
	bDeviceMemory(Loader.GetBool(this, "DeviceMemory"_wwise_db, WwiseRequiredMetadata::Optional))
{
	Loader.LogParsed("MediaAttributes"_wwise_db, Id);
}

WwiseMetadataMediaLocation WwiseMetadataMediaAttributes::LocationFromString(const WwiseDBString& LocationString)
{
	if (LocationString == "Memory"_wwise_db)
	{
		return WwiseMetadataMediaLocation::Memory;
	}
	else if (LocationString == "Loose"_wwise_db)
	{
		return WwiseMetadataMediaLocation::Loose;
	}
	else if (LocationString == "OtherBank"_wwise_db)
	{
		return WwiseMetadataMediaLocation::OtherBank;
	}
	else
	{
		WWISE_DB_LOG(Warning, "WwiseMetadataMediaAttributes: Unknown Location: %s", *LocationString);
		return WwiseMetadataMediaLocation::Unknown;
	}
}

WwiseMetadataMedia::WwiseMetadataMedia(WwiseMetadataLoader& Loader) :
	WwiseMetadataMediaAttributes(Loader),
	ShortName(Loader.GetString(this, "ShortName"_wwise_db)),
	Path(Loader.GetString(this, "Path"_wwise_db, WwiseRequiredMetadata::Optional)),
	CachePath(Loader.GetString(this, "CachePath"_wwise_db, WwiseRequiredMetadata::Optional)),
	PrefetchSize(Loader.GetWwiseShortId(this, "PrefetchSize"_wwise_db, WwiseRequiredMetadata::Optional))
{
	if (Path.IsEmpty() && Location == WwiseMetadataMediaLocation::Loose)
	{
		Loader.Fail("!Path+Location=Loose"_wwise_db);
	}
	else if (Path.IsEmpty() && Location == WwiseMetadataMediaLocation::Memory && bStreaming)
	{
		Loader.Fail("!Path+Streaming"_wwise_db);
	}
	else if (!Path.IsEmpty() && Location == WwiseMetadataMediaLocation::Memory && !bStreaming)
	{
		Loader.Fail("Path+Memory"_wwise_db);
	}
	Loader.LogParsed("Media"_wwise_db, Id);
}
