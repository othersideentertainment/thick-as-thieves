// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Online Documentation: https://docs.unrealengine.com/en-US/Programming/BuildTools/UnrealBuildTool/TargetFiles/index.html
//

using System;
using EpicGames.Core;
using UnrealBuildTool;

public class TATTarget : TargetRules
{
   [ConfigFile(ConfigHierarchyType.Engine, "OnlineSubsystemSteam", "SteamDevAppId")]
   private string SteamAppId = string.Empty;

   public TATTarget(TargetInfo Target) : base(Target)
   {
      DefaultBuildSettings = BuildSettingsVersion.V5;
      IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

      Type = TargetType.Game;

      // Force "Include What You Use" for all modules. The docs indicate that 
      // it is not on or enforced by default for game modules.
      //bIWYU = true;
      bEnforceIWYU = true;

      // Uncomment these lines to find header include issues, it disables unity and PCH which is slow but will shake out any issues
      // bUseUnityBuild = false;
      // bUsePCHFiles = false;

      bWithPushModel = true;

      string steamAppIdOverride = Environment.GetEnvironmentVariable("UE_PROJECT_STEAMSHIPPINGID");
      if (!string.IsNullOrEmpty(steamAppIdOverride))
      {
         Log.TraceInformationOnce("Using Steam App ID override = {0}", steamAppIdOverride);
         SteamAppId = steamAppIdOverride;
      }

      GlobalDefinitions.AddRange(new[]
      {
         "UE_PROJECT_STEAMSHIPPINGID=" + SteamAppId,
         "UE_PROJECT_STEAMPRODUCTNAME=\"TAT\"",
         "UE_PROJECT_STEAMGAMEDIR=\"TAT\"",
         "UE_PROJECT_STEAMGAMEDESC=\"Thick as Thieves\"",
      });

      ExtraModuleNames.AddRange(new[]
      {
         "OSECore",
         "OSEAI",
         "OSEIndividualAttitudes",
         "TAT"
      });
   }
}
