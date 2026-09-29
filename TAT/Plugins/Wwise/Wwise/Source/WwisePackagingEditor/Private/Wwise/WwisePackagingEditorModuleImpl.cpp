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

#include "Wwise/WwisePackagingEditorModuleImpl.h"

#include "Wwise/Packaging/WwiseAssetLibrary.h"
#include "Wwise/Packaging/WwiseAssetLibraryDetailsCustomization.h"
#include "Wwise/Packaging/WwiseAssetLibraryPreCooker.h"
#include "Wwise/Packaging/WwiseAssetLibraryProcessor.h"
#include "Wwise/Packaging/WwisePackagingTypeActions.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PropertyEditorModule.h"
#include "Wwise/Packaging/WwiseSharedAssetLibraryFilter.h"

IMPLEMENT_MODULE(FWwisePackagingEditorModule, WwisePackagingEditor)

#define LOCTEXT_NAMESPACE "Wwise"

FWwisePackagingEditorModule::FWwisePackagingEditorModule()
{
}

inline FWwisePackagingEditorModule::~FWwisePackagingEditorModule()
{
}

void FWwisePackagingEditorModule::StartupModule()
{
	RegisterTypeActions();
	RegisterDetailsCustomizations();
	SetAssetLibrarySubMenus();
}

void FWwisePackagingEditorModule::ShutdownModule()
{
	UnregisterTypeActions();
}

void FWwisePackagingEditorModule::FillTypeActions()
{
	TypeActions_WwiseAssetLibrary.Emplace( MakeShared<FAssetTypeActions_WwiseAssetLibrary>() );
	TypeActions_WwiseAssetLibrary.Emplace( MakeShared<FAssetTypeActions_WwiseSharedAssetLibraryFilter>() );
}

void FWwisePackagingEditorModule::RegisterTypeActions()
{
	FillTypeActions();
	
	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
	for(const auto& TypeAction : TypeActions_WwiseAssetLibrary)
	{
		AssetTools.RegisterAssetTypeActions(TypeAction);
	}
}

void FWwisePackagingEditorModule::UnregisterTypeActions()
{
	if (FAssetToolsModule::IsModuleLoaded())
	{
		IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();
		for(const auto& TypeAction : TypeActions_WwiseAssetLibrary)
		{
			AssetTools.UnregisterAssetTypeActions(TypeAction);
		}
	}
	TypeActions_WwiseAssetLibrary.Reset();
}

void FWwisePackagingEditorModule::RegisterDetailsCustomizations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UWwiseAssetLibrary::StaticClass()->GetFName(),
		CreateAssetLibraryDetailLayoutDelegate());
	PropertyModule.RegisterCustomClassLayout(UWwiseSharedAssetLibraryFilter::StaticClass()->GetFName(),
		CreateAssetLibraryDetailLayoutDelegate());
}

FOnGetDetailCustomizationInstance FWwisePackagingEditorModule::CreateAssetLibraryDetailLayoutDelegate()
{
	return FOnGetDetailCustomizationInstance::CreateStatic(&FWwiseAssetLibraryDetailsCustomization::MakeInstance);
}


FWwiseAssetLibraryPreCooker* FWwisePackagingEditorModule::InstantiatePreCooker(FWwiseProjectDatabase& ProjectDatabase)
{
	return new FWwiseAssetLibraryPreCooker(ProjectDatabase);
}

FWwiseAssetLibraryProcessor* FWwisePackagingEditorModule::GetAssetLibraryProcessor()
{
	if (!AssetLibraryProcessor.IsValid())
	{
		AssetLibraryProcessor.Reset(InstantiateAssetLibraryProcessor());
	}
	return AssetLibraryProcessor.Get();
}

FWwiseAssetLibraryProcessor* FWwisePackagingEditorModule::InstantiateAssetLibraryProcessor() const
{
	return new FWwiseAssetLibraryProcessor;
}

#undef LOCTEXT_NAMESPACE
