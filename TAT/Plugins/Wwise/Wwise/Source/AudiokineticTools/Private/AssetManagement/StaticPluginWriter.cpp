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

#include "StaticPluginWriter.h"


#include "AkAudioBankGenerationHelpers.h"
#include "IAudiokineticTools.h"
#include "Platforms/AkPlatformInfo.h"
#include "Platforms/AkUEPlatform.h"

#include "Containers/Map.h"
#include "Misc/FileHelper.h"
#include "Wwise/WwiseStringConverter.h"
#include "Wwise/Metadata/WwiseMetadataPluginInfo.h"
#include "Wwise/Metadata/WwiseMetadataRootFile.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"

namespace StaticPluginWriter_Helper
{
	TArray<FString> GetLibraryFileNames(const FString& InWwisePlatformName)
	{
		TArray<FString> Result;

		FString RootSoundBanksMetadataFilePath = WwiseUnrealHelper::GetSoundBankProjectMetadataFile();
		WwiseDBString DBFilePath(FWwiseStringConverter::ToWwiseDBString(RootSoundBanksMetadataFilePath));
		if (!FPaths::FileExists(RootSoundBanksMetadataFilePath))
		{
			UE_LOG(LogAudiokineticTools, Verbose,
			       TEXT("StaticPluginWriter::GetLibraryFileNames: Failed to get SoundBanks Root metadata file"))
			return Result;
		}

		WwiseDBArray<WwiseDBString> FilePaths;
		FilePaths.Add(DBFilePath);
		WwiseMetadataFileMap JsonFiles = WwiseMetadataRootFile::LoadFiles(FilePaths);
		FString FileContents;
		if (!FFileHelper::LoadFileToString(FileContents, *RootSoundBanksMetadataFilePath))
		{
			UE_LOG(LogAudiokineticTools, Error,
			       TEXT("StaticPluginWriter::GetLibraryFileNames, Error while loading file %s to string")
			       , *RootSoundBanksMetadataFilePath)
			return Result;
		}

		WwiseMetadataSharedRootFilePtr MetadataRoot = WwiseMetadataRootFile::LoadFile(DBFilePath);

		FName PlatformPath;
		WwiseDBString AdapatedPlatformName(FWwiseStringConverter::ToWwiseDBString(InWwisePlatformName));
		for (const auto& Platform : MetadataRoot->ProjectInfo->Platforms)
		{
			if (Platform.Name.Equals(AdapatedPlatformName, EStringCompareType::IgnoreCase))
			{
				PlatformPath = *Platform.Path;
				break;
			}
		}

		if (!PlatformPath.IsValid() || PlatformPath.IsNone())
		{
			UE_LOG(LogAudiokineticTools, Verbose,
			       TEXT("StaticPluginWriter::GetLibraryFileNames, Could not find generated Soundbank for Platform %s"),
			       *InWwisePlatformName);
			return Result;
		}

		FString PlatformPluginInfoPath = *PlatformPath.ToString() / FString(TEXT("PluginInfo.json"));
		PlatformPluginInfoPath = WwiseUnrealHelper::GetSoundBankDirectory() / *PlatformPluginInfoPath;

		if (!FPaths::FileExists(*PlatformPluginInfoPath))
		{
			UE_LOG(LogAudiokineticTools, VeryVerbose,
			       TEXT("StaticPluginWriter::GetLibraryFileNames, Could not find PluginInfo metadata for Platform %s at %s"),
			       *InWwisePlatformName, *RootSoundBanksMetadataFilePath);
			return Result;
		}

		MetadataRoot = WwiseMetadataRootFile::LoadFile(FWwiseStringConverter::ToWwiseDBString(PlatformPluginInfoPath));

		// PluginLibNames + PluginLibIDs
		for (WwiseRefIndexType PluginLibIndex = 0; PluginLibIndex < MetadataRoot->PluginInfo->PluginLibs.Size(); ++
		     PluginLibIndex)
		{
			const WwiseMetadataPluginLib& PluginLib = MetadataRoot->PluginInfo->PluginLibs[PluginLibIndex];
			auto StaticPluginLibName = PluginLib.StaticLib;
			if (!StaticPluginLibName.IsEmpty())
			{
				Result.Add(FWwiseStringConverter::ToFString(StaticPluginLibName));
				UE_LOG(LogAudiokineticTools, Verbose,
				       TEXT("StaticPluginWriter_Helper::GetLibraryFileNames: Found plugin %s, id: 0x%X"),
				       *FWwiseStringConverter::ToFString(PluginLib.LibName), PluginLib.LibId);
			}
		}

		return Result;
	}

	void ModifySourceCode(const TCHAR* FilePath, const FString& PlatformName, const TArray<FString>& Plugins)
	{
		constexpr auto AkFactoryHeaderFormatString = TEXT("#include <AK/Plugin/{0}Factory.h>");

		constexpr auto StaticFactoryHeaderFileTemplate = TEXT("#if PLATFORM_{0}\n{1}\n#endif\n");

		TArray<FString> PluginLines;

		const TArray<FString> PluginsToSkip
		{
			// Don't include AkAudioInputSourceFactory.h in file because it is already linked in AkAudioInputManager
			TEXT("AkAudioInputSource"),
			// Don't include AkMeterFX.h because it is already included in WwiseSoundEngineAPI
			TEXT("AkMeterFX"),
			// Don't include TencentGME.h because it is already included in the GME Plugin
			TEXT("TencentGME")
		};
		for (auto& PluginName : Plugins)
		{
			if (PluginsToSkip.Contains(PluginName))
			{
				continue;
			}
			PluginLines.Add(FString::Format(AkFactoryHeaderFormatString, { PluginName }));
		}


		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, FilePath);

		TArray<FString> Header;
		for(auto& Line : Lines)
		{
			if(Line.Len() > 0)
			{
				Header.Add(Line);
			}
			else
			{
				break;
			}
		}

		FString HeaderString = FString::Join(Header, TEXT("\n"));
		HeaderString.Append(TEXT("\n"));
		HeaderString.Append(TEXT("\n"));
		
		FString StaticFactoryHeaderContent = FString::Format(StaticFactoryHeaderFileTemplate, { PlatformName.ToUpper(), FString::Join(PluginLines, TEXT("\n")) });

		HeaderString.Append(StaticFactoryHeaderContent);
		if (FFileHelper::SaveStringToFile(HeaderString, FilePath))
		{
			UE_LOG(LogAudiokineticTools, Display, TEXT("StaticPluginWriter::ModifySourceCode: Modified <%s> for <%s> platform."), FilePath, *PlatformName);
		}
		else
		{
			UE_LOG(LogAudiokineticTools, Warning, TEXT("StaticPluginWriter::ModifySourceCode: Could not modify <%s> for <%s> platform."), FilePath, *PlatformName);
		}
	}
}

namespace StaticPluginWriter
{
	void OutputPluginInformation(const FString& InWwisePlatform)
	{
		auto* PlatformInfo = UAkPlatformInfo::GetAkPlatformInfo(InWwisePlatform);
		if (!PlatformInfo)
		{
			UE_LOG(LogAudiokineticTools, Warning, TEXT("StaticPluginWriter::OutputPluginInformation: AkPlatformInfo class not found for <%s> platform."), *InWwisePlatform);
			return;
		}

		if (PlatformInfo->bUsesStaticLibraries)
		{
			const auto PluginArray = StaticPluginWriter_Helper::GetLibraryFileNames(InWwisePlatform);

			const FString AkPluginIncludeFileName = FString::Format(TEXT("Ak{0}Plugins.h"), { PlatformInfo->WwisePlatform });
			const FString AkPluginIncludeFilePath = FPaths::Combine(FAkPlatform::GetWwiseSoundEnginePluginDirectory(), TEXT("Source"), TEXT("WwiseSoundEngine"), TEXT("Public"), TEXT("Generated"), AkPluginIncludeFileName);

			StaticPluginWriter_Helper::ModifySourceCode(*AkPluginIncludeFilePath, PlatformInfo->WwisePlatform, PluginArray);
		}
	}
}
