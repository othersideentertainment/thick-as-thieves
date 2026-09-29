// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using System;
using EpicGames.Core;
using UnrealBuildTool;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class TATServerTarget : TargetRules
{
   [ConfigFile(ConfigHierarchyType.Engine, "OnlineSubsystemSteam", "SteamDevAppId")]
   private string SteamAppId = string.Empty;

   public TATServerTarget(TargetInfo Target) : base(Target)
   {
      DefaultBuildSettings = BuildSettingsVersion.V5;
      IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

      Type = TargetType.Server;

      // Force "Include What You Use" for all modules. The docs indicate that 
      // it is not on or enforced by default for game modules.
      //bIWYU = true;
      bEnforceIWYU = true;

      // Enable logs for shipping builds on the server
      bUseLoggingInShipping = true;

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
         "TAT"
      });
   }
}
