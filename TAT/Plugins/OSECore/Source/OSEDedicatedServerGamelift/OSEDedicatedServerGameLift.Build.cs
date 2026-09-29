// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEDedicatedServerGameLift : ModuleRules
{
   public OSEDedicatedServerGameLift(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bWarningsAsErrors = true;

      PrivateIncludePaths.Add("OSEDedicatedServer/Private");

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core", "CoreUObject", "Engine",    // Core
            "OSECore", "OSEDedicatedServer",    // OSE
            "DeveloperSettings",                // Developer Settings
            "GameLiftServerSDK",                // GameLift
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
