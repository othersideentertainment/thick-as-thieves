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

#include "Wwise/Packaging/WwiseAssetLibraryFilter.h"
#include "Wwise/Packaging/WwisePackagingTypeActions.h"

#include "WwiseAssetLibraryFilterSoundBankType.generated.h"

UCLASS(BlueprintType, DisplayName="Exclude User-Defined SoundBanks", Meta = (Category = "Wwise|AssetLibrary|Filter"))
class WWISEPACKAGINGEDITOR_API UWwiseAssetLibraryFilterUserDefinedSoundBanks : public UWwiseAssetLibraryFilter
{
	GENERATED_BODY()
	
public:
	UWwiseAssetLibraryFilterUserDefinedSoundBanks() {}

	// Filter SoundBanks based on their type
	// It will filter User Defined SoundBanks when disabled
	// It will filter Auto Defined SoundBanks when enabled
	UPROPERTY()
	bool bFilterUserBanks = false;
	
	virtual bool IsAssetAvailable(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const override;
};

UCLASS(BlueprintType, DisplayName="Exclude Auto-Defined SoundBanks", Meta = (Category = "Wwise|AssetLibrary|Filter"))
class WWISEPACKAGINGEDITOR_API UWwiseAssetLibraryFilterAutoDefinedSoundBanks : public UWwiseAssetLibraryFilterUserDefinedSoundBanks
{
	GENERATED_BODY()
	
public:
	UWwiseAssetLibraryFilterAutoDefinedSoundBanks()
	{
		bFilterUserBanks = true;
	}
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_WwiseAssetLibrarySoundBankType: public FAssetTypeActions_Base_WwiseAssetLibraryFilter
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_WwiseAssetLibraryFilterAutoDefinedSoundBankType: public FAssetTypeActions_WwiseAssetLibrarySoundBankType
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};