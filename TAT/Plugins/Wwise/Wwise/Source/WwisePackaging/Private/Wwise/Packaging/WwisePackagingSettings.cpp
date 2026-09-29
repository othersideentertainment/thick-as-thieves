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

#include "Wwise/Packaging/WwisePackagingSettings.h"


#if WITH_EDITOR
#include "Engine/Engine.h"
#include "Misc/ConfigCacheIni.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "ModuleDescriptor.h"
#include "SSettingsEditorCheckoutNotice.h"
#include "Interfaces/IPluginManager.h"
#include "Wwise/Packaging/WwiseAssetLibrary.h"
#include "Wwise/Packaging/WwiseAssetLibraryFilter.h"
#include "Wwise/Stats/Packaging.h"
#endif

#define LOCTEXT_NAMESPACE "WwisePackaging"

#if WITH_EDITOR

void UWwisePackagingSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UWwisePackagingSettings, bPackageAsBulkData))
	{
		CreateInitialAssetLibrary();
	}

	OnSettingsChanged.Broadcast(this);
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UWwisePackagingSettings::PostInitProperties()
{
	Super::PostInitProperties();

	CreateInitialAssetLibrary();
}

void UWwisePackagingSettings::OnPostEngineInit()
{
	CreateInitialAssetLibrary();
}

bool UWwisePackagingSettings::EnsurePostEngineInit()
{
	if (const ELoadingPhase::Type CurrentPhase{ IPluginManager::Get().GetLastCompletedLoadingPhase() };
		CurrentPhase == ELoadingPhase::None || CurrentPhase < ELoadingPhase::PostDefault || !GEngine)
	{
		if (!PostEngineInitDelegate.IsValid())
		{
			PostEngineInitDelegate = FCoreDelegates::OnPostEngineInit.AddUObject(this, &UWwisePackagingSettings::OnPostEngineInit);
		}
		return false;
	}

	return true;
}

void UWwisePackagingSettings::CreateInitialAssetLibrary()
{
	if(!bInitialAssetLibraryCreated && bPackageAsBulkData && AssetLibraries.Num() == 0)
	{
		if (!EnsurePostEngineInit()) return;

		static const FString AssetName{ TEXT("WwiseMultiReferenceAssetLibrary") };
		const FString DefaultAssetCreationPath{ GConfig->GetStringOrDefault(
			TEXT("/Script/AkAudio.AkSettings"),
			TEXT("DefaultAssetCreationPath"),
			TEXT("/Game/WwiseAudio"),
			GGameIni) };

		const FSoftObjectPath Path = DefaultAssetCreationPath / AssetName;
		UWwiseAssetLibrary* AssetLibrary{ nullptr };
		if (auto* LoadedUObject{ Path.TryLoad() })
		{
			AssetLibrary = Cast<UWwiseAssetLibrary>(LoadedUObject);
			if (LIKELY(IsValid(AssetLibrary)))
			{
				UE_LOG(LogWwisePackaging, Display, TEXT("UWwisePackagingSettings::CreateInitialAssetLibrary: Using existing Shared Asset Library at %s"), *Path.ToString());
			}
			else
			{
				AssetLibrary = nullptr;
			}
		}

		if (!AssetLibrary)
		{
			FScopedSlowTask SlowTask(0, LOCTEXT("CreateInitialAssetLibrary", "Creating initial Wwise Asset Library..."));
			UE_LOG(LogWwisePackaging, Display, TEXT("UWwisePackagingSettings::CreateInitialAssetLibrary: Creating Shared Asset Library at %s"), *Path.ToString());

			auto& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			AssetLibrary = Cast<UWwiseAssetLibrary>(AssetToolsModule.CreateAsset(AssetName, DefaultAssetCreationPath, UWwiseAssetLibrary::StaticClass(), nullptr));
			if (UNLIKELY(!AssetLibrary))
			{
				UE_LOG(LogWwisePackaging, Error, TEXT("UWwisePackagingSettings::CreateInitialAssetLibrary: Could not create Shared Asset Library at %s"), *Path.ToString());
				return;
			}

			// Find and add UWwiseAssetLibraryFilterMultiReference
			{
				FSoftClassPath MultiReferencePath{ TEXT("/Script/WwisePackagingEditor.WwiseAssetLibraryFilterMultiReference") };
				auto* MultiReferenceClass{ MultiReferencePath.ResolveClass() };
				if (UNLIKELY(!MultiReferenceClass))
				{
					UE_LOG(LogWwisePackaging, Error, TEXT("UWwisePackagingSettings::CreateInitialAssetLibrary: Could not find %s"), *MultiReferencePath.GetAssetPath().ToString());
					return;
				}

				auto* Filter = NewObject<UWwiseAssetLibraryFilter>(GetTransientPackage(), MultiReferenceClass);
				
				if (UNLIKELY(!Filter))
				{
					UE_LOG(LogWwisePackaging, Error, TEXT("UWwisePackagingSettings::CreateInitialAssetLibrary: Could not create Shared Filter for Asset Library at %s"), *Path.ToString());
					return;
				}
				AssetLibrary->Info.Filters.Add(Filter);
			}
			
			UEditorLoadingAndSavingUtils::SavePackages({ AssetLibrary->GetPackage() }, true);
		}

		AssetLibraries.Add(AssetLibrary);
		bInitialAssetLibraryCreated = true;
		SaveConfigFile();
	}
	else if(!bInitialAssetLibraryCreated && AssetLibraries.Num() > 0)
	{
		if (!EnsurePostEngineInit()) return;

		bInitialAssetLibraryCreated = true;
		SaveConfigFile();
	}
}

bool UWwisePackagingSettings::SaveConfigFile()
{
	const FString ConfigFilename = GetDefaultConfigFilename();
	if(ISourceControlModule::Get().IsEnabled())
	{
		if (!SettingsHelpers::IsCheckedOut(ConfigFilename, true))
		{
			if (!SettingsHelpers::CheckOutOrAddFile(ConfigFilename, true))
			{
				return false;
			}
		}
	}

	return TryUpdateDefaultConfigFile();
}
#endif

#undef LOCTEXT_NAMESPACE
