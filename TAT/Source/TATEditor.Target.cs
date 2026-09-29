// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class TATEditorTarget : TargetRules
{
   public TATEditorTarget(TargetInfo Target) : base(Target)
   {
      DefaultBuildSettings = BuildSettingsVersion.V5;
      IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

      Type = TargetType.Editor;

      // Force "Include What You Use" for all modules. The docs indicate that 
      // it is not on or enforced by default for game modules.
      //bIWYU = true;
      bEnforceIWYU = true;

      // Uncomment these lines to find header include issues, it disables unity and PCH which is slow but will shake out any issues
      // bUseUnityBuild = false;
      // bUsePCHFiles = false;

      bWithPushModel = true;

      ExtraModuleNames.AddRange(new[]
      {
         "OSECore",
         "OSECoreEditor",
         "OSEAI",
         "TAT",
         "TATEditor"
      });
   }
}
