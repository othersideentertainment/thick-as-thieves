// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class TATEditor : ModuleRules
{
   public TATEditor(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bEnableNonInlinedGenCppWarnings = true;
      bWarningsAsErrors = true;

      PrivateIncludePaths.Add("TATEditor/Private");

      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            "Core",
            "CoreUObject",
            "DataValidation",
            "Engine",
            "GameplayTags",
            "OSECore",
            "OSECoreEditor",
            "OSEAI",
            "OSEGenericGraphRuntime",
            "OSEGenericGraphEditor",
            "OSEInteraction",
            "Slate",
            "SlateCore",
            "InputCore",
            "TAT",
            "UnrealEd",
            "DetailCustomizations",
            "EditorStyle",
            "EditorSubsystem",
            "Layers",
            "PropertyEditor",
            "StructUtilsEditor",
            "SubobjectEditor",
            "SubobjectDataInterface",
            "DeveloperSettings",
            "DeveloperToolSettings",
            "AnimationModifiers",
            "AnimationBlueprintLibrary",
            "AkAudio",
            "CableComponent",
            "WorkspaceMenuStructure",
            "ContentBrowser",
            "Niagara",
            "AssetTools",
            "AssetDefinition",
            "EngineAssetDefinitions",
            "NavigationSystem",
            "Blutility",
            "ToolMenus",
            "ContentBrowser",
            "StringTableEditor",
         }
      );
   }
}
