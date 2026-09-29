// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEIndividualAttitudesDebug : ModuleRules
{
    public OSEIndividualAttitudesDebug(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "GameplayDebugger",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "OSEIndividualAttitudes",
            }
        );
    }
}
