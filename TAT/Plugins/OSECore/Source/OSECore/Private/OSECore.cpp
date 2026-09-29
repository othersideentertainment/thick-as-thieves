// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSECore.h"

// ose
#include "Player/OSEPlayerStatsDebugVis.h"

// ue
#include "GameFramework/HUD.h"

// Settings
#if WITH_EDITOR
#   include "ISettingsModule.h"
#   include "ISettingsSection.h"
#   include "ISettingsContainer.h"
#endif

DEFINE_LOG_CATEGORY(LogOSECore);

#define LOCTEXT_NAMESPACE "FOSECore"


void FOSECore::StartupModule()
{
#if WITH_EDITOR
   _RegisterSettings();
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
   // NB: current uses in engine don't seem to unregister this, so doing the same
   AHUD::OnShowDebugInfo.AddStatic(&PlayerStatsDebugVis::OnShowDebugInfo);
#endif
}

void FOSECore::ShutdownModule()
{
#if WITH_EDITOR
   if (UObjectInitialized())
   {
      _UnRegisterSettings();
   }
#endif
}

#if WITH_EDITOR

// Callback for when the settings were saved.
bool FOSECore::_HandleSettingsSaved()
{
   return true;
}

void FOSECore::_RegisterSettings()
{
   if (ISettingsModule* settingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
   {
      ISettingsContainerPtr settingsContainer = settingsModule->GetContainer("Project");
   }
}

void FOSECore::_UnRegisterSettings()
{
   if (ISettingsModule* settingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
   {
      settingsModule->UnregisterSettings("Project", "OSEAIConfigSettings", "General");
   }
}

#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOSECore, OSECore)
