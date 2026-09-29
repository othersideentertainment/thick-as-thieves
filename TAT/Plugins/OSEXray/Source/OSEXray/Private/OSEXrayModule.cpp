// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// UE
#include <ShaderCore.h>
#include <Interfaces/IPluginManager.h>
#include <Modules/ModuleManager.h>

class FOSEXrayModule : public IModuleInterface
{
public:
   virtual void StartupModule() override
   {
      const FString shaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(UE_PLUGIN_NAME)->GetBaseDir(), TEXT("Shaders"));
      AddShaderSourceDirectoryMapping(TEXT("/OSEXray"), shaderDir);
   }
};

IMPLEMENT_MODULE(FOSEXrayModule, OSEXray);
