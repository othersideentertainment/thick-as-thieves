// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using System.IO;
using UnrealBuildTool;

public class OSEXray : ModuleRules
{
   public OSEXray(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

      PublicIncludePaths.Add(Path.Combine(GetModuleDirectory("Renderer"), "Internal"));
      PublicIncludePaths.Add(Path.Combine(GetModuleDirectory("Renderer"), "Private"));

      PublicDependencyModuleNames.AddRange(new[]
      {
         "Core",
         "CoreUObject",
         "Engine",
      });

      PrivateDependencyModuleNames.AddRange(new[]
      {
         "Projects",
         "RenderCore",
         "Renderer",
         "RHI",
      });
   }
}
