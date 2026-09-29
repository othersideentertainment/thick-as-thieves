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

public class WwiseLowLevelUtils : ModuleRules
{
	public WwiseLowLevelUtils(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
		});
		
		PublicDefinitions.AddRange(new string[]
		{
			"WWISE_INTEGRATION_VERSION_MAJOR=2024",	
			"WWISE_INTEGRATION_VERSION_MINOR=1",
			"WWISE_INTEGRATION_VERSION=\"2024.1\"",
			"WWISE_INTEGRATION_2024_1=1",	

			"WWISE_INTEGRATION_2022_1_OR_LATER=1",	
			"WWISE_INTEGRATION_2023_1_OR_LATER=1",	
			"WWISE_INTEGRATION_2024_1_OR_LATER=1",	
			"WWISE_INTEGRATION_2025_1_OR_LATER=0",	
		});


#if UE_5_3_OR_LATER
		bLegacyParentIncludePaths = false;
		CppStandard = CppStandardVersion.Default;
#endif
	}
}
