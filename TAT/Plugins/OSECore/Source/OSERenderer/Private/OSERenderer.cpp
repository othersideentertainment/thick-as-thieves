// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSERenderer.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigUtilities.h"


DEFINE_LOG_CATEGORY(LogOSERenderer);

#define LOCTEXT_NAMESPACE "FOSERenderer"


//--------------------------------------------------------------------------------------------------
// Project specific renderer console variables.
// \see OSERenderSettings.h
//--------------------------------------------------------------------------------------------------

namespace CVarRender
{
   static TAutoConsoleVariable<int32> AllowCustomDataByDefault(
      TEXT("OSE.Render.AllowCustomDataByDefault"),
      0,
      TEXT("When enabled, custom data will be enabled for default lit materials.\n")
      TEXT("Changing this setting will require shaders to be recompiled.\n")
      TEXT(" 0: disabled (default)\n")
      TEXT(" 1: enabled"),
      ECVF_ReadOnly);
}

void FOSERenderer::StartupModule()
{
   // Apply renderer-specific project settings (see OSERenderSettings.h)
   UE::ConfigUtilities::ApplyCVarSettingsFromIni(TEXT("/Script/OSERenderer.OSERenderSettings"), *GEngineIni, ECVF_SetByProjectSetting);

   // Custom engine shaders need to be loaded early.
   // (this is why the module is loaded at PostConfigInit)
   {
      // All shaders are expected to live in the plugin "Shaders" directory
      TSharedPtr<IPlugin> corePlugin = IPluginManager::Get().FindPlugin(TEXT("OSECore"));
      check(corePlugin.IsValid());

      // Build a path to the "Shaders" directory
      const FString pluginDir = corePlugin->GetBaseDir();
      const FString pluginShaderDir = FPaths::Combine(pluginDir, TEXT("Shaders"));

      // Expected to always exist, but avoiding an assert before shaders are added to the plugin
      if (FPaths::DirectoryExists(pluginShaderDir))
      {
         // Virtual mapping to use in shader code
         AddShaderSourceDirectoryMapping(TEXT("/Plugin/OSECore"), pluginShaderDir);
      }
   }
}

void FOSERenderer::ShutdownModule()
{
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FOSERenderer, OSERenderer)
