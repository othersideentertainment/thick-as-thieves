/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal

License Usage

Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

using UnrealBuildTool;
using System;
using System.IO;
using System.Linq;

public class WwiseSoundEngineVersion
{
	public int Major = -1;
	public int Minor = -1;
	public int SubMinor = -1;
	public int Build = -1;
	private static WwiseSoundEngineVersion Instance = null;

	private WwiseSoundEngineVersion(string PluginDirectory)
	{
		var WwiseSdkVersionPath = Path.Combine(PluginDirectory, "ThirdParty/include/AK/", "AkWwiseSDKVersion.h");
		if (!System.IO.File.Exists(WwiseSdkVersionPath))
		{
			throw new BuildException(string.Format("Wwise ThirdParty is not installed. Unable to find \"{0}\" file.", WwiseSdkVersionPath));
		}

		var WwiseSdkVersionLines = System.IO.File.ReadAllLines(WwiseSdkVersionPath);
		foreach (var Line in WwiseSdkVersionLines)
		{
			try
			{
				if (Line.StartsWith("#define AK_WWISESDK_VERSION_MAJOR"))
				{
					Major = int.Parse(Line.Split('\t').Last());
				}
				else if (Line.StartsWith("#define AK_WWISESDK_VERSION_MINOR"))
				{
					Minor = int.Parse(Line.Split('\t').Last());
				}
				else if (Line.StartsWith("#define AK_WWISESDK_VERSION_SUBMINOR"))
				{
					SubMinor = int.Parse(Line.Split('\t').Last());
				}
				else if (Line.StartsWith("#define AK_WWISESDK_VERSION_BUILD"))
				{
					Build = int.Parse(Line.Split('\t').Last());
				}
			}
			catch (Exception)
			{
				throw new BuildException(string.Format("Invalid numeral at end of line \"{}\"", Line));
			}
		}

		if (Major == -1 || Minor == -1 || SubMinor == -1 || Build == -1)
		{
			throw new BuildException(string.Format("Could not find full Wwise version in \"{0}\" file.", WwiseSdkVersionPath));
		}
	}

	public static WwiseSoundEngineVersion GetInstance(string PluginDirectory)
	{
		return Instance != null ? Instance : Instance = new WwiseSoundEngineVersion(PluginDirectory);
	}

    public static string[] GetVersionDefinesFromPluginDirectory(string PluginDirectory)
	{
		WwiseSoundEngineVersion version = GetInstance(PluginDirectory);
		return new[]
		{
			string.Format("AK_WWISE_SOUNDENGINE_VERSION=\"{0}.{1}.{2}\"", version.Major, version.Minor, version.SubMinor),
            string.Format("AK_WWISE_SOUNDENGINE_{0}_{1}", version.Major, version.Minor),
            string.Format("AK_WWISE_SOUNDENGINE_MAJOR_VERSION={0}", version.Major),
            string.Format("AK_WWISE_SOUNDENGINE_MINOR_VERSION={0}", version.Minor),
            string.Format("AK_WWISE_SOUNDENGINE_SUBMINOR_VERSION={0}", version.SubMinor)
		};
	}

	public static bool IsSoundEngineVersionSupported(string PluginDirectory, string ClassName)
	{
		var ClassSplit = ClassName.Split('_');
		int RequiredMajor = -1, RequiredMinor = -1, RequiredSubMinor = -1, RequiredBuild = -1;
		switch (ClassSplit.Length)
		{
			case 5: RequiredBuild = int.Parse(ClassSplit[4]);
				goto case 4;
			case 4: RequiredSubMinor = int.Parse(ClassSplit[3]);
				goto case 3;
			case 3: RequiredMinor = int.Parse(ClassSplit[2]);
				goto case 2;
			case 2: RequiredMajor = int.Parse(ClassSplit[1]);
				break;
			default:
				throw new BuildException(string.Format("WwiseSoundEngine class name is invalid: {0}", ClassName));
		}

		var Version = GetInstance(PluginDirectory);
		return Version.Major == RequiredMajor
			&& (RequiredMinor == -1 || Version.Minor == RequiredMinor)
			&& (RequiredSubMinor == -1 || Version.SubMinor == RequiredSubMinor)
			&& (RequiredBuild == -1 || Version.Build == RequiredBuild);
	}

	public static string GetSoundEngineVersion(string PluginDirectory, string[] Modules)
	{
		foreach (var Module in Modules)
		{
			if (IsSoundEngineVersionSupported(PluginDirectory, Module))
			{
				return Module;
			}
		}

		var Version = GetInstance(PluginDirectory);
		throw new BuildException(string.Format("WwiseSoundEngine does not support the current SDK version: {0}.{1}.{2}.{3}", Version.Major, Version.Minor, Version.SubMinor, Version.Build));
	}
}