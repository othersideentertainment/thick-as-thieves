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
#include "Wwise/Packaging/WwiseAssetLibraryFilter.h"
#include "Wwise/Packaging/WwisePackagingTypeActions.h"

#include "WwiseAssetLibraryTextFilter.generated.h"

UENUM()
enum class EWwiseAssetLibraryTextFilterType : uint8
{
	/** The Name of the Wwise Object*/
	Name UMETA(DisplayName = "Name"),
	/** The Path of the Wwise Unreal Asset*/
	SystemPath UMETA(DisplayName = "System Path"),
	/** The Path of the Wwise Object in Wwise*/
	PathInWwise UMETA(DisplayName = "Path In Wwise")
};

UCLASS(BlueprintType, DisplayName="Filter Assets by Text", Meta = (Category = "Wwise|AssetLibrary|Filter"))
class WWISEPACKAGINGEDITOR_API UWwiseAssetLibraryTextFilter : public UWwiseAssetLibraryFilter
{
	GENERATED_BODY()

public:
	// Filter according to one or multiple global searches, using * for any number of characters, each filter separated by a space.
	// For Regex, only one filter is permitted in standard Unreal regex notation.
	// Examples without Regex:
	//		Foo* filters everything starting with Foo
	//		*Foo filters everything ending with Foo
	//		*Foo* filters everything containing Foo
	// Examples with Regex:
	//		^(Foo) filters everything starting with Foo
	//		(Foo)\b filters everything ending with Foo
	//		\w(Foo) filters everything containing Foo
	UPROPERTY(EditAnywhere, Category = "Default")
	FString Filter{ TEXT("*") };

	/** The type of text by which to filter. */
	UPROPERTY(EditAnywhere, Category = "Default")
	EWwiseAssetLibraryTextFilterType FilterType{EWwiseAssetLibraryTextFilterType::Name};

	/** When selected, make this filter case sensitive. */
	UPROPERTY(EditAnywhere, Category = "Default")
	bool bCaseSensitive{false};

	/** When selected, make this filter an exclusion filter. */
	UPROPERTY(EditAnywhere, Category = "Default")
	bool bIsExclusionFilter{false};

	/** Use Regular Expressions in the filter. */
	UPROPERTY(EditAnywhere, Category = "Default")
	bool bUseRegex{false};

	/** Include SoundBanks that match the filter criteria. */
	UPROPERTY(EditAnywhere, Category = "Default", DisplayName = "Filter SoundBanks")
	bool bFilterSoundBanks{true};

	/** Include Media that match the filter criteria. */
	UPROPERTY(EditAnywhere, Category = "Default")
	bool bFilterMedia{true};

	bool PatternMatch(const WwiseMetadataLoadable* Metadata, const WwiseRefType AssetType) const;
	bool PatternMatch(const FString& InputText) const;
	static bool RegexSearch(const FString& Pattern, const FString& InputText);
	static bool GlobSearch(const FString& Pattern, const FString& InputText, bool bCaseSensitive, int32 PatternIndex = 0, int32 InputIndex = 0);
	static FString GetInputText(const WwiseMetadataLoadable* Metadata, const EWwiseAssetLibraryTextFilterType FilterType, const WwiseRefType AssetType);

	TSet<FString> GlobFilters;
	
public:
	UWwiseAssetLibraryTextFilter() {}

	virtual void PreFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo) override;
	
	virtual bool IsAssetAvailable(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const override;

	virtual void PostFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo) override;

	void Reset(const FString& NewFilter = {},
		EWwiseAssetLibraryTextFilterType NewFilterType = EWwiseAssetLibraryTextFilterType::Name,
		bool NewIsExclusionFilter = false,
		bool NewUseRegex = false);
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_WwiseAssetLibraryTextFilter : public FAssetTypeActions_Base_WwiseAssetLibraryFilter
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};
