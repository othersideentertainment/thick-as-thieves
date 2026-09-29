// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using System;
using EpicGames.Core;
using UnrealBuildTool;

public class TATEnvironmentConfig : ModuleRules
{
   public TATEnvironmentConfig(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
      
      string defaultEnvironmentName = Environment.GetEnvironmentVariable("TAT_DEFAULT_ENVIRONMENT");
      if (!string.IsNullOrEmpty(defaultEnvironmentName))
      {
         Log.TraceInformationOnce("Using Default Environment override = {0}", defaultEnvironmentName);
         PrivateDefinitions.AddRange(new[]
         {
            $"TAT_DEFAULT_ENVIRONMENT=\"{defaultEnvironmentName}\"",
         });
      }
      

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
            // ... add other public dependencies that you statically link with here ...
         }
         );
         
      
      PrivateDependencyModuleNames.AddRange(
         new string[]
         {
            "CoreUObject",
            "Engine",
         }
         );
      
      
      DynamicallyLoadedModuleNames.AddRange(
         new string[]
         {
            // ... add any modules that your module loads dynamically here ...
         }
         );
   }
}
