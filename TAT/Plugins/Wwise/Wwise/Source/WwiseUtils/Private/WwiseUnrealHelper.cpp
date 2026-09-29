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

#include "WwiseUnrealHelper.h"

#include "Async/Async.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Wwise/Stats/Global.h"


namespace WwiseUnrealHelper
{
	const TCHAR* MediaFolderName = TEXT("Media");
	constexpr auto SoundBankNamePrefix = TEXT("SB_");
	const FGuid InitBankID(0x701ECBBD, 0x9C7B4030, 0x8CDB749E, 0xE5D1C7B9);

	FString(*GetWwiseSoundEnginePluginDirectoryPtr)();
	FString(*GetWwiseProjectPathPtr)();
	FString(*GetSoundBankDirectoryPtr)();
	
	void SetHelperFunctions(FString(* GetWwiseSoundEnginePluginDirectoryImpl)(), FString(* GetWwiseProjectPathImpl)(),
		FString(* GetSoundBankDirectoryImpl)())
	{
		GetWwiseSoundEnginePluginDirectoryPtr = GetWwiseSoundEnginePluginDirectoryImpl;
		GetWwiseProjectPathPtr = GetWwiseProjectPathImpl;
		GetSoundBankDirectoryPtr = GetSoundBankDirectoryImpl;
	}

	FString GetWwiseSoundEnginePluginDirectory()
	{
		if (!GetWwiseSoundEnginePluginDirectoryPtr)
		{
			UE_LOG(LogWwiseUtils, Error, TEXT("WwiseUnrealHelper::GetWwiseSoundEnginePluginDirectory implementation not set."));
			return {};
		}	
		return GetWwiseSoundEnginePluginDirectoryPtr();
	}

	FString GetWwiseProjectPath()
	{
		if (!GetWwiseProjectPathPtr)
		{
			UE_LOG(LogWwiseUtils, Error, TEXT("WwiseUnrealHelper::GetWwiseProjectPath implementation not set."));
			return {};
		}	
		return GetWwiseProjectPathPtr();
	}

	FString GetSoundBankDirectory()
	{
		if (!GetSoundBankDirectoryPtr)
		{
			UE_LOG(LogWwiseUtils, Error, TEXT("WwiseUnrealHelper::GetSoundBankDirectory implementation not set."));
			return {};
		}	
		return GetSoundBankDirectoryPtr();
	}
	
	FString GetSoundBankProjectMetadataFile()
	{
		return GetSoundBankDirectoryPtr() / TEXT("ProjectInfo.json");
	}

	void TrimPath(FString& Path)
	{
		Path.TrimStartAndEndInline();
	}

	FString GetProjectDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	}

	FString GetContentDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	}

	FString GetThirdPartyDirectory()
	{
		return FPaths::Combine(GetWwiseSoundEnginePluginDirectory(), TEXT("ThirdParty"));
	}
	
	void RunTaskInGameThread(TFunction<void()> Function)
	{
		if (!IsInGameThread())
		{
			AsyncTask(ENamedThreads::Type::GameThread, [Function]
			{
				Function();
			});
		}
		else
		{
			Function();
		}
	}

#if WITH_EDITOR
	FString GuidToBankName(const FGuid& Guid)
	{
		if (Guid == InitBankID)
		{
			return TEXT("Init");
		}

		return FString(SoundBankNamePrefix) + Guid.ToString(EGuidFormats::Digits);
	}

	FGuid BankNameToGuid(const FString& BankName)
	{
		FString copy = BankName;
		copy.RemoveFromStart(SoundBankNamePrefix);

		FGuid result;
		FGuid::ParseExact(copy, EGuidFormats::Digits, result);

		return result;
	}

	FString FormatFolderPath(const FString folderPath)
	{
		auto path = folderPath.Replace(TEXT("\\"), TEXT("/"));
		if (path[0] == '/') {
			path.RemoveAt(0);
		}
		return path;
	}
#endif
}
