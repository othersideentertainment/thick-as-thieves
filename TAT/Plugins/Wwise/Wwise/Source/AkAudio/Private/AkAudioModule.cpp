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

#include "AkAudioModule.h"
#include "AkAudioDevice.h"
#include "AkAudioStyle.h"
#include "AkSettings.h"
#include "AkSettingsPerUser.h"
#include "WwiseUnrealDefines.h"

#include "Wwise/Packaging/WwiseAssetLibrary.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseSoundEngineModule.h"
#include "WwiseInitBankLoader/WwiseInitBankLoader.h"

#include "Misc/ScopedSlowTask.h"

#include "UObject/UObjectIterator.h"


#include "Wwise/API/WwiseSoundEngineAPI.h"

#if WITH_EDITORONLY_DATA
#include "Wwise/WwiseFileHandlerModule.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseDataStructure.h"
#include "Wwise/WwiseResourceCooker.h"
#endif

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Wwise/WwisePackagingEditorModule.h"
#include "Wwise/Packaging/WwiseAssetLibraryPreCooker.h"
#endif
#include "Async/Async.h"
#include "Platforms/AkPlatformInfo.h"
#include "Wwise/WwisePackagingModule.h"
#include "Wwise/WwiseExternalSourceManager.h"
#include "Wwise/Packaging/WwisePackagingSettings.h"

IMPLEMENT_MODULE(FAkAudioModule, AkAudio)
#define LOCTEXT_NAMESPACE "AkAudio"

FAkAudioModule* FAkAudioModule::AkAudioModuleInstance = nullptr;
FSimpleMulticastDelegate FAkAudioModule::OnModuleInitialized;
FSimpleMulticastDelegate FAkAudioModule::OnWwiseAssetDataReloaded;

// WwiseUnrealHelper overrides

namespace WwiseUnrealHelper
{
	static bool IsValidClass(UClass* inClass)
	{
		return IsValid(inClass) && inClass->GetName() != "None";
	}
	
	static FString GetWwiseSoundEnginePluginDirectoryImpl()
	{
		return FAkPlatform::GetWwiseSoundEnginePluginDirectory();
	}

	static FString GetWwiseProjectPathImpl()
	{
		if(!IsValidClass(UAkSettings::StaticClass()))
		{
			return {};
		}

		FString projectPath;

		if (auto* settings = GetDefault<UAkSettings>())
		{
			projectPath = settings->WwiseProjectPath.FilePath;

			if (FPaths::IsRelative(projectPath))
			{
				projectPath = FPaths::ConvertRelativePathToFull(GetProjectDirectory(), projectPath);
			}

#if PLATFORM_WINDOWS
			projectPath.ReplaceInline(TEXT("/"), TEXT("\\"));
#endif
		}

		return projectPath;
	}

	static FString GetSoundBankDirectoryImpl()
	{
		if(!IsValidClass(UAkSettingsPerUser::StaticClass()) || !IsValidClass(UAkSettings::StaticClass()))
		{
			return {};
		}

		const UAkSettingsPerUser* UserSettings = GetDefault<UAkSettingsPerUser>();
		FString SoundBankDirectory;
		if (UserSettings && !UserSettings->RootOutputPathOverride.Path.IsEmpty())
		{
			SoundBankDirectory = UserSettings->RootOutputPathOverride.Path;
			if(FPaths::IsRelative(UserSettings->RootOutputPathOverride.Path))
			{
				SoundBankDirectory = FPaths::Combine(GetContentDirectory(), UserSettings->RootOutputPathOverride.Path);
			}
		}
		else if (const UAkSettings* AkSettings = GetDefault<UAkSettings>())
		{
			if(AkSettings->RootOutputPath.Path.IsEmpty())
			{
				return {};
			}
			SoundBankDirectory = AkSettings->RootOutputPath.Path;
			if(FPaths::IsRelative(AkSettings->RootOutputPath.Path))
			{
				SoundBankDirectory = FPaths::Combine(GetContentDirectory(), AkSettings->RootOutputPath.Path);	
			}
		}
		else
		{
			UE_LOG(LogAkAudio, Warning, TEXT("WwiseUnrealHelper::GetSoundBankDirectory : Please set the Generated Soundbanks Folder in Wwise settings. Otherwise, sound will not function."));
			return {};
		}
		FPaths::CollapseRelativeDirectories(SoundBankDirectory);
		if(!SoundBankDirectory.EndsWith(TEXT("/")))
		{
			SoundBankDirectory.AppendChar('/');
		}

		return SoundBankDirectory;
	}

