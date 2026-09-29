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
#include "Wwise/Packaging/WwiseAssetLibraryInfo.h"

class UWwiseAssetLibrary;

/**
 * The pre-cooker takes all the asset libraries from the Settings, and processes them globally.
 */
class WWISEPACKAGINGEDITOR_API FWwiseAssetLibraryPreCooker
{
public:
	FWwiseAssetLibraryPreCooker(FWwiseProjectDatabase& ProjectDatabase);
	virtual ~FWwiseAssetLibraryPreCooker();

	using FAssetLibrarySoftPtr = TSoftObjectPtr<UWwiseAssetLibrary>;
	using FAssetLibraryArray = TArray<FAssetLibrarySoftPtr>;

	using FAssetLibraryInfoPtr = TSharedPtr<FWwiseAssetLibraryInfo>;
	using FAssetLibraryInfoMap = TMap<UObject*, FAssetLibraryInfoPtr>;

	virtual FAssetLibraryInfoMap Process(const FAssetLibraryArray& AssetLibraryArray);

protected:
	FWwiseProjectDatabase& ProjectDatabase;
};
