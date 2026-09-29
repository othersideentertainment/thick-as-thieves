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

#include "Wwise/WwiseProjectDatabase.h"

class FWwiseResourceLoader;
class FWwiseProjectDatabase;
using FSharedWwiseDataStructure = TSharedRef<WwiseDataStructure, ESPMode::ThreadSafe>;

class WWISEPROJECTDATABASE_API FWwiseProjectDatabaseImpl : public FWwiseProjectDatabase
{
public:
	FWwiseProjectDatabaseImpl();
	~FWwiseProjectDatabaseImpl() override;

	FWwiseResourceLoaderPtr ResourceLoaderOverride;

	void UpdateDataStructure(
		const WwiseDBGuid* InBasePlatformGuid = &BasePlatformGuid) override;

	void PrepareProjectDatabaseForPlatform(FWwiseResourceLoaderPtr&& InResourceLoader) override;
	FWwiseResourceLoaderPtr GetResourceLoader() override;
	const FWwiseResourceLoaderPtr GetResourceLoader() const override;

protected:
	bool bShouldBroadcast = true;
	FSharedWwiseDataStructure LockedDataStructure;

	/**
	 * @brief Location where the Wwise Generated SoundBanks is found on disk relative to the project
	 */
	const WwiseDBString* GeneratedSoundBanksPath;


	FSharedWwiseDataStructure& GetLockedDataStructure() override { return LockedDataStructure; }
	const FSharedWwiseDataStructure& GetLockedDataStructure() const override { return LockedDataStructure; }
};