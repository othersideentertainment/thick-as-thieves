// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEIndividualAttitudes : ModuleRules
{
	public OSEIndividualAttitudes(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bWarningsAsErrors = true;
		PublicDependencyModuleNames.AddRange(new string[]
		{
		   "Core",
         "NetCore",
		});
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
         "GameplayDebugger"
		});
	}
}