	static FString GetStagePathFromSettings()
	{
		if(!IsValidClass(UAkSettings::StaticClass()))
		{
			return {};
		}

		// In !WITH_EDITORONLY_DATA, you must prepend `FPaths::ProjectContentDir() /` to the defined Stage Path
		const UAkSettings* Settings = GetDefault<UAkSettings>();
		if (Settings && !Settings->WwiseStagingDirectory.Path.IsEmpty())
		{
			return Settings->WwiseStagingDirectory.Path;
		}
		return TEXT("WwiseAudio");
	}
}

void FAkAudioModule::StartupModule()
{
	IWwiseSoundEngineModule::ForceLoadModule();
	WwiseUnrealHelper::SetHelperFunctions(
		WwiseUnrealHelper::GetWwiseSoundEnginePluginDirectoryImpl,
		WwiseUnrealHelper::GetWwiseProjectPathImpl,
		WwiseUnrealHelper::GetSoundBankDirectoryImpl);

#if WITH_EDITOR
	// It is not wanted to initialize the SoundEngine while running the GenerateSoundBanks commandlet.
	if (IsRunningCommandlet())
	{
		// We COULD use GetRunningCommandletClass(), but unfortunately it is set to nullptr in OnPostEngineInit.
		// We need to parse the command line.
		FString CmdLine(FCommandLine::Get());
		if (CmdLine.Contains(TEXT("run=GenerateSoundBanks")))
		{
			UE_LOG(LogAkAudio, Log, TEXT("FAkAudioModule::StartupModule: Detected GenerateSoundBanks commandlet is running. AkAudioModule will not be initialized."));
			return;
		}

#if WITH_EDITORONLY_DATA
		if(!IWwiseProjectDatabaseModule::ShouldInitializeProjectDatabase())
		{
			// Initialize the Rersource Cooker
			IWwiseResourceCookerModule::GetModule();
		}
#endif
	}
#endif

	if (AkAudioModuleInstance == this)
	{
		UE_LOG(LogAkAudio, Log, TEXT("FAkAudioModule::StartupModule: AkAudioModuleInstance already exists."));
		return;
	}
	UE_CLOG(AkAudioModuleInstance, LogAkAudio, Warning, TEXT("FAkAudioModule::StartupModule: Updating AkAudioModuleInstance from (%p) to (%p)! Previous Module instance was improperly shut down!"), AkAudioModuleInstance, this);

	AkAudioModuleInstance = this;

	FScopedSlowTask SlowTask(0, LOCTEXT("InitWwisePlugin", "Initializing Wwise Plug-in AkAudioModule..."));

#if !UE_SERVER
	UpdateWwiseResourceCookerSettings();
#endif
	
#if WITH_EDITORONLY_DATA
	if (auto* AkSettings = GetDefault<UAkSettings>())
	{
		if (AkSettings->AreSoundBanksGenerated())
		{
			ParseGeneratedSoundBankData();
			FWwiseInitBankLoader::Get()->UpdateInitBankInSettings();
#if !UE_SERVER
			UpdateWwiseResourceCookerSettings();
#endif
		}
	}

	// Loading the File Handler Module, in case it loads a different module with UStructs, so it gets packaged (Ex.: Simple External Source Manager)
	IWwiseFileHandlerModule::GetModule();

	// Loading the AssetLibrary module to set the cooking initialization function
	if (auto* PackagingModule = IWwisePackagingModule::GetModule())
	{
		PackagingModule->SetCreateResourceCookerForPlatformFct([](const ITargetPlatform* TargetPlatform)
		{
			CreateResourceCookerForPlatform(TargetPlatform);
		});
	}
	
#endif

	AkAudioDevice = new FAkAudioDevice;
	if (!AkAudioDevice)
	{
		UE_LOG(LogAkAudio, Log, TEXT("FAkAudioModule::StartupModule: Couldn't create FAkAudioDevice. AkAudioModule will not be fully initialized."));
		bModuleInitialized = true;
		return;
	}

	// Loading Asset Libraries
#if !WITH_EDITORONLY_DATA && !UE_SERVER
	{
		if (auto* PackagingSettings = GetDefault<UWwisePackagingSettings>())
		{
			bool bPackageAsBulkData = PackagingSettings->bPackageAsBulkData;
			const auto& AssetLibraries = PackagingSettings->AssetLibraries;
			if (bPackageAsBulkData && AssetLibraries.Num() > 0)
			{
				FScopedSlowTask SlowTaskAssetLibrary(0, LOCTEXT("InitAssetLibraries", "Initializing Wwise AssetLibrary..."));
				for (const auto& AssetLibrary : AssetLibraries)
				{
					SCOPED_AKAUDIO_EVENT_F_2(TEXT("FAkAudioModule::StartupModule: Loading AssetLibrary(%s)"), *AssetLibrary.GetAssetName());
					UE_LOG(LogAkAudio, Verbose, TEXT("FAkAudioModule::StartupModule: Loading Asset Library %s"), *AssetLibrary.GetAssetName());
					auto* Loaded = AssetLibrary.LoadSynchronous();
					UE_CLOG(UNLIKELY(!Loaded), LogAkAudio, Warning, TEXT("FAkAudioModule::StartupModule: Unable to load asset library %s."), *AssetLibrary.GetAssetName());
				}
			}
		}
	}
#endif

	if (!AkAudioDevice->Init())
	{
		UE_LOG(LogAkAudio, Log, TEXT("FAkAudioModule::StartupModule: Couldn't initialize FAkAudioDevice. AkAudioModule will not be fully initialized."));
		bModuleInitialized = true;
		delete AkAudioDevice;
		AkAudioDevice = nullptr;
		return;
	}

	//Load init bank in Runtime
	UE_LOG(LogAkAudio, VeryVerbose, TEXT("FAkAudioModule::StartupModule: Loading Init Bank."));
	FWwiseInitBankLoader::Get()->LoadInitBank();

	OnTick = FTickerDelegate::CreateRaw(AkAudioDevice, &FAkAudioDevice::Update);
	TickDelegateHandle = FCoreTickerType::GetCoreTicker().AddTicker(OnTick);

	AkAudioDevice->LoadDelayedObjects();

	UE_LOG(LogAkAudio, VeryVerbose, TEXT("FAkAudioModule::StartupModule: Module Initialized."));
	OnModuleInitialized.Broadcast();
	bModuleInitialized = true;
}

