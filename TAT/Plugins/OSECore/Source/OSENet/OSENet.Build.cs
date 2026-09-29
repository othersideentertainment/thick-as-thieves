// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSENet : ModuleRules
{
   public OSENet(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bWarningsAsErrors = true;

      PrivateIncludePaths.Add("OSENet/Private");

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core", "CoreUObject", "Engine",                  // Core
            "Json", "JsonUtilities",                        // Json
            "HTTP",                                     // Net
         }
      );
   }
}
