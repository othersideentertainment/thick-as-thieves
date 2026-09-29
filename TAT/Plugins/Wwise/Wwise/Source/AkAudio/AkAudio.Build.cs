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
using System;
using System.IO;
using System.Collections.Generic;
using System.Reflection;

public class AkAudio : ModuleRules
{
	public AkAudio(ReadOnlyTargetRules Target) : base(Target)
	{
		// If packaging as an Engine plugin, the UBT expects to already have a precompiled plugin available
		// This can be set to true so long as plugin was already precompiled
		bUsePrecompiled = false;
		bPrecompile = false;

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bAllowConfidentialPlatformDefines = true;

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"Settings",
				"UMG",
				"TargetPlatform"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"UMG",
				"WwisePackaging",
				"WwiseResourceLoader",
				"WwiseObstructionOcclusion",
				"WwiseProcessing",
				"WwiseUtils"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",

				"AudioMixer",
				"Chaos",
				"HeadMountedDisplay",
				"InputCore",
				"Json",
				"MovieScene",
				"MovieSceneTracks",
				"NetworkReplayStreaming",
				"PhysicsCore",
				"Projects",
				"Slate",
				"SlateCore",
				"XmlParser",

				"WwiseConcurrency",
				"WwiseFileHandler",
				"WwiseLowLevelUtils",
				"WwiseSoundEngine",
				"WwiseUtils",
			}
		);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				}
			);
			
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"DesktopPlatform",
					"DeveloperToolSettings",
					"EditorStyle",

					"GeometryMode",
					"RenderCore",
					"SettingsEditor",
					"SharedSettingsWidgets",
					"SourceControl",
					"TargetPlatform",
					"UnrealEd",

					"WwiseAuthoring",
					"WwiseEditorUtils",
				}
			);
		}

		if (Target.bBuildWithEditorOnlyData)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"WwisePackagingEditor",
					"WwiseProjectDatabase"
				}
			);
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"WwiseResourceCooker"
				}
			);
		}
		
		if (Target.bCompileAgainstCoreUObject)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
				"WwiseObjectUtils"
			});
		}
		
		if (Target.bCompileAgainstEngine)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
				"WwiseEngineUtils"
			});
		}

		PrivateIncludePaths.Add("AkAudio/Private");
		PrivateIncludePaths.Add("AkAudio/Classes/GTE");

		#if UE_5_3_OR_LATER
			bLegacyParentIncludePaths = false;
			CppStandard = CppStandardVersion.Default;
		#endif
	}
}
