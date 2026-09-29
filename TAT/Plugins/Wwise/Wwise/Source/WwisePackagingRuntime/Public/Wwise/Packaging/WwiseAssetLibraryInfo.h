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
#include "WwiseAssetLibraryInfo.generated.h"

class UWwiseSharedAssetLibraryFilter;
class UWwiseAssetLibraryFilter;

/**
 * List of possible asset library references, as defined in the filtered assets
 */
UENUM()
enum class EWwiseAssetLibraryRefType
{
	Unknown,
	InitBank,
	SoundBank,
	Media,
	ExternalSource
};

/**
 * One asset library reference towards a Wwise asset. This is the equivalent of a short-hand Wwise Project Database's AnyRef.
 */
USTRUCT()
struct WWISEPACKAGINGRUNTIME_API FWwiseAssetLibraryRef
{
	GENERATED_BODY()

public:
	FWwiseAssetLibraryRef() {}
	virtual ~FWwiseAssetLibraryRef() {}
	
	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	EWwiseAssetLibraryRefType Type { EWwiseAssetLibraryRefType::Unknown };

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	FGuid Guid;

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	int Id { 0 };

	UPROPERTY(VisibleInstanceOnly, Category = "Default")
	FName Name;

	UPROPERTY(VisibleInstanceOnly, Category = "Advanced")
	int LanguageId { 0 };

	UPROPERTY(VisibleInstanceOnly, Category = "Advanced", meta=(EditCondition = "Type == EWwiseAssetLibraryRefType::Media", EditConditionHides))
	int SoundBankId { 0 };
};

/**
 * Information used to define the contents included in the Asset Library, to be cooked in the FWwiseAssetLibraryCookedData.
 */
USTRUCT(BlueprintType, Meta = (Category = "Wwise|AssetLibrary|Info", DisplayName = "Asset Library Info"))
struct WWISEPACKAGINGRUNTIME_API FWwiseAssetLibraryInfo
{
	GENERATED_BODY()

public:
	FWwiseAssetLibraryInfo() {}
	virtual ~FWwiseAssetLibraryInfo() {}
	
#if WITH_EDITORONLY_DATA
	/**
	 * Filters to use to select assets to include in this Asset Library.
	 */
	UPROPERTY(EditAnywhere, Category = "Default")
	TArray<TObjectPtr<UWwiseAssetLibraryFilter>> Filters;

	/**
	 * Shared filters to use to select assets to include in this Asset Library, and that can be applied to multiple Asset Libraries.
	 */
	UPROPERTY(EditAnywhere, Category = "Default", meta=(EditConditionHides=true))
	TArray<TSoftObjectPtr<UWwiseSharedAssetLibraryFilter>> SharedFilters;

	/**
	 * Filtered assets, as determined by the current filter criteria.
	 *
	 * This does not consider other asset libraries, as prioritized in the Wwise Packaging library list.
	 */
	TArray<FWwiseAssetLibraryRef> FilteredAssets;

	void PreloadFilters();
	TArray<TObjectPtr<UWwiseAssetLibraryFilter>> GetFilters(
		TSet<const UObject*>* ProcessedFilters = nullptr) const;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWwiseSharedAssetLibraryFilter>> PreloadedFilters;
#endif
};
