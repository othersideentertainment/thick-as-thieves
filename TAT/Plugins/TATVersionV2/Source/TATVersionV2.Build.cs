// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

using System;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildTool;

public class TATVersionV2 : ModuleRules
{
	public TATVersionV2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"../Resources"
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
				"Slate",
				"SlateCore",
            "BuildSettings"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
      
      FileReference JsonPath = FileReference.Combine(new DirectoryReference(PluginDirectory), "Resources", "TAT.version");
      string text = FileReference.ReadAllText(JsonPath);
      JsonObject Object = JsonObject.Parse(text);
      
      PrivateDefinitions.AddRange(new[]
      {
         $"TAT_BUILD_VERSION_MAJOR={Object.GetIntegerField("MajorVersion")}",
         $"TAT_BUILD_VERSION_MINOR={Object.GetIntegerField("MinorVersion")}",
         $"TAT_BUILD_NUMBER={Object.GetIntegerField("BuildNumber")}",
         $"TAT_BUILD_VCS_NUMBER={Object.GetIntegerField("VcsNumber")}",
         $"TAT_BUILD_VCS_BRANCH=\"{Object.GetStringField("VcsBranch")}\"",
         $"TAT_BUILD_PRODUCT_NAME=\"{Object.GetStringField("Product")}\"",
         $"TAT_BUILD_SHORT_NAME=\"{Object.GetStringField("Shortname")}\"",
         $"TAT_BUILD_SHORT_HASH=\"{Object.GetStringField("Shorthash")}\"",
         $"TAT_BUILD_DISCRIMINATOR=\"{Object.GetStringField("Discriminator")}\"",
      });
	}
}
