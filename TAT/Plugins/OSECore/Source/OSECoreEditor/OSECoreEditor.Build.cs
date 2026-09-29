// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSECoreEditor : OSECore
{
   public OSECoreEditor(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bWarningsAsErrors = true;

      PrivateIncludePaths.Add("OSECoreEditor/Private");

      PublicDependencyModuleNames.Add("OSECore");

      // Editor specific
      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            "ApplicationCore",
            "AnimGraph",
            "AnimationModifiers",
            "AnimationBlueprintLibrary",
            "DeveloperToolSettings",
            "BlueprintGraph",
            "DataValidation",
            "DesktopPlatform",
            "EditorStyle",
            "EditorFramework",
            "EnvironmentQueryEditor",
            "GraphEditor",
            "Projects",
            "PropertyEditor",
            "Settings",
            "UnrealEd",
            "ToolMenus",
            "Slate",
            "SlateCore",
            "AppFramework",
            "SourceControl",
            "WwiseSoundEngine",
            "WwiseFileHandler",
         }
      );
   }
}
