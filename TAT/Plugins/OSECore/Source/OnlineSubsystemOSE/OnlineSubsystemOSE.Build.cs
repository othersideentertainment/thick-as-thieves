// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

using System.Linq;

public class OnlineSubsystemOSE : ModuleRules
{
   public OnlineSubsystemOSE(ReadOnlyTargetRules Target) : base(Target)
   {
      PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

      PrivateIncludePaths.Add("OnlineSubsystemOSE/Private");

      PublicDependencyModuleNames.AddRange(
         new string[]
         {
            "OSECore",                                                           // OSE
            "Core", "CoreUObject", "Engine",                                     // Core
            "OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemUtils",    // Online
            "Sockets",                                                           // Networking
            "OnlineSubsystemSteam",                                              // Steam
         }
      );
   }
}
