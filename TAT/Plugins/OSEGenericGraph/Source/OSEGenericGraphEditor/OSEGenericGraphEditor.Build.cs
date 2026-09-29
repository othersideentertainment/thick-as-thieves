// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEGenericGraphEditor : ModuleRules
{
   public OSEGenericGraphEditor(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bLegacyPublicIncludePaths = false;
      ShadowVariableWarningLevel = WarningLevel.Error;
      bWarningsAsErrors = true;

      PublicIncludePaths.AddRange(
         new string[]
         {
            // ... add public include paths required here ...
         }
      );

      PrivateIncludePaths.AddRange(
         new string[]
         {
            // ... add other private include paths required here ...
            "OSEGenericGraphEditor/Private",
            "OSEGenericGraphEditor/Public",
         }
      );

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            // ... add other public dependencies that you statically link with here ...
         }
      );

      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            "OSEGenericGraphRuntime",
            "AssetTools",
            "Slate",
            "InputCore",
            "SlateCore",
            "GraphEditor",
            "PropertyEditor",
            "EditorStyle",
            "Kismet",
            "KismetWidgets",
            "ApplicationCore",
            "ToolMenus",
            // ... add private dependencies that you statically link with here ...
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
