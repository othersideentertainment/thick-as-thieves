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

#include "Wwise/WwisePackagingModuleImpl.h"

#include "Wwise/Packaging/WwisePackagingSettings.h"

#if WITH_EDITOR
#include "ISettingsModule.h"
#endif

#define LOCTEXT_NAMESPACE "WwisePackaging"

IMPLEMENT_MODULE(FWwisePackagingModule, WwisePackaging)

FWwisePackagingModule::FWwisePackagingModule()
#if WITH_EDITORONLY_DATA 
:
	CreateResourceCookerForPlatformFct {
		[](const ITargetPlatform*)
		{
			check(false);
		}
	}
#endif
{
}

void FWwisePackagingModule::StartupModule()
{
	IWwisePackagingModule::StartupModule();

#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Project", "Wwise", "Wwise Packaging",
			LOCTEXT("RuntimeSettingsName", "Wwise Packaging"),
			LOCTEXT("RuntimeSettingsDescription", "Fine tune how your Wwise project is packaged for release."),
			GetMutableDefault<UWwisePackagingSettings>());
	}
#endif
}

void FWwisePackagingModule::ShutdownModule()
{
	IWwisePackagingModule::ShutdownModule();

#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Wwise", "Wwise Packaging");
	}
#endif
}

#if WITH_EDITORONLY_DATA 
void FWwisePackagingModule::DoCreateResourceCookerForPlatform(const ITargetPlatform* TargetPlatform) const
{
	CreateResourceCookerForPlatformFct(TargetPlatform);
}

void FWwisePackagingModule::SetCreateResourceCookerForPlatformFct(const TFunction<void(const ITargetPlatform* TargetPlatform)>& Func)
{
	CreateResourceCookerForPlatformFct = Func;
}
#endif

#undef LOCTEXT_NAMESPACE