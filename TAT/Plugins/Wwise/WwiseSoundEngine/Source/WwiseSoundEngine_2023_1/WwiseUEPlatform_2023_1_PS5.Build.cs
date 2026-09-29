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
using System.Collections.Generic;
using System.Reflection;

#if UE_5_2_OR_LATER
using Microsoft.Extensions.Logging;
#endif
using EpicGames.Core;

public class WwiseUEPlatform_2023_1_PS5 : WwiseUEPlatform
{
	public WwiseUEPlatform_2023_1_PS5(ReadOnlyTargetRules in_TargetRules, string in_ThirdPartyFolder) : base(in_TargetRules, in_ThirdPartyFolder) {}

	public override string GetLibraryFullPath(string LibName, string LibPath)
	{		
		return Path.Combine(LibPath, "lib" + LibName + ".a");
	}

	public override bool SupportsAkAutobahn { get { return false; } }
	
	public override bool SupportsCommunication { get { return true; } }

	public override bool SupportsDeviceMemory { get { return true; } }

	public override string AkPlatformLibDir 
	{ 
		get 
		{
			string ExpectedSdkVersion = Environment.ExpandEnvironmentVariables("%SCE_PROSPERO_SDK_DIR%");
			ExpectedSdkVersion = System.IO.Path.GetFileName(ExpectedSdkVersion); 

			if (ExpectedSdkVersion.Contains("9.000") || ExpectedSdkVersion.Contains("10.000"))
			{
				ExpectedSdkVersion += "_ABI" + new ABIGetter(Target).GetABI();
			}

			return "PS5_SDK" + ExpectedSdkVersion; 
		}
	}

	public override string DynamicLibExtension { get { return "prx"; } }

	public override List<string> GetAdditionalWwiseLibs()
	{
		return new List<string>();
	}

	public override List<string> GetPublicSystemLibraries()
	{
		return new List<string>
		{
			"SceAcm_stub_weak",
			"SceAudioOut2_stub_weak"
		};
	}
	
	public override List<string> GetPublicDelayLoadDLLs()
	{
		return new List<string>();
	}

	public override List<string> GetPublicDefinitions()
	{
		return new List<string>();
	}

	public override Tuple<string, string> GetAdditionalPropertyForReceipt(string ModuleDirectory)
	{
		return null;
	}

	public override List<string> GetPublicFrameworks()
	{
		return new List<string>();
	}
	
	private class ABIGetter
	{
		private ReadOnlyTargetRules Target;
		public ABIGetter(ReadOnlyTargetRules in_Target)
		{
			Target = in_Target;
		}
		
		private object GetPS5ToolchainObject()
		{
			var PS5ToolchainType = Type.GetType("UnrealBuildTool.PS5ToolChain, UnrealBuildTool");
			if (PS5ToolchainType == null)
			{
				return null;
			}

			var ClangToolChainOptionsType = Type.GetType("UnrealBuildTool.ClangToolChainOptions, UnrealBuildTool");
			if (ClangToolChainOptionsType == null)
			{
				return null;
			}
				
			var options = System.Enum.ToObject(ClangToolChainOptionsType, 0);
			if (options == null)
			{
				return null;
			}
				
#if UE_5_3_OR_LATER
			ILogger Logger = Target.Logger;
#else
			ILogger Logger = Log.Logger;
#endif
			
			return Activator.CreateInstance(PS5ToolchainType, new object[] { options, Logger });
		}

		private MethodInfo GetGetCompileArguments_GlobalMethod()
		{
			var PS5ToolchainType = Type.GetType("UnrealBuildTool.PS5ToolChain, UnrealBuildTool");
			if (PS5ToolchainType == null)
			{
				return null;
			}
			return PS5ToolchainType.GetMethod("GetCompileArguments_Global", BindingFlags.NonPublic | BindingFlags.Instance);
		}

		private object GetCppCompileEnvironmentObject()
		{
			var CPPCompileEnvironmentType = Type.GetType("UnrealBuildTool.CppCompileEnvironment, UnrealBuildTool");
			if (CPPCompileEnvironmentType == null)
			{
				return null;
			}
				
			var CppConfigurationType = Type.GetType("UnrealBuildTool.CppConfiguration, UnrealBuildTool");
			if (CppConfigurationType == null)
			{
				return null;
			}
				
				
			var CppConfig = System.Enum.Parse(CppConfigurationType, Target.Configuration.ToString());
			if (CppConfig == null)
			{
				return null;
			}
				
			return Activator.CreateInstance(CPPCompileEnvironmentType, new object[] { UnrealTargetPlatform.Parse("PS5"), CppConfig, Target.Architectures, null });
		}

		public string GetABI()
		{
			// Do everything needed to call PS5ToolChain.GetCompileArguments_Global
			string FoundABI = "v1";

			object PS5ToolchainObject = null;
			MethodInfo GetCompileArgsMethod = null;
			object CppCompileEnvironmentObject = null;
			
			// Activator.CreateInstance can throw. If this happens, we simply want to return the default ABI
			try
			{
				PS5ToolchainObject = GetPS5ToolchainObject();
				GetCompileArgsMethod = GetGetCompileArguments_GlobalMethod();
				CppCompileEnvironmentObject = GetCppCompileEnvironmentObject();
			}
			catch {}

			if (PS5ToolchainObject == null || GetCompileArgsMethod == null || CppCompileEnvironmentObject == null)
			{
#if UE_5_3_OR_LATER
				Target.Logger.LogInformation("Wwise_PS5: Unable to get sce-stdlib version using reflection, using default value of " + FoundABI);
#else
				Log.TraceInformation("Wwise_PS5: Unable to get sce-stdlib version using reflection, using default value of " + FoundABI);
#endif
				return FoundABI;
			}
			
			List<string> CompileArgs = new List<string>();
			GetCompileArgsMethod.Invoke(PS5ToolchainObject, new[] { CppCompileEnvironmentObject, CompileArgs });

			foreach (var arg in CompileArgs)
			{
				if (arg.Contains("sce-stdlib"))
				{
					FoundABI = arg.Split('=')[1];
					break;
				}
			}

			return FoundABI;
		}		
	}
}

public class WwiseUEPlatform_2023_1_DPX : WwiseUEPlatform_2023_1_PS5
{
	public WwiseUEPlatform_2023_1_DPX(ReadOnlyTargetRules in_TargetRules, string in_ThirdPartyFolder) : base(in_TargetRules, in_ThirdPartyFolder) { }
}