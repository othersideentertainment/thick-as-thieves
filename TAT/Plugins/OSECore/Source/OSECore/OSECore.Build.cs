// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class OSECore : ModuleRules
{
   public OSECore(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bEnableNonInlinedGenCppWarnings = true;
      bWarningsAsErrors = true;

      SetupIrisSupport(Target);

      PrivateIncludePaths.Add("OSECore/Private");

      // OSE module dependencies
      PublicDependencyModuleNames.Add("OSERenderer");

      // UE4 module dependencies
      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "Core", "CoreUObject", "Engine", "EngineSettings", "InputCore", "NetCore",       // Core
            "ApplicationCore", "RHI",                                                        // Core (continued)
            "AnimGraphRuntime",                                                              // Animation graph
            "GameplayAbilities", "GameplayTasks", "GameplayTags",                            // Gameplay Ability System
            "AIModule", "NavigationSystem", "SmartObjectsModule", "GameplayBehaviorsModule", // AI and Navigation
            "Slate", "SlateCore", "UMG",                                                     // Slate UI
            "EnhancedInput",                                                                 // Input
            "OnlineSubsystem",                                                               // Online
            "CinematicCamera",                                                               // Camera
            "Paper2D",                                                                       // Paper2D
            "DeveloperSettings",                                                             // DeveloperSettings
            "PhysicsCore",                                                                   // Physics
            "AkAudio",                                                                       // Audio
            "OnlineSubsystemUtils",                                                          // OnlineSubsystemUtils
            "WwiseFileHandler", "WwiseUtils", // Wwise (for external sources)
            "CommonInput",
            "CommonUI",
         }
      );

      PrivateDependencyModuleNames.AddRange(
       new string[]
       {
            "MotionWarping"
       });

      if (Target.bBuildEditor == true)
      {
         // Editor specific
         PrivateDependencyModuleNames.Add("UnrealEd");
         PrivateDependencyModuleNames.Add("EnvironmentQueryEditor");
         PrivateDependencyModuleNames.Add("Persona"); //Provides access to the animation preview actor.

         PrivateDependencyModuleNames.Add("WwiseSoundEngine"); // for vo line validation
      }

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
