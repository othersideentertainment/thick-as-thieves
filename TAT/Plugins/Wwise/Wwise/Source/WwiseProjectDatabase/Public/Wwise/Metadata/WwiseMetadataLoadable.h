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

#include "Wwise/Metadata/WwiseMetadataForwardDeclarations.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"

struct WWISEPROJECTDATABASE_API WwiseMetadataLoadable
{
protected:

	WwiseDBSet<WwiseDBString> RequestedValues;
	size_t LoadedSize;

	inline WwiseMetadataLoadable() :
		RequestedValues(),
		LoadedSize(0)
	{}

	inline ~WwiseMetadataLoadable()
	{
		if (LoadedSize != 0)
		{
			UnloadLoadedSize();
		}
	}

	WwiseMetadataLoadable(WwiseMetadataLoadable&& Other)
	: LoadedSize(Other.LoadedSize)
	{
		Other.LoadedSize = 0;
	}

	WwiseMetadataLoadable(const WwiseMetadataLoadable& other) = default;
	WwiseMetadataLoadable& operator=(const WwiseMetadataLoadable& Other) = default;
	WwiseMetadataLoadable& operator=(const WwiseMetadataLoadable&& Other) = delete;

public:
	void AddRequestedValue(const WwiseDBString& Type, const WwiseDBString& Value);
	void CheckRequestedValues(WwiseDBJsonObject& JsonObject) const;
	void IncLoadedSize(size_t Size);
	void DecLoadedSize(size_t Size);
	void UnloadLoadedSize();
};
