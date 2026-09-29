// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


using UnrealBuildTool;

public class OSEPerceptionBT : ModuleRules
{
   public OSEPerceptionBT(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
      bUseUnity = false;
      OptimizeCode = CodeOptimization.Never;

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
            "Core",
            "AIModule",
            "GameplayTags",
				// ... add other public dependencies that you statically link with here ...
			}
         );


      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "GameplayTasks",
            "OSEPerception",
				// ... add private dependencies that you statically link with here ...	
			}
         );


      DynamicallyLoadedModuleNames.AddRange(
         new string[]
         {
				// ... add any modules that your module loads dynamically here ...
			}
         );

      if (Target.bBuildEditor == true)
      {
         PrivateDependencyModuleNames.Add("UnrealEd");
      }

      if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping))
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
