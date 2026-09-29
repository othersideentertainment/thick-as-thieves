// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using AutomationTool;
using EpicGames.Core;
using Microsoft.Extensions.Logging;
using UnrealBuildBase;
using UnrealBuildTool;

[Help("Updates your TAT version with the extended build parameters")]
[RequireP4]
public class UpdateTATVersion : BuildCommand
{
	private string Shortname;
	private string ExtraTag;
	private string Product;
	private int BuildNumber;
	private string Shorthash;
	private string Discriminator;
	private int? MajorVersion;
	private int? MinorVersion;
	public override void ExecuteBuild()
	{
		UnrealBuild UnrealBuild = new UnrealBuild(this);
		int? ChangelistOverride = ParseParamNullableInt("cl");
		int? CompatibleChangelistOverride = ParseParamNullableInt("compatiblecl");
		string Build = ParseParamValue("Build", null);
		BuildNumber = ParseParamInt("BuildNumber", 0);
		Shorthash = ParseParamValue("Shorthash","deadbee");
		Shortname = ParseParamValue("Shortname",null);
		ExtraTag = ParseParamValue("ExtraTag", null);
		Discriminator = "";
		if (ParseParam("useShorthash"))
		{
			Discriminator += Shorthash;
		}
		else
		{
			Discriminator += BuildNumber.ToString();
		}
		if (ExtraTag != null)
		{
			Discriminator += $"_{ExtraTag}";
		}
		Product = ParseParamValue("Product",null);
		MajorVersion = ParseParamNullableInt("TATMajorVersion");
		MinorVersion = ParseParamNullableInt("TATMinorVersion");
		List<FileReference> updatedFiles = UnrealBuild.UpdateVersionFiles(ChangelistNumberOverride: ChangelistOverride, CompatibleChangelistNumberOverride: CompatibleChangelistOverride, Build: Build);
		// Read the updated file in
		string text = FileReference.ReadAllText(updatedFiles[0]);
		JsonObject engineVersion = JsonObject.Parse(text);
		// Write out the TAT file
		WriteTATVersionFile(engineVersion);
		// Read the TAT version file again
		text = FileReference.ReadAllText(tatVersionFileReference());
		JsonObject tatVersion = JsonObject.Parse(text);
		// Write out the BuildInfo.h for TAT.rc
		WriteBuildInfoForTATrc(tatVersion);
		// Rerun the above with the new build string
		Build = GetBuildVersion(engineVersion, tatVersion, Discriminator, forceCompat: false);
		UnrealBuild.UpdateVersionFiles(ChangelistNumberOverride: ChangelistOverride, CompatibleChangelistNumberOverride: CompatibleChangelistOverride, Build: Build);
	}
	
	string GetBuildVersion(JsonObject engineVersion, JsonObject tatVersion, string discriminator, bool forceCompat)
	{
		string build = $"{tatVersion.GetStringField("Product").ToLower()}-{tatVersion.GetIntegerField("MajorVersion")}.{tatVersion.GetIntegerField("MinorVersion")}.{engineVersion.GetIntegerField("Changelist")}";
		if (engineVersion.GetIntegerField("CompatibleChangelist") != 0 || forceCompat)
		{
			int compatCl;
			if (engineVersion.GetIntegerField("CompatibleChangelist") == 0)
			{
				compatCl = engineVersion.GetIntegerField("Changelist");
			}
			else
			{
				compatCl = engineVersion.GetIntegerField("CompatibleChangelist");
			}
			build += $"-c.{compatCl}";
		}
		build += $"-{tatVersion.GetStringField("Shortname").ToLower()}";
		build += $"-{discriminator}";
		
		return build;
	}
	
	string GetBuildVersionForTAT(JsonObject engineVersion, JsonObject tatVersion, string discriminator, bool forceCompat)
	{
		string product = Product ?? tatVersion.GetStringField("Product").ToLower();
		string shortName = Shortname ?? tatVersion.GetStringField("Shortname").ToLower();
		string build = $"{product}-{tatVersion.GetIntegerField("MajorVersion")}.{tatVersion.GetIntegerField("MinorVersion")}.{engineVersion.GetIntegerField("Changelist")}";
		if (engineVersion.GetIntegerField("CompatibleChangelist") != 0 || forceCompat)
		{
			int compatCl;
			if (engineVersion.GetIntegerField("CompatibleChangelist") == 0)
			{
				compatCl = engineVersion.GetIntegerField("Changelist");
			}
			else
			{
				compatCl = engineVersion.GetIntegerField("CompatibleChangelist");
			}
			build += $"-c.{compatCl}";
		}
		build += $"-{shortName}";
		build +=  $"-{discriminator}";

		return build;
	}
	
	FileReference tatVersionFileReference()
	{
		return FileReference.Combine(Unreal.EngineDirectory, "..", "TAT", "Plugins", "TATVersionV2", "Resources", "TAT.version");
	}

