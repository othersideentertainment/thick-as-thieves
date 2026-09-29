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

#include "Wwise/Metadata/WwiseMetadataSwitchValue.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataSwitchValueAttributes::WwiseMetadataSwitchValueAttributes()
{
}

WwiseMetadataSwitchValueAttributes::WwiseMetadataSwitchValueAttributes(WwiseMetadataLoader& Loader) :
	GroupType(GroupTypeFromString(Loader.GetString(this, "GroupType"_wwise_db))),
	GroupId(Loader.GetWwiseShortId(this, "GroupId"_wwise_db)),
	Id(Loader.GetWwiseShortId(this, "Id"_wwise_db)),
	GUID(Loader.GetGuid(this, "GUID"_wwise_db)),
	Color(Loader.GetInt(this, "Color"_wwise_db, WwiseRequiredMetadata::Optional)),
	bDefault(Loader.GetBool(this, "Default"_wwise_db, WwiseRequiredMetadata::Optional))
{
	Loader.LogParsed("SwitchValueAttributes"_wwise_db);
}

WwiseMetadataSwitchValueGroupType WwiseMetadataSwitchValueAttributes::GroupTypeFromString(const WwiseDBString& TypeString)
{
	if (TypeString == "Switch"_wwise_db)
	{
		return WwiseMetadataSwitchValueGroupType::Switch;
	}
	else if (TypeString == "State"_wwise_db)
	{
		return WwiseMetadataSwitchValueGroupType::State;
	}
	WWISE_DB_LOG(Warning, "Wwise/Metadata/WwiseMetadataSwitchValueAttributes: Unknown GroupType: %s", *TypeString);
	return WwiseMetadataSwitchValueGroupType::Unknown;
}

WwiseMetadataSwitchValue::WwiseMetadataSwitchValue()
{
}

WwiseMetadataSwitchValue::WwiseMetadataSwitchValue(WwiseMetadataLoader& Loader) :
	WwiseMetadataSwitchValueAttributes(Loader)
{
	Loader.LogParsed("SwitchValue"_wwise_db);
}
