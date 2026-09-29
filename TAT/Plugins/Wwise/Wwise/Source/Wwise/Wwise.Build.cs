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

using System.IO;
using UnrealBuildTool;

public class Wwise : ModuleRules
{
	public Wwise(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		WwiseHelper.AddDependencies(this, Target);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"WwiseAudioLinkRuntime",
				"WwiseLowLevelUtils",
				"WwiseUtils",
			}
		);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"AudiokineticTools",
					"UnrealEd",
					"WwiseAudioLinkEditor",
				}
			);
		}

#if UE_5_3_OR_LATER
		bLegacyParentIncludePaths = false;
		CppStandard = CppStandardVersion.Default;
#endif
	}

	public void AddOptionalModule(string Module, bool AddPublic = true)
	{
		ConditionalAddModuleDirectory(
			EpicGames.Core.DirectoryReference.Combine(new EpicGames.Core.DirectoryReference(ModuleDirectory),  "..", Module));
		ExternalDependencies.Add(Path.Combine(ModuleDirectory, "..", Module, Module + "_OptionalModule.Build.cs"));
		if (AddPublic)
		{
			PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "..", Module, "Public"));
		}
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", Module, "Private"));
	}
}