void FAkAudioModule::ShutdownModule()
{
	UE_CLOG(AkAudioModuleInstance && AkAudioModuleInstance != this, LogAkAudio, Warning, TEXT("FAkAudioModule::ShutdownModule: Shutting down a different instance (%p) that was initially instantiated (%p)!"), this, AkAudioModuleInstance);

	FCoreTickerType::GetCoreTicker().RemoveTicker(TickDelegateHandle);

	if (AkAudioDevice)
	{
		AkAudioDevice->Teardown();
		delete AkAudioDevice;
		AkAudioDevice = nullptr;
	}

	if (IWwiseSoundEngineModule::IsAvailable())
	{
		WwiseUnrealHelper::SetHelperFunctions(nullptr, nullptr, nullptr);
	}

	AkAudioModuleInstance = nullptr;
}

FAkAudioDevice* FAkAudioModule::GetAkAudioDevice() const
{
	return AkAudioDevice;
}

void FAkAudioModule::ReloadWwiseAssetData() const
{
	SCOPED_AKAUDIO_EVENT(TEXT("ReloadWwiseAssetData"));
	if (FAkAudioDevice::IsInitialized())
	{
		UE_LOG(LogAkAudio, Log, TEXT("FAkAudioModule::ReloadWwiseAssetData : Reloading Wwise asset data."));
		if (AkAudioDevice)
		{
			AkAudioDevice->ClearSoundBanksAndMedia();
		}
		
		auto* InitBankLoader = FWwiseInitBankLoader::Get();
		if (LIKELY(InitBankLoader))
		{
			InitBankLoader->LoadInitBank();
		}
		else
		{
			UE_LOG(LogAkAudio, Error, TEXT("LoadInitBank: WwiseInitBankLoader is not initialized."));
		}

		for (TObjectIterator<UAkAudioType> AudioAssetIt; AudioAssetIt; ++AudioAssetIt)
		{
			AudioAssetIt->LoadData();
		}
		OnWwiseAssetDataReloaded.Broadcast();
	}
	else
	{
		UE_LOG(LogAkAudio, Verbose, TEXT("FAkAudioModule::ReloadWwiseAssetData : Skipping asset data reload because the SoundEngine is not initialized."));
	}
}

void FAkAudioModule::UpdateWwiseResourceCookerSettings()
{
#if WITH_EDITORONLY_DATA
	SCOPED_AKAUDIO_EVENT(TEXT("UpdateWwiseResourceCookerSettings"));

	auto* ResourceCooker = IWwiseResourceCooker::GetDefault();
	if (!ResourceCooker)
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::UpdateWwiseResourceCookerSettings : No Default Resource Cooker!"));
		return;
	}
	auto* ProjectDatabase = ResourceCooker->GetProjectDatabase();
	if (!ProjectDatabase)
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::UpdateWwiseResourceCookerSettings : No Project Database!"));
		return;
	}
	auto ExternalSourceManager = IWwiseExternalSourceManager::Get();
	if (!ExternalSourceManager)
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::UpdateWwiseResourceCookerSettings : No External Source Manager!"))
		return;
	}

	const auto StagePath = WwiseUnrealHelper::GetStagePathFromSettings();
	ResourceCooker->SetWwiseStagePath(StagePath);

	const auto SoundBankDirectory = WwiseUnrealHelper::GetSoundBankDirectory();
	ProjectDatabase->SetGeneratedSoundBanksPath(FDirectoryPath{ SoundBankDirectory });

	const auto& Platform = ProjectDatabase->GetCurrentPlatform().Platform.Get();
	if (!Platform.ExternalSourceRootPath.IsNone())
	{
		auto ExternalSourcePath = Platform.PathRelativeToGeneratedSoundBanks.ToString() / Platform.ExternalSourceRootPath.ToString();
		if (FPaths::IsRelative(ExternalSourcePath))
		{
			ExternalSourcePath = SoundBankDirectory / ExternalSourcePath;
		}
		ExternalSourceManager->SetExternalSourcePath(FDirectoryPath{ExternalSourcePath });
	}
#endif
}

