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

#include "Wwise/Packaging/Filters/WwiseAssetLibraryFilterSoundBankType.h"

#include "Wwise/Packaging/WwiseAssetLibraryFilteringSharedData.h"

bool UWwiseAssetLibraryFilterUserDefinedSoundBanks::IsAssetAvailable(const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const
{
	if(auto SoundBankRef = Asset.GetSoundBankRef())
	{
		return !SoundBankRef->IsInitBank() && SoundBankRef->IsUserBank() == bFilterUserBanks;
	}
	if(auto MediaRef = Asset.GetMediaRef())
	{
		return MediaRef->IsUserBank() == bFilterUserBanks;
	}
	return false;
}

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_WwiseAssetLibrarySoundBankType::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseAssetLibraryFilterUserDefined", "Wwise Asset Library Filter : User Defined SoundBank");
}

UClass* FAssetTypeActions_WwiseAssetLibrarySoundBankType::GetSupportedClass() const
{
	return UWwiseAssetLibraryFilterUserDefinedSoundBanks::StaticClass();
}

FText FAssetTypeActions_WwiseAssetLibraryFilterAutoDefinedSoundBankType::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseAssetLibraryFilterAutoDefined", "Wwise Asset Library Filter : Auto Defined SoundBank");
}

UClass* FAssetTypeActions_WwiseAssetLibraryFilterAutoDefinedSoundBankType::GetSupportedClass() const
{
	return UWwiseAssetLibraryFilterAutoDefinedSoundBanks::StaticClass();
}

#undef LOCTEXT_NAMESPACE
