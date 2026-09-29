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

#include "Modules/ModuleManager.h"
#include "Wwise/Stats/Wwise.h"

#if !UE_SERVER
#include "Misc/CoreDelegates.h"
#include "Misc/ConfigCacheIni.h"
#include "WwiseDefines.h"
#include "WwiseUnrealDefines.h"

#include "Wwise/WwisePackagingRuntimeModuleImpl.h"
#if WITH_EDITOR
#include "Wwise/WwisePackagingEditorModuleImpl.h"
#endif

#include "Wwise/WwiseAudioLinkRuntimeModule.h"
#if WITH_EDITOR
#include "Wwise/WwiseAudioLinkEditorModule.h"
#endif
#endif

class FWwiseModule : public IModuleInterface
{
#if !UE_SERVER
public:
	virtual void StartupModule() override
	{
		auto& ModuleManager = FModuleManager::Get();

		bool bPackageAsBulkData = false;
		GConfig->GetBool(TEXT("/Script/WwisePackaging.WwisePackagingSettings"), TEXT("bPackageAsBulkData"), bPackageAsBulkData, GGameIni);
		if (bPackageAsBulkData || WITH_EDITOR)
		{
			{
				SCOPED_WWISE_EVENT(TEXT("StartupModule: WwisePackaging Runtime"));
				UE_LOG(LogWwise, Log, TEXT("WwiseModule: Loading WwisePackagingRuntime"));
				IWwisePackagingRuntimeModule::GetModule();
			}
#if WITH_EDITOR
			{
				SCOPED_WWISE_EVENT(TEXT("StartupModule: WwisePackaging Editor"));
				UE_LOG(LogWwise, Log, TEXT("WwiseModule: Loading WwisePackagingEditor"));
				IWwisePackagingEditorModule::GetModule();
			}
#endif
		}
		else
		{
			UE_LOG(LogWwise, Verbose, TEXT("WwiseModule: WwisePackaging is disabled without Bulk Data Packaging. Ignoring."));
		}

		{
			SCOPED_WWISE_EVENT(TEXT("StartupModule: AkAudio"));
			UE_LOG(LogWwise, Log, TEXT("WwiseModule: Loading AkAudio"));
			ModuleManager.LoadModule(TEXT("AkAudio"));
			FCoreDelegates::OnPostEngineInit.AddRaw(this, &FWwiseModule::OnPostEngineInit);
		}

		// Loading optional modules - Modules defined by Project Settings
		bool bAkAudioMixerEnabled = false;
		GConfig->GetBool(TEXT("/Script/AkAudio.AkSettings"), TEXT("bAkAudioMixerEnabled"), bAkAudioMixerEnabled, GGameIni);
		if (bAkAudioMixerEnabled)
		{
			SCOPED_WWISE_EVENT(TEXT("StartupModule: AkAudioMixer"));
			UE_LOG(LogWwise, Log, TEXT("WwiseModule: Loading AkAudioMixer"));
			ModuleManager.LoadModule(TEXT("AkAudioMixer"));
		}

		bool bWwiseAudioLinkEnabled = false;
		GConfig->GetBool(TEXT("/Script/AkAudio.AkSettings"), TEXT("bWwiseAudioLinkEnabled"), bWwiseAudioLinkEnabled, GGameIni);
		if (bWwiseAudioLinkEnabled)
		{
			{
				SCOPED_WWISE_EVENT(TEXT("StartupModule: WwiseAudioLink Runtime"));
				UE_LOG(LogWwise, Log, TEXT("WwiseModule: Loading WwiseAudioLink"));
				ModuleManager.LoadModule(TEXT("WwiseAudioLinkRuntime"));
			}
#if WITH_EDITOR
			{
				SCOPED_WWISE_EVENT(TEXT("StartupModule: WwiseAudioLink Editor"));
				ModuleManager.LoadModule(TEXT("WwiseAudioLinkEditor"));
			}
#endif
		}
	}
	
	void OnPostEngineInit()
	{
		SCOPED_WWISE_EVENT(TEXT("OnPostEngineInit"));
#if WITH_EDITOR
		{
			SCOPED_WWISE_EVENT(TEXT("OnPostEngineInit: AudiokineticTools"));
			UE_LOG(LogWwise, Log, TEXT("WwiseModule: Loading AudiokineticTools"));
			FModuleManager::Get().LoadModule(TEXT("AudiokineticTools"));
		}
#endif
	}

	virtual void ShutdownModule() override
	{
	}

#endif
};

IMPLEMENT_MODULE(FWwiseModule, Wwise);
