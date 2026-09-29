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

#include "WwiseAssetLibraryFilterLanguage.generated.h"

UCLASS(BlueprintType, DisplayName="Keep Assets from a Specific Language", Meta = (Category = "Wwise|AssetLibrary|Filter"))
class WWISEPACKAGINGEDITOR_API UWwiseAssetLibraryFilterLanguage : public UWwiseAssetLibraryFilter
{
	GENERATED_BODY()

	UFUNCTION()
	TArray<FName> GetAvailableLanguages();

	FString LanguageListString;
public:
	UWwiseAssetLibraryFilterLanguage() {}

	UPROPERTY(EditAnywhere, Category = "Default", meta=(GetOptions="GetAvailableLanguages"))
	TSet<FString> SelectedLanguages;
	
	virtual void PreFilter(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const FWwiseAssetLibraryInfo& AssetLibraryInfo) override;
	
	virtual bool IsAssetAvailable(
		const FWwiseAssetLibraryFilteringSharedData& Shared,
		const WwiseAnyRef& Asset) const override;
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_WwiseAssetLibraryFilterLanguage : public FAssetTypeActions_Base_WwiseAssetLibraryFilter
{
public:
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
};