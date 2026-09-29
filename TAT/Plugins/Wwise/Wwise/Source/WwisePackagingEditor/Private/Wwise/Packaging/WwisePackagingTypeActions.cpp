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

#include "Wwise/Packaging/WwisePackagingTypeActions.h"

#include "AssetTypeCategories.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Wwise/WwiseEditorUtilsModule.h"
#include "Wwise/WwisePackagingEditorModule.h"

#include "Wwise/Packaging/WwiseAssetLibrary.h"
#include "Wwise/Packaging/WwiseSharedAssetLibraryFilter.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_WwiseAssetLibrary::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseAssetLibrary", "Wwise Asset Library");
}

FColor FAssetTypeActions_WwiseAssetLibrary::GetTypeColor() const
{
	return FColor(0, 84, 159);
}

const TArray<FText>& FAssetTypeActions_WwiseAssetLibrary::GetSubMenus() const
{
	if (auto* EditorUtils = IWwisePackagingEditorModule::GetModule())
	{
		return EditorUtils->GetAssetLibrarySubMenu();
	}
	else
	{
		static TArray<FText> Submenu{};
		return Submenu;
	}
}

UClass* FAssetTypeActions_WwiseAssetLibrary::GetSupportedClass() const
{
	return UWwiseAssetLibrary::StaticClass();
}

uint32 FAssetTypeActions_WwiseAssetLibrary::GetCategories()
{
	if (auto* EditorUtils = IWwiseEditorUtilsModule::GetModule())
	{
		return EditorUtils->GetWwiseAssetTypeCategory();
	}
	else
	{
		return EAssetTypeCategories::Sounds;
	}
}


FColor FAssetTypeActions_Base_WwiseAssetLibraryFilter::GetTypeColor() const
{
	return FColor(101, 45, 134);
}

const TArray<FText>& FAssetTypeActions_Base_WwiseAssetLibraryFilter::GetSubMenus() const
{
	if (auto* EditorUtils = IWwisePackagingEditorModule::GetModule())
	{
		return EditorUtils->GetAssetLibrarySubMenu();
	}
	else
	{
		static TArray<FText> Submenu{};
		return Submenu;
	}
}

uint32 FAssetTypeActions_Base_WwiseAssetLibraryFilter::GetCategories()
{
	if (auto* EditorUtils = IWwiseEditorUtilsModule::GetModule())
	{
		return EditorUtils->GetWwiseAssetTypeCategory();
	}
	else
	{
		return EAssetTypeCategories::Sounds;
	}
}

FText FAssetTypeActions_WwiseSharedAssetLibraryFilter::GetName() const
{
	return LOCTEXT("AssetTypeActions_WwiseSharedAssetLibraryFilter", "Wwise Shared Asset Library Filter");
}


FColor FAssetTypeActions_WwiseSharedAssetLibraryFilter::GetTypeColor() const
{
	return FColor(101, 45, 134);
}

const TArray<FText>& FAssetTypeActions_WwiseSharedAssetLibraryFilter::GetSubMenus() const
{
	if (auto* EditorUtils = IWwisePackagingEditorModule::GetModule())
	{
		return EditorUtils->GetAssetLibrarySubMenu();
	}
	else
	{
		static TArray<FText> Submenu{};
		return Submenu;
	}
}

UClass* FAssetTypeActions_WwiseSharedAssetLibraryFilter::GetSupportedClass() const
{
	return UWwiseSharedAssetLibraryFilter::StaticClass();
}

uint32 FAssetTypeActions_WwiseSharedAssetLibraryFilter::GetCategories()
{
	if (auto* EditorUtils = IWwiseEditorUtilsModule::GetModule())
	{
		return EditorUtils->GetWwiseAssetTypeCategory();
	}
	else
	{
		return EAssetTypeCategories::Sounds;
	}
}

#undef LOCTEXT_NAMESPACE
