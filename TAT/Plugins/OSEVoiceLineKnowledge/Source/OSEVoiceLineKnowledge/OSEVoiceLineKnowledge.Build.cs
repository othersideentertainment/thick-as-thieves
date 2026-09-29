// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEVoiceLineKnowledge : ModuleRules
{
	public OSEVoiceLineKnowledge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bWarningsAsErrors = true;
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
            "OSEIndividualKnowledge",
			});
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
            "GameplayTags", 
            "OSEIndividualKnowledge",
			});
	}
}
