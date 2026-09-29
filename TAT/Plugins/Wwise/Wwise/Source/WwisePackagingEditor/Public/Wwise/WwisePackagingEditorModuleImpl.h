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

#include "PropertyEditorDelegates.h"
#include "Wwise/WwisePackagingEditorModule.h"

class FWwiseAssetLibraryProcessor;
class FAssetTypeActions_Base_WwisePackaging;

class WWISEPACKAGINGEDITOR_API FWwisePackagingEditorModule : public IWwisePackagingEditorModule
{
public:
	FWwisePackagingEditorModule();
	~FWwisePackagingEditorModule();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual void FillTypeActions();
	virtual void RegisterTypeActions();
	virtual void UnregisterTypeActions();
	virtual void RegisterDetailsCustomizations();
	virtual FOnGetDetailCustomizationInstance CreateAssetLibraryDetailLayoutDelegate();

	virtual FWwiseAssetLibraryPreCooker* InstantiatePreCooker(FWwiseProjectDatabase& ProjectDatabase) override;
	virtual FWwiseAssetLibraryProcessor* GetAssetLibraryProcessor() override;
	virtual FWwiseAssetLibraryProcessor* InstantiateAssetLibraryProcessor() const override;
		
	virtual void SetAssetLibrarySubMenus() override {};

	virtual const TArray<FText>& GetAssetLibrarySubMenu() override
	{
		static TArray<FText> Array;
		return Array;
	}

	virtual const TArray<FText>& GetSharedAssetLibraryFilterSubMenu() override
	{
		static TArray<FText> Array;
		return Array;
	}

protected:
	using SharedTypeActions = TSharedRef<FAssetTypeActions_Base_WwisePackaging>;
	TArray<SharedTypeActions> TypeActions_WwiseAssetLibrary;

	TUniquePtr<FWwiseAssetLibraryProcessor> AssetLibraryProcessor;

	EAssetTypeCategories::Type WwiseAssetTypeCategory;
};

