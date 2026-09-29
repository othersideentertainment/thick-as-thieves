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

#include "Wwise/Packaging/Filters/WwiseAssetLibraryTextFilter.h"

#include "Internationalization/Regex.h"
#include "Wwise/Packaging/WwiseAssetLibraryFilteringSharedData.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_WwiseAssetLibraryTextFilter::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseAssetLibraryTextFilter", "Wwise Asset Library Filter : Text");
}

UClass* FAssetTypeActions_WwiseAssetLibraryTextFilter::GetSupportedClass() const
{
	return UWwiseAssetLibraryTextFilter::StaticClass();
}

#undef LOCTEXT_NAMESPACE 

bool UWwiseAssetLibraryTextFilter::PatternMatch(const WwiseMetadataLoadable* Metadata, const WwiseRefType AssetType) const
{
	return PatternMatch(GetInputText(Metadata, FilterType, AssetType));
}

bool UWwiseAssetLibraryTextFilter::PatternMatch(const FString& InputText) const
{
	if(bUseRegex)
	{
		return RegexSearch(Filter, InputText);
	}
	else
	{
		for (const auto& CurFilter: GlobFilters)
		{
			const bool bResult = GlobSearch(CurFilter, InputText, bCaseSensitive);
			if (bResult)
			{
				return true;
			}
		}
		return false;
	}
}

bool UWwiseAssetLibraryTextFilter::RegexSearch(const FString& Pattern, const FString& InputText)
{
	//Empty filter is invalid
	if(Pattern.IsEmpty())
	{
		return false;
	}
	FRegexMatcher Matcher(FRegexPattern(Pattern), InputText);
	return Matcher.FindNext();
}

bool UWwiseAssetLibraryTextFilter::GlobSearch(const FString& Pattern, const FString& InputText, bool bCaseSensitive, int32 PatternIndex, int32 InputIndex)
{
	while(PatternIndex < Pattern.Len())
	{
		const auto currentChar = Pattern[PatternIndex];
		if(currentChar == '?')
		{
			if(InputIndex >= InputText.Len())
				return false;
			++PatternIndex;
			++InputIndex;
		}
		else if(currentChar == '*')
		{
			if(GlobSearch(Pattern, InputText, bCaseSensitive, PatternIndex + 1, InputIndex) ||
				(InputIndex < InputText.Len() && GlobSearch(Pattern, InputText, bCaseSensitive, PatternIndex, InputIndex+1)))
				return true;
			return false;
		}
		else
		{
			if(InputIndex >= InputText.Len() || (bCaseSensitive ? currentChar != InputText[InputIndex] : FChar::ToLower(currentChar) != FChar::ToLower(InputText[InputIndex])))
				return false;
			++PatternIndex;
			++InputIndex;
		}
	}
	return InputIndex == InputText.Len();
}

FString UWwiseAssetLibraryTextFilter::GetInputText(const WwiseMetadataLoadable* Metadata,
	const EWwiseAssetLibraryTextFilterType FilterType, const WwiseRefType AssetType)
{
	FString InputText;
	switch (FilterType)
	{
	case EWwiseAssetLibraryTextFilterType::Name:
		if(AssetType == WwiseRefType::Media)
		{
			InputText = *(static_cast<const WwiseMetadataMedia*>(Metadata)->ShortName);
		}
		else if(AssetType == WwiseRefType::SoundBank)
		{
			InputText = *(static_cast<const WwiseMetadataSoundBank*>(Metadata)->ShortName);
		}
		break;
	case EWwiseAssetLibraryTextFilterType::SystemPath:
		if(AssetType == WwiseRefType::Media)
		{
			InputText = *(static_cast<const WwiseMetadataMedia*>(Metadata)->Path);
		}
		else if(AssetType == WwiseRefType::SoundBank)
		{
			InputText = *(static_cast<const WwiseMetadataSoundBank*>(Metadata)->Path);
		}
		break;
	case EWwiseAssetLibraryTextFilterType::PathInWwise:
		if(AssetType == WwiseRefType::SoundBank)
		{
			InputText = *(static_cast<const WwiseMetadataSoundBank*>(Metadata)->ObjectPath);
		}
		break;

	}
	return InputText;
}

void UWwiseAssetLibraryTextFilter::PreFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	if (!bUseRegex)
	{
		TArray<FString> GlobFiltersArray;
		Filter.ParseIntoArrayWS(GlobFiltersArray);
		GlobFilters = TSet(GlobFiltersArray);
	}
}

bool UWwiseAssetLibraryTextFilter::IsAssetAvailable(const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const
{
	const WwiseMetadataLoadable* Metadata;
	const WwiseRefType AssetType = Asset.GetType();
	FString AssetName;
	if(AssetType == WwiseRefType::Media)
	{
		Metadata = Asset.GetMedia();
		AssetName = *(static_cast<const WwiseMetadataMedia*>(Metadata)->ShortName);
	}
	else if(AssetType == WwiseRefType::SoundBank)
	{
		Metadata = Asset.GetSoundBank();
		AssetName = *(static_cast<const WwiseMetadataSoundBank*>(Metadata)->ShortName);
	}
	else
	{
		return false;
	}

	if (AssetType == WwiseRefType::Media && !bFilterMedia)
	{
		return bIsExclusionFilter;
	}
	if (AssetType == WwiseRefType::SoundBank && !bFilterSoundBanks)
	{
		return bIsExclusionFilter;
	}
	
	const bool bMatched { PatternMatch(Metadata, AssetType) };
	return bMatched == !bIsExclusionFilter;
}

void UWwiseAssetLibraryTextFilter::PostFilter(const FWwiseAssetLibraryFilteringSharedData& Shared,
	const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	GlobFilters.Empty();
}

void UWwiseAssetLibraryTextFilter::Reset(const FString& NewFilter, EWwiseAssetLibraryTextFilterType NewFilterType, bool NewIsExclusionFilter, bool NewUseRegex)
{
	Filter = NewFilter;
	FilterType = NewFilterType;
	bIsExclusionFilter = NewIsExclusionFilter;
	bUseRegex = NewUseRegex;
}
