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
#include "WwiseAssetLibraryInfo.h"
#include "WwiseAssetLibraryFilter.generated.h"

class WwiseAnyRef;
struct FWwiseAssetLibraryFilteringSharedData;

/**
 * Abstract base UObject representing a single AssetLibrary filter.
 *
 * A filter removes elements from the the Asset Library through the IsAssetAvailable operation.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, Meta = (Category = "Wwise|AssetLibrary|Filter"))
class WWISEPACKAGINGRUNTIME_API UWwiseAssetLibraryFilter : public UObject
{
	GENERATED_BODY()

public:
	UWwiseAssetLibraryFilter() {}

	/**
	 * Allows for data preparation before asset filtering begins.
	 * Is called per WwiseAssetLibrary, before iterating over its filters.
	 *
	 * @param Shared Data accessible to filters for quick checks
	 * @param AssetLibraryInfo Asset Library information from the library containing the filter
	 */
	virtual void PreFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo)
	{}
	
	/**
	 * Tells whether the asset is available or not.
	 *
	 * @warning This method can be called on any given thread, concurrently with other filters.
	 *
	 * @param Shared Data accessible to filters for quick checks
	 * @param Asset The asset to check
	 * @return Whether the asset is kept (true), or if it's filtered out (false). 
	 */
	virtual bool IsAssetAvailable(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const
	PURE_VIRTUAL(IsAssetAvailable, return true;)
	
	/**
	 * Allows for data teardown after asset filtering has ended.
	 * Is called per WwiseAssetLibrary, after iterating over its filters
	 *
	 * @param Shared Data accessible to filters for quick checks
	 * @param AssetLibraryInfo Asset Library information from the library containing the filter
	 */
	virtual void PostFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo)
	{}
};
