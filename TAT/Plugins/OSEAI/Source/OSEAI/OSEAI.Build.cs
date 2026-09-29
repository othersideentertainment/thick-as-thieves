// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSEAI : ModuleRules
{
	public OSEAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bWarningsAsErrors = true;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
            "Core", "CoreUObject", "Engine", "EngineSettings", "InputCore", "NetCore",       // Core
            "GameplayAbilities", "GameplayTasks", "GameplayTags",                            // Gameplay Ability System
            "Slate", "SlateCore", "UMG",                                                     // Slate UI
            "AIModule", "NavigationSystem", "SmartObjectsModule", "GameplayBehaviorsModule", // AI and Navigation
            "DeveloperSettings",                                                             // DeveloperSettings
            "OSECore",                                                                       // OSE
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			});
      
      if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
      {
         PrivateDependencyModuleNames.Add("GameplayDebugger");
         PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
      }
      else
      {
         PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
      }
	}
}
