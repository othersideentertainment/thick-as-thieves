// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEGenericGraphRuntime : ModuleRules
{
   public OSEGenericGraphRuntime(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bLegacyPublicIncludePaths = false;
      ShadowVariableWarningLevel = WarningLevel.Error;
      bWarningsAsErrors = true;

      PublicIncludePaths.AddRange(
         new string[] {
            // ... add public include paths required here ...
         }
      );

      PrivateIncludePaths.AddRange(
         new string[] {
            "OSEGenericGraphRuntime/Private",
            // ... add other private include paths required here ...
         }
      );

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core",
            "CoreUObject",
            "Engine",
            // ... add other public dependencies that you statically link with here ...
         }
      );

      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            // ... add private dependencies that you statically link with here ...
            "Slate",
            "SlateCore",
            "GameplayTags"
         }
      );

      DynamicallyLoadedModuleNames.AddRange(
         new string[]
         {
            // ... add any modules that your module loads dynamically here ...
         }
      );
   }
}
