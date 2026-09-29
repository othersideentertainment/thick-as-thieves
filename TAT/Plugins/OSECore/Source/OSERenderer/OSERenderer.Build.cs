// (c) 2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSERenderer : ModuleRules
{
   public OSERenderer(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bWarningsAsErrors = true;

      PrivateIncludePaths.Add("OSERenderer/Private");

      // (4.26) Private engine directories need to be included for full RDG access
      PrivateIncludePaths.AddRange(
         new string[]
         {
            EngineDirectory + "/Source/Runtime/Renderer/Private",
            EngineDirectory + "/Source/Runtime/RHI/Private",
         }
      );

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core", "CoreUObject", "Engine",       // Core
            "RenderCore", "Renderer", "RHI",       // Rendering
            "DeveloperSettings",                   // DeveloperSettings
            "Niagara", "NiagaraCore", "VectorVM",  // Niagara; required for data interfaces and spawning systems from code
            "CableComponent",                      // CableComponent
         }
      );

      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            "Projects",
         }
      );
   }
}
