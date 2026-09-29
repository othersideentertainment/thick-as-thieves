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

#include "WwisePackagingSettings.generated.h"

class UWwiseAssetLibrary;

UCLASS(config = Game, defaultconfig)
class WWISEPACKAGING_API UWwisePackagingSettings : public UObject
{
	GENERATED_BODY()

public:
	static UWwisePackagingSettings* Get();
	
	UWwisePackagingSettings() {}
	virtual ~UWwisePackagingSettings() override {}

	//Determines whether the files are written as loose files in the packages, or inside the UAssets as Bulk Data.
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", DisplayName = "Package as Bulk Data")
	bool bPackageAsBulkData{ true };

	//Prioritized list of all Wwise Libraries that should be applied to a package Wwise assets as Bulk Data.
	//If no value is set or if they fall through, shared Wwise asset files will be cooked as Additional Data.
	//Ensure that the Shared Asset library is at the bottom of this list.
	UPROPERTY(Config, EditAnywhere, Category = "Cooking", DisplayName = "Libraries used for cooking Wwise UAssets as Bulk Data",
		meta=(EditCondition="bPackageAsBulkData", EditConditionHides=true))
	TArray<TSoftObjectPtr<UWwiseAssetLibrary>> AssetLibraries;

#if WITH_EDITORONLY_DATA
	//Editor: If we are enabling the Bulk Data for the first time, we should create a default Shared Asset library. 
	UPROPERTY(Config)
	bool bInitialAssetLibraryCreated{ false };
#endif

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingsChanged, UWwisePackagingSettings*);
#if WITH_EDITOR
	/**
	 * Called when the settings for Asset Libraries are changed.
	 *
	 * This is the equivalent of an UObject-specific FCoreUObjectDelegates::OnObjectPropertyChanged. 
	 */
	FOnSettingsChanged OnSettingsChanged;
#endif

#if WITH_EDITOR
protected:
	FDelegateHandle PostEngineInitDelegate;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;

	virtual void OnPostEngineInit();
	virtual bool EnsurePostEngineInit();
	virtual void CreateInitialAssetLibrary();

	bool SaveConfigFile();
#endif
};
