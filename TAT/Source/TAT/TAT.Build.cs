// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using System;
using UnrealBuildTool;

public class TAT : ModuleRules
{
   public TAT(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
      bEnableNonInlinedGenCppWarnings = true;
      bWarningsAsErrors = true;

      SetupIrisSupport(Target);

      PublicDependencyModuleNames.AddRange(new[]
      {
         // OSE
         "OSEAI",
         "OSECore",
         "OSEDedicatedServer",
         "OSEGenericGraphRuntime",
         "OSEIndividualAttitudes",
         "OSEIndividualKnowledge",
         "OSEInteraction",
         "OSEItem", // TO delete once gone
         "OSELightDetection",
         "OSEMetrics",
         "OSENet",
         "OSEScheduler",
         "OSEVoiceLineKnowledge",
         "OSEXray",
         "OnlineSubsystemOSE",
         // Wwise
         "AkAudio",
         "WwiseLowLevelUtils",
         "WwiseSoundEngine",
         // UE
         "AIModule",
         "AssetRegistry",
         "CommonUI",
         "CommonInput",
         "Core",
         "CoreUObject",
         "DeveloperSettings",
         "Engine",
         "EnhancedInput",
         "GameplayAbilities",
         "GameplayBehaviorSmartObjectsModule",
         "GameplayBehaviorsModule",
         "GameplayStateTreeModule",
         "GameplayTags",
         "GameplayTasks",
         "HTTP",
         "ImageCore",
         "InputCore",
         "Json",
         "JsonUtilities",
         "LevelSequence",
         "MediaAssets",
         "MotionWarping",
         "MoviePlayer",
         "MovieScene",
         "NavigationSystem",
         "NetCore",
         "Niagara",
         "OnlineSubsystem",
         "OnlineSubsystemUtils",
         "Paper2D",
         "PhysicsCore",
         "ProceduralMeshComponent",
         "SignificanceManager",
         "Slate",
         "SlateCore",
         "SmartObjectsModule",
         "StateTreeModule",
         "TraceLog",
         "UMG",
         "WorldConditions",
      });

      PrivateDependencyModuleNames.AddRange(new[]
      {
         "ApplicationCore",
         "ContextualAnimation",
         "CoreOnline",
         "RenderCore",
         "Steamworks",
         "TATEnvironmentConfig",
      });

      if (Target.bBuildEditor)
      {
         PublicDependencyModuleNames.AddRange(new[]
         {
            // OSE
            "OSECoreEditor",
            // UE
            "UnrealEd",
            "MessageLog",
         });
      }

      if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
      {
         PublicDependencyModuleNames.Add("ImGui");
         PublicDefinitions.Add("TAT_ENABLE_DEV_TOOLS=1");

         PrivateDependencyModuleNames.Add("GameplayDebugger");
         PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
      }
      else
      {
         PublicDefinitions.Add("TAT_ENABLE_DEV_TOOLS=0");
         PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
      }

      // Compile-time build tweaks

      // Enabled/disable screen messages in important builds
      PrivateDefinitions.Add("TAT_DISABLE_SCREEN_MESSAGES=0");
      
      // Handle migration to the new TAT version
      string versionEditionIn = Environment.GetEnvironmentVariable("TAT_VERSION_EDITION") ?? "1";
      int versionEdition;
      if (!Int32.TryParse(versionEditionIn, out versionEdition))
      {
         versionEdition = 1;
      }
      PrivateDefinitions.Add($"TAT_VERSION_EDITION={versionEdition}");
      if (versionEdition == 2)
      {
         PrivateDependencyModuleNames.AddRange(new[]
         {
            "TATVersionV2",
         });
      }
   }
}