#if WITH_EDITORONLY_DATA
void FAkAudioModule::CreateResourceCookerForPlatform(const ITargetPlatform* TargetPlatform)
{
	SCOPED_AKAUDIO_EVENT(TEXT("CreateResourceCookerForPlatform"));

	// OSE BEGIN: Judd, 10/24/2024: There are several places below which do not check for server platforms and then error out when a resource cooker is not created
	if (TargetPlatform != nullptr && TargetPlatform->IsServerOnly())
	{
		return;
	}
	// OSE END

	auto* ResourceCooker = IWwiseResourceCooker::GetForPlatform(TargetPlatform);
	FWwiseProjectDatabase* ProjectDatabase;

	if (LIKELY(ResourceCooker))
	{
		ProjectDatabase = ResourceCooker->GetProjectDatabase();
		if (!ProjectDatabase)
		{
			UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : No Project Database!"));
			return;
		}
	}
	else
	{
		const auto PlatformID = UAkPlatformInfo::GetSharedPlatformInfo(TargetPlatform->IniPlatformName());
		if (UNLIKELY(!PlatformID.IsValid()))
		{
			UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : Could not get platform info for target %s!"), *TargetPlatform->IniPlatformName());
			return;
		}

		bool bPackageAsBulkData{ false };
		const FWwiseAssetLibraryPreCooker::FAssetLibraryArray* AssetLibraries { nullptr };
		if (auto* WwisePackagingSettings = GetDefault<UWwisePackagingSettings>())
		{
			bPackageAsBulkData = WwisePackagingSettings->bPackageAsBulkData;
			AssetLibraries = &WwisePackagingSettings->AssetLibraries;
		}

		if(IsRunningCookCommandlet() && FParse::Param(FCommandLine::Get(), TEXT("DisableWwiseBulkData")))
		{
			UE_LOG(LogAkAudio, Display, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : Disabling bulk data packaging because of command line."));
			bPackageAsBulkData = false;
		}
		
		EWwisePackagingStrategy TargetPackagingStrategy{ EWwisePackagingStrategy::AdditionalFile };
		if (bPackageAsBulkData)
		{
			TargetPackagingStrategy = EWwisePackagingStrategy::BulkData;
		}

		ResourceCooker = IWwiseResourceCooker::CreateForPlatform(TargetPlatform, PlatformID, TargetPackagingStrategy, EWwiseExportDebugNameRule::Name);
		if (UNLIKELY(!ResourceCooker))
		{
			UE_CLOG(!TargetPlatform->IsServerOnly(), LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : Could not create Resource Cooker!"));
			return;
		}

		ProjectDatabase = ResourceCooker->GetProjectDatabase();
		if (!ProjectDatabase)
		{
			UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : No Project Database!"));
			return;
		}

		if (bPackageAsBulkData && AssetLibraries && AssetLibraries->Num() > 0)
		{
			auto* AssetLibraryEditorModule = IWwisePackagingEditorModule::GetModule();
			if (UNLIKELY(!AssetLibraryEditorModule))
			{
				UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : Could not get AssetLibraryEditor module!"));
				return;
			}
			const TUniquePtr<FWwiseAssetLibraryPreCooker> AssetLibraryPreCooker { AssetLibraryEditorModule ? AssetLibraryEditorModule->InstantiatePreCooker(*ProjectDatabase) : nullptr };
			if (!AssetLibraryPreCooker)
			{
				UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : Could not create AssetLibrary PreCooker!"));
				return;
			}
			ResourceCooker->PreCacheAssetLibraries(AssetLibraryPreCooker->Process(*AssetLibraries));
		}
	}
	
	auto ExternalSourceManager = IWwiseExternalSourceManager::Get();
	if (!ExternalSourceManager)
	{
		UE_LOG(LogAkAudio, Error, TEXT("FAkAudioModule::CreateResourceCookerForPlatform : No External Source Manager!"))
		return;
	}

	const auto StagePath = WwiseUnrealHelper::GetStagePathFromSettings();
	ResourceCooker->SetWwiseStagePath(StagePath);

	const auto SoundBankDirectory = WwiseUnrealHelper::GetSoundBankDirectory();
	ProjectDatabase->SetGeneratedSoundBanksPath(FDirectoryPath{ SoundBankDirectory });

	const auto& Platform = ProjectDatabase->GetCurrentPlatform().Platform.Get();
	if (!Platform.ExternalSourceRootPath.IsNone())
	{
		auto ExternalSourcePath = Platform.PathRelativeToGeneratedSoundBanks.ToString() / Platform.ExternalSourceRootPath.ToString();
		if (FPaths::IsRelative(ExternalSourcePath))
		{
			ExternalSourcePath = SoundBankDirectory / ExternalSourcePath;
		}
		ExternalSourceManager->SetExternalSourcePath(FDirectoryPath{ExternalSourcePath });
	}
}

void FAkAudioModule::ParseGeneratedSoundBankData()
{
	SCOPED_AKAUDIO_EVENT(TEXT("ParseGeneratedSoundBankData"));
	UE_LOG(LogAkAudio, Log, TEXT("FAkAudioModule::ParseGeneratedSoundBankData : Parsing Wwise project data."));
	auto* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAkAudio, Warning, TEXT("FAkAudioModule::ParseGeneratedSoundBankData : Could not get FWwiseProjectDatabase instance. Generated sound data will not be parsed."));
	}
	else
	{
		ProjectDatabase->UpdateDataStructure();
	}
}
#endif

#undef LOCTEXT_NAMESPACE