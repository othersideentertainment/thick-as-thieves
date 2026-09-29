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

#include "WwiseDatabaseIdentifiers.h"
#include "AdapterTypes/WwiseDataTypesAdapter.h"
#include "AdapterTypes/WwiseWrapperTypes.h"

struct WwiseDBPlatformId
{

	WwiseDBPlatformId() :
	PlatformGuid(),
	PlatformName()
	{}

	WwiseDBPlatformId(const WwiseDBGuid& InPlatformGuid, const WwiseDBString& InPlatformName) :
		PlatformGuid(InPlatformGuid),
		PlatformName(InPlatformName)
	{}

	WwiseDBPlatformId(const WwiseDBGuid& InPlatformGuid, const WwiseDBString& InPlatformName, const WwiseDBString& InGeneratedSoundBanksPath) :
	PlatformGuid(InPlatformGuid),
	PlatformName(InPlatformName),
	PathRelativeToGeneratedSoundBanks(InGeneratedSoundBanksPath)
	{}
	
	WwiseDBGuid PlatformGuid;

	WwiseDBString PlatformName;

	WwiseDBString PathRelativeToGeneratedSoundBanks;

	WwiseDBString ExternalSourceRootPath;
};

struct WwiseDBSharedPlatformId
{

	WwiseDBSharedPtr<WwiseDBPlatformId> Platform;

	WwiseDBSharedPlatformId() :
		Platform(new WwiseDBPlatformId)
	{}

	WwiseDBSharedPlatformId(const WwiseDBGuid& InPlatformGuid, const WwiseDBString& InPlatformName) :
		Platform(new WwiseDBPlatformId(InPlatformGuid, InPlatformName))
	{}

	WwiseDBSharedPlatformId(const WwiseDBGuid& InPlatformGuid, const WwiseDBString& InPlatformName, const WwiseDBString& InRelativePath) :
	Platform(new WwiseDBPlatformId(InPlatformGuid, InPlatformName, InRelativePath))
	{}

	const WwiseDBGuid& GetPlatformGuid() const
	{
		return Platform->PlatformGuid;
	}

	const WwiseDBString& GetPlatformName() const
	{
		return Platform->PlatformName;
	}

	bool operator<(const WwiseDBSharedPlatformId& Rhs) const
	{
		return Platform->PlatformGuid < Rhs.Platform->PlatformGuid;
	}
	
	bool operator==(const WwiseDBSharedPlatformId& Rhs) const
	{
		return Platform->PlatformGuid == Rhs.Platform->PlatformGuid;
	}

	bool operator!=(const WwiseDBSharedPlatformId& Rhs) const
	{
		return Platform->PlatformGuid != Rhs.Platform->PlatformGuid;
	}
};

inline WwiseDBShortId GetTypeHash(const WwiseDBSharedPlatformId& Id)
{
	return GetTypeHash(Id.Platform->PlatformGuid);
}
