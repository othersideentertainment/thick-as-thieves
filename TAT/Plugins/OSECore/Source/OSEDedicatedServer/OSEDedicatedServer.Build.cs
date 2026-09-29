// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEDedicatedServer : ModuleRules
{
   public OSEDedicatedServer(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bWarningsAsErrors = true;

      PrivateIncludePaths.Add("OSEDedicatedServer/Private");

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core", "CoreUObject", "Engine",    // Core
            "OSECore",                          // OSE
            "DeveloperSettings",                // Developer Settings
            "Sockets",                          // Networking
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
