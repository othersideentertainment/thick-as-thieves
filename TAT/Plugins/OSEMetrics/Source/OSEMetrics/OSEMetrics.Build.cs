// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEMetrics : ModuleRules
{
   public OSEMetrics(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

      //PrivateIncludePaths.Add("OSEDedicatedServer/Private");

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            // Core
            "Core",
            "CoreUObject",
            "Engine",
            "Json",
            "HTTP",
            "HTTPServer",
            "DeveloperSettings",
            // OSE
            "OSECore",
         }
      );

      // PrivateDependencyModuleNames.AddRange(
      //    new string[]
      //    {
      //       "Projects",
      //    }
      // );
   }
}