	JsonObject GetTATVersionsSource()
	{
		FileReference tatVersionReference = FileReference.Combine(Unreal.EngineDirectory, "..", "TAT", "Plugins", "TATVersionV2", "Resources", "TAT.version.source");
		string text = FileReference.ReadAllText(tatVersionReference);
		JsonObject tatVersion = JsonObject.Parse(text);
		Logger.LogInformation($"Loaded {tatVersionReference.FullName}");
		Logger.LogInformation($"MajorVersion {tatVersion.GetIntegerField("MajorVersion")}");
		Logger.LogInformation($"MinorVersion {tatVersion.GetIntegerField("MinorVersion")}");
		Logger.LogInformation($"Shortname {tatVersion.GetStringField("Shortname")}");
		Logger.LogInformation($"Product {tatVersion.GetStringField("Product")}");
		Logger.LogInformation($"Command Line");
		Logger.LogInformation($"MajorVersion {MajorVersion}");
		Logger.LogInformation($"MinorVersion {MinorVersion}");
		Logger.LogInformation($"Shortname {Shortname}");
		Logger.LogInformation($"Product {Product}");
		Logger.LogInformation($"Will write");
		Logger.LogInformation($"MajorVersion {MajorVersion ?? tatVersion.GetIntegerField("MajorVersion")}");
		Logger.LogInformation($"MinorVersion {MinorVersion ?? tatVersion.GetIntegerField("MinorVersion")}");
		Logger.LogInformation($"Shortname {Shortname ?? tatVersion.GetStringField("Shortname")}");
		Logger.LogInformation($"Product {Product ?? tatVersion.GetStringField("Product")}");
		return tatVersion;
	}

	void WriteTATVersionFile(JsonObject engineVersion)
	{
		FileReference tatVersionReference = FileReference.Combine(Unreal.EngineDirectory, "..", "TAT", "Plugins", "TATVersionV2", "Resources", "TAT.version");
		using (StreamWriter Writer = new StreamWriter(tatVersionReference.FullName))
		{
			using (JsonWriter OtherWriter = new JsonWriter(Writer))
			{
				OtherWriter.WriteObjectStart();
				WriteProperties(OtherWriter, engineVersion, GetTATVersionsSource());
				OtherWriter.WriteObjectEnd();
			}
		}
	}

	void WriteProperties(JsonWriter Writer, JsonObject engineVersion, JsonObject tatVersion)
	{
		Writer.WriteValue("MajorVersion", MajorVersion ?? tatVersion.GetIntegerField("MajorVersion"));
		Writer.WriteValue("MinorVersion", MinorVersion ?? tatVersion.GetIntegerField("MinorVersion"));
		Writer.WriteValue("Shortname", Shortname ?? tatVersion.GetStringField("Shortname"));
		Writer.WriteValue("Product", Product ?? tatVersion.GetStringField("Product"));
		Writer.WriteValue("BuildNumber", BuildNumber);
		Writer.WriteValue("Shorthash", Shorthash);
		Writer.WriteValue("Discriminator", Discriminator);
		Writer.WriteValue("VcsNumber", engineVersion.GetIntegerField("Changelist"));
		Writer.WriteValue("VcsBranch", engineVersion.GetStringField("BranchName").Replace("+", "/"));
		Writer.WriteValue("BuildVersion", GetBuildVersionForTAT(engineVersion, tatVersion, Discriminator, false));
		Writer.WriteValue("BuildVersionWithCompat", GetBuildVersionForTAT(engineVersion, tatVersion, Discriminator, true));
	}

	void WriteBuildInfoForTATrc(JsonObject tatVersion)
	{
		FileReference buildInfoReference = FileReference.Combine(Unreal.EngineDirectory, "..", "TAT", "Source", "TAT", "Resources", "BuildInfo.h");
		if (File.Exists(buildInfoReference.FullName))
		{
			VersionFileUpdater.MakeFileWriteable(buildInfoReference.FullName, true);
		}

		using (StreamWriter Writer = new StreamWriter(buildInfoReference.FullName))
		{
			Writer.WriteLine("// This file is written by the build machine");
			Writer.WriteLine("#pragma once");
			Writer.WriteLine($"#define TAT_BUILD_VERSION_MAJOR   {tatVersion.GetIntegerField("MajorVersion")}");
			Writer.WriteLine($"#define TAT_BUILD_VERSION_MINOR   {tatVersion.GetIntegerField("MinorVersion")}");
			Writer.WriteLine($"#define TAT_BUILD_NUMBER          {tatVersion.GetIntegerField("BuildNumber")}");
			Writer.WriteLine($"#define TAT_BUILD_VCS_NUMBER      {tatVersion.GetIntegerField("VcsNumber")}");
			Writer.WriteLine($"#define TAT_BUILD_VCS_BRANCH      \"{tatVersion.GetStringField("VcsBranch")}\"");
			Writer.WriteLine("#define TAT_BUILD_DATE            __DATE__");
		}
	}
}
