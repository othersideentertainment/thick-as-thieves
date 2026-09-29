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

#include "Wwise/Metadata/WwiseMetadataLoadable.h"

void WwiseMetadataLoadable::AddRequestedValue(const WwiseDBString& Type, const WwiseDBString& Value)
{
	bool IsAlreadySet = false;
	RequestedValues.Add(Value, &IsAlreadySet);
	if (IsAlreadySet) [[unlikely]]
	{
		WWISE_DB_LOG(Fatal, "Trying to load the same %s field twice: %s", *Type, *Value);
	}
}

void WwiseMetadataLoadable::CheckRequestedValues(WwiseDBJsonObject& JsonObject) const
{
	WwiseDBArray<WwiseDBString> Keys;
	JsonObject.GetKeys(Keys);
	auto Diff = WwiseDBSet<WwiseDBString>(Keys.Array).Difference(RequestedValues);
	for (const auto& Key : Diff)
	{
		WWISE_DB_LOG(Warning, "Unknown Json field: %s", *Key);
	}
}

void WwiseMetadataLoadable::IncLoadedSize(size_t Size)
{
	LoadedSize += Size;
}

void WwiseMetadataLoadable::DecLoadedSize(size_t Size)
{
	LoadedSize -= Size;
}

void WwiseMetadataLoadable::UnloadLoadedSize()
{
	LoadedSize = 0;
}
