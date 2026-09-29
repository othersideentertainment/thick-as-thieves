/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

using UnrealBuildTool;

public class AudiokineticTools : ModuleRules
{
	public AudiokineticTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add("AudiokineticTools/Private");
		PrivateIncludePaths.Add("AkAudio/Classes/GTE");
		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"TargetPlatform",
				"MainFrame",
				"MovieSceneTools"
			}
		);

		PublicIncludePathModuleNames.AddRange(
			new string[] 
			{ 
				"AssetTools",
				"ContentBrowser",
				"ToolMenus"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{ 
				"Core",
				"CoreUObject",
				"Engine",

				"ContentBrowser",
				"ContentBrowserData",
				"DesktopPlatform",
				"DesktopWidgets",
				"DeveloperToolSettings",
				"DirectoryWatcher",
				"EditorFramework",
				"EditorStyle",
				"InputCore",
				"Json",
				"LevelEditor",
				"MovieScene",
				"MovieSceneTools",
				"MovieSceneTracks",
				"Projects",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"Sequencer",
				"SequencerCore",
				"SharedSettingsWidgets",
				"Slate",
				"SlateCore",
				"SourceControl",
				"ToolMenus",
				"UnrealEd",
				"WorkspaceMenuStructure",
				"XmlParser",

				"AkAudio",
				"WwiseAuthoring",
				"WwiseEditorUtils",
				"WwiseLowLevelUtils",
				"WwiseProjectDatabase",
				"WwiseResourceLoader",
				"WwiseUtils",
			}
		);

		if (Target.bBuildWithEditorOnlyData)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"WwiseReconcile",
				}
			);
		}

#if UE_5_3_OR_LATER
		bLegacyParentIncludePaths = false;
		CppStandard = CppStandardVersion.Default;
#endif
	}
}
