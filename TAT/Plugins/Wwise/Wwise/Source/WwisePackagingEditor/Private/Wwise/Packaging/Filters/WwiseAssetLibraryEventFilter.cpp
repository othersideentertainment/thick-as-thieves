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

#include "Wwise/Packaging/Filters/WwiseAssetLibraryEventFilter.h"

#include "Internationalization/Regex.h"
#include "Wwise/Packaging/WwiseAssetLibraryFilteringSharedData.h"
#include "Wwise/Stats/PackagingEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_WwiseAssetLibraryEventFilter::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseAssetLibraryEventFilter", "Wwise Asset Library Filter : Event");
}

UClass* FAssetTypeActions_WwiseAssetLibraryEventFilter::GetSupportedClass() const
{
	return UWwiseAssetLibraryEventFilter::StaticClass();
}

#undef LOCTEXT_NAMESPACE

void UWwiseAssetLibraryEventFilter::FilterSoundBank(bool bInside, const WwiseMetadataSoundBank* SoundBank)
{
	{
		auto& SoundBankSet{ bInside ? AdditionalSoundBank : OutsideSoundBank };
	
		SoundBankSet.Add(*SoundBank->ShortName);
	}

	if (bFilterMedia)
	{
		auto& MediaSet{ bInside ? AdditionalMedia : OutsideMedia };
		for (const auto& Media : SoundBank->Media)
		{
			MediaSet.Add(*Media.ShortName);
		}
	}
}

void UWwiseAssetLibraryEventFilter::FilterMedia(bool bInside, const WwiseMetadataMedia* Media)
{
	auto& MediaSet{ bInside ? AdditionalMedia : OutsideMedia };
	MediaSet.Add(*Media->ShortName);
}

bool UWwiseAssetLibraryEventFilter::PatternMatch(const FString& InputText) const
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

bool UWwiseAssetLibraryEventFilter::RegexSearch(const FString& Pattern, const FString& InputText)
{
	//Empty filter is invalid
	if(Pattern.IsEmpty())
	{
		return false;
	}
	FRegexMatcher Matcher(FRegexPattern(Pattern), InputText);
	return Matcher.FindNext();
}

bool UWwiseAssetLibraryEventFilter::GlobSearch(const FString& Pattern, const FString& InputText, bool bCaseSensitive, int32 PatternIndex, int32 InputIndex)
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

void UWwiseAssetLibraryEventFilter::PreFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	if (!bUseRegex)
	{
		TArray<FString> GlobFiltersArray;
		Filter.ParseIntoArrayWS(GlobFiltersArray);
		GlobFilters = TSet(GlobFiltersArray);
	}

	const WwiseEventGlobalIdsMap& Events = Shared.Db.GetEvents();
	for (auto Event : Events)
	{
		const bool bInside = PatternMatch(**Event.Value.EventName());
		if (!bInside && !bIncludeSingleReferenceAssetsOnly)
		{
			continue;
		}
		
		if (bFilterSoundBanks)
		{
			FilterSoundBank(bInside, Event.Value.GetSoundBank());
		}
		if (bFilterMedia)
		{
			const auto AllMedia = Event.Value.GetAllMedia(Shared.Db.GetMediaFiles());
			for (auto Media: AllMedia)
			{
				FilterMedia(bInside, Media.Value.GetMedia());	
			}
		}
	}

	if (bIncludeSingleReferenceAssetsOnly)
	{
		AdditionalMedia = AdditionalMedia.Difference(OutsideMedia);
		AdditionalSoundBank = AdditionalSoundBank.Difference(OutsideSoundBank);

		OutsideMedia.Reset();
		OutsideSoundBank.Reset();
	}
}

bool UWwiseAssetLibraryEventFilter::IsAssetAvailable(const FWwiseAssetLibraryFilteringSharedData& Shared,
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

	const bool bMatched {
		AssetType == WwiseRefType::Media ? AdditionalMedia.Contains(AssetName) : AdditionalSoundBank.Contains(AssetName)
	};
		
	return bMatched;
}

void UWwiseAssetLibraryEventFilter::PostFilter(const FWwiseAssetLibraryFilteringSharedData& Shared,
	const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	GlobFilters.Empty();
	AdditionalMedia.Empty();
	AdditionalSoundBank.Empty();
}

void UWwiseAssetLibraryEventFilter::Reset(const FString& NewFilter, bool NewIncludeSingleReferenceAssetsOnly, bool NewUseRegex)
{
	Filter = NewFilter;
	bIncludeSingleReferenceAssetsOnly = NewIncludeSingleReferenceAssetsOnly;
	bUseRegex = NewUseRegex;
}
