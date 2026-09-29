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

#include "Wwise/WwiseDirectoryVisitor.h"

#include "Wwise/Metadata/WwiseMetadataRootFile.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"
#include "Wwise/Metadata/WwiseMetadataPlatform.h"
#include "Wwise/Metadata/WwiseMetadataLanguage.h"

#include "Wwise/AdapterTypes/IWwiseDirectoryVisitor.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"

class WwisePlatformVisitor
{
public:
	virtual ~WwisePlatformVisitor() = default;
	virtual WwiseGeneratedFiles::FPlatformFiles& Get() = 0;
};

//
// FPlatformRootDirectoryVisitor
//
class WwiseDirectoryVisitor::FPlatformRootDirectoryVisitor : public WwisePlatformVisitor
{
public:
	FPlatformRootDirectoryVisitor(
		const WwiseDBSharedPlatformId& InPlatform) :
		Platform(InPlatform)
	{}
	bool Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory);
	bool StartJobIfValid();
	WwiseGeneratedFiles::FPlatformFiles& Get();

	const WwiseDBSharedPlatformId Platform;

	WwiseGeneratedFiles::FPlatformFiles PlatformFiles;
	WwiseDBArray<WwiseDBFuture<WwisePlatformVisitor*>> Futures;
};

//
// FSoundBankVisitor
//
class WwiseDirectoryVisitor::FSoundBankVisitor : public WwisePlatformVisitor
{
public:
	FSoundBankVisitor()
	{
	}

	bool Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory);
	WwiseGeneratedFiles::FPlatformFiles& Get() override;

	WwiseGeneratedFiles::FPlatformFiles Result;
};

//
// FMediaVisitor
//
class WwiseDirectoryVisitor::FMediaVisitor : public WwisePlatformVisitor
{
public:
	FMediaVisitor()
	{
	}

	bool Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory);
	WwiseGeneratedFiles::FPlatformFiles& Get() override;

	WwiseGeneratedFiles::FPlatformFiles Result;
};

//
// WwiseDirectoryVisitor
//
bool WwiseDirectoryVisitor::Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory)
{
	// make sure all paths are "standardized" so the other end can match up with it's own standardized paths
	WwiseDBString RelativeFilename = FilenameOrDirectory;
	RelativeFilename.MakeStandardFilename();
	const auto Filename = RelativeFilename.GetFileName();

	if (Filename.StartsWith("."_wwise_db))
	{
		WWISE_DB_LOG(Verbose, "[WwiseDirectoryVisitor] Skipping: %s", *RelativeFilename);
		return true;
	}

	WWISE_DB_LOG(VeryVerbose, "[WwiseDirectoryVisitor] Visiting %s", *RelativeFilename);

	if (bIsDirectory)
	{
		// Skip directories, they are to be processed by ProjectInfo.json's Path
		return true;
	}

	WwiseGeneratedFiles::FileTuple FileToAdd(*RelativeFilename, IWwiseDirectoryVisitor<WwiseDirectoryVisitor>::GetTimeStamp(FilenameOrDirectory));
	const auto Extension = RelativeFilename.GetExtension();

	if (Filename.Equals("ProjectInfo.json"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(Verbose, "Found ProjectInfo: %s", *RelativeFilename);

		// We need to retrieve the Path from ProjectInfo. Parse this file immediately
		// (will be parsed twice. Now once, and officially later - since the file is small, it's not a big worry)
		auto Root = WwiseMetadataRootFile::LoadFile(WwiseDBString(*RelativeFilename));
		if (!Root || !Root->ProjectInfo)
		{
			WWISE_DB_LOG(Error, "Could not read ProjectInfo to retrieve paths.", *RelativeFilename);
			return true;
		}

		GeneratedDirectory.ProjectInfo = Root;

		const auto Path = WwiseDBString(FilenameOrDirectory).GetPath();

		auto& Platforms = Root->ProjectInfo->Platforms;
		bool bFoundPlatform = false;
		if (!PlatformName)
		{
			WWISE_DB_LOG(Log, "Skipping loading all platforms");
		}
		else
		{
			for (auto& Platform : Platforms)
			{
				if (PlatformName->Equals(Platform.Name, EStringCompareType::IgnoreCase))
				{
					bFoundPlatform = true;
					break;
				}
			}

			WWISE_DB_CLOG(!bFoundPlatform, Log, "Requested platform not found: %s", **PlatformName);
		}

		if (bFoundPlatform)
		{
			for (auto& Platform : Platforms)
			{
				const auto PlatformPath = Path / Platform.Path;

				WwiseDBString RequestedPlatformPath = ""_wwise_db; 

				if (PlatformPath.IsRelative())
				{
					RequestedPlatformPath = Path / Platform.Path;
				}
				else
				{
					RequestedPlatformPath = PlatformPath;
				}

				if (!PlatformName->Equals(Platform.Name, EStringCompareType::IgnoreCase))
				{
					WWISE_DB_LOG(Verbose, "Skipping platform %s", *Platform.Name);
					continue;
				}

				WWISE_DB_LOG(Verbose, "Visiting platform %s at: %s", *Platform.Name, *Platform.Path);
				
				Futures.Add(WwiseAsync([this, Platform, PlatformPath] {
					WwiseDBSharedPlatformId PlatformRef;
					PlatformRef.Platform = WwiseDBSharedPtr<WwiseDBPlatformId>(new WwiseDBPlatformId(Platform.GUID, *Platform.Name, *Platform.Path));
					auto RootVisitor = IWwiseDirectoryVisitor<FPlatformRootDirectoryVisitor>(new FPlatformRootDirectoryVisitor(PlatformRef));
					if (!RootVisitor.IterateDirectory(PlatformPath) ||
						!RootVisitor.DirectoryVisitor->StartJobIfValid())
					{
						delete RootVisitor.DirectoryVisitor;
						RootVisitor.DirectoryVisitor = nullptr;
						WWISE_DB_LOG(Warning, "Could not find generated platform %s at: %s", *PlatformRef.GetPlatformName(), *PlatformPath);
					}
					return RootVisitor.DirectoryVisitor;
				}));
			}
		}

		GeneratedDirectory.GeneratedRootFiles.ProjectInfoFile = std::move(FileToAdd.Tuple);
	}
	else if (Filename.Equals("Wwise_IDs.h"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(Verbose, "Found Wwise IDs: %s", *RelativeFilename);
		GeneratedDirectory.GeneratedRootFiles.WwiseIDsFile = std::move(FileToAdd.Tuple);
	}
	else if (Filename.Equals("SoundBanksGeneration.log"_wwise_db, EStringCompareType::IgnoreCase)
		|| Extension.Equals("xml"_wwise_db, EStringCompareType::IgnoreCase))
	{
		// Nothing to do
	}
	else
	{
		WWISE_DB_LOG(Log, "Unknown file. Not in a platform. Will be ignored: %s", *RelativeFilename);
	}

	return true;
}

WwiseGeneratedFiles& WwiseDirectoryVisitor::Get()
{
	for (auto& Future : Futures)
	{
		auto* Result = Future.Get();
		if (Result)
		{
			auto& PlatformFiles = Result->Get();
			if (PlatformFiles.IsValid())
			{
				GeneratedDirectory.Platforms.Add(Result->Platform, PlatformFiles);
			}
			delete Result;
		}
	}
	Futures.Empty();
	return GeneratedDirectory;
}



//
// FPlatformRootDirectoryVisitor
//
bool WwiseDirectoryVisitor::FPlatformRootDirectoryVisitor::Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory)
{
	// make sure all paths are "standardized" so the other end can match up with it's own standardized paths
	WwiseDBString RelativeFilename = WwiseDBString(FilenameOrDirectory);
	RelativeFilename.MakeStandardFilename();
	const auto Filename = RelativeFilename.GetFileName();

	if (Filename.StartsWith("."_wwise_db))
	{
		WWISE_DB_LOG(Verbose, "[RootFilesVisitor] Skipping: %s", *RelativeFilename);
		return true;
	}

	WWISE_DB_LOG(VeryVerbose, "[RootFilesVisitor] Visiting %s", *RelativeFilename);

	if (bIsDirectory)
	{
		PlatformFiles.DirectoriesToWatch.Add(RelativeFilename);

		if (Filename.Equals("Media"_wwise_db, EStringCompareType::IgnoreCase))
		{
			WWISE_DB_LOG(Verbose, "Found media directory: %s", *RelativeFilename);
			PlatformFiles.MediaDirectory = WwiseDBString(FilenameOrDirectory);
		}
		else if (Filename.Equals("Bus"_wwise_db, EStringCompareType::IgnoreCase)
			|| Filename.Equals("Event"_wwise_db, EStringCompareType::IgnoreCase))
		{
			WWISE_DB_LOG(Verbose, "Found auto SoundBank directory: %s", *RelativeFilename);
			PlatformFiles.AutoSoundBankDirectories.Add(WwiseDBString(FilenameOrDirectory));
		}
		else {
			WWISE_DB_LOG(Verbose, "Found language directory: %s", *RelativeFilename);
			PlatformFiles.LanguageDirectories.Add(WwiseDBString(FilenameOrDirectory));
		}

		return true;
	}

	WwiseGeneratedFiles::FileTuple FileToAdd(RelativeFilename, IWwiseDirectoryVisitor<WwiseDirectoryVisitor>::GetTimeStamp(FilenameOrDirectory));
	const auto Extension = RelativeFilename.GetExtension();

	if (!Extension.Equals("json"_wwise_db, EStringCompareType::IgnoreCase) 
		&& !Extension.Equals("txt"_wwise_db, EStringCompareType::IgnoreCase)
		&& !Extension.Equals("bnk"_wwise_db, EStringCompareType::IgnoreCase)
		&& !Extension.Equals("xml"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Adding extra file: %s", *RelativeFilename);
		PlatformFiles.ExtraFiles.Add(std::move(FileToAdd));
		return true;
	}

	if (Extension.Equals("bnk"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Adding SoundBank file: %s", *RelativeFilename);
		PlatformFiles.SoundBankFiles.Add(std::move(FileToAdd));
		return true;
	}
	else if (Extension.Equals("xml"_wwise_db, EStringCompareType::IgnoreCase)
		|| Extension.Equals("txt"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Skipping file: %s", *RelativeFilename);
		return true;
	}
	else if (Filename.Equals("SoundbanksInfo.json"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(Verbose, "Found monolithic SoundBank info: %s", *RelativeFilename);
		PlatformFiles.SoundbanksInfoFile = std::move(FileToAdd);
	}
	else if (Filename.Equals("PlatformInfo.json"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(Verbose, "Found platform info: %s", *RelativeFilename);
		PlatformFiles.PlatformInfoFile = std::move(FileToAdd);
	}
	else if (Filename.Equals("PluginInfo.json"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(Verbose, "Found plugin info: %s", *RelativeFilename);
		PlatformFiles.PluginInfoFile = std::move(FileToAdd);
	}
	else
	{
		WWISE_DB_LOG(VeryVerbose, "Adding metadata file: %s", *RelativeFilename);
		PlatformFiles.MetadataFiles.Add(std::move(FileToAdd));
	}

	return true;
}

bool WwiseDirectoryVisitor::FPlatformRootDirectoryVisitor::StartJobIfValid()
{
	if (!PlatformFiles.IsValid())
	{
		return false;
	}

	if (!PlatformFiles.MediaDirectory.IsEmpty())
	{
		const auto& Elem = PlatformFiles.MediaDirectory;
		Futures.Add(WwiseAsync([this, Elem] {
			WWISE_DB_LOG(Verbose, "Visiting media directory: %s", *Elem);
			auto MediaVisitor = IWwiseDirectoryVisitor<FMediaVisitor>(new FMediaVisitor());
			MediaVisitor.IterateDirectory(Elem);
			return (WwisePlatformVisitor*)MediaVisitor.DirectoryVisitor;
		}));
	}
	for (const auto& Elem : PlatformFiles.AutoSoundBankDirectories)
	{
		Futures.Add(WwiseAsync([this, Elem] {
			WWISE_DB_LOG(Verbose, "Visiting auto SoundBank directory: %s", *Elem);
			auto SoundBankVisitor = IWwiseDirectoryVisitor<FSoundBankVisitor>(new FSoundBankVisitor());
			SoundBankVisitor.IterateDirectory(Elem);
			return (WwisePlatformVisitor*)SoundBankVisitor.DirectoryVisitor;
		}));
	}

	for (const auto& Elem : PlatformFiles.LanguageDirectories)
	{
		Futures.Add(WwiseAsync([this, Elem] {
			WWISE_DB_LOG(Verbose, "Visiting language directory: %s", *Elem);
			auto SoundBankVisitor = IWwiseDirectoryVisitor<FSoundBankVisitor>(new FSoundBankVisitor());
			SoundBankVisitor.IterateDirectory(Elem);
			return (WwisePlatformVisitor*)SoundBankVisitor.DirectoryVisitor;
		}));
	}

	return true;
}

WwiseGeneratedFiles::FPlatformFiles& WwiseDirectoryVisitor::FPlatformRootDirectoryVisitor::Get()
{
	for (auto& Future : Futures)
	{
		auto Result = Future.Get();
		if (Result)
		{
			PlatformFiles.Append(std::move(Result->Get()));
			delete Result;
		}
	}
	Futures.Empty();
	return PlatformFiles;
}

//
// FSoundBankVisitor
//
bool WwiseDirectoryVisitor::FSoundBankVisitor::Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory)
{
	// make sure all paths are "standardized" so the other end can match up with it's own standardized paths
	WwiseDBString RelativeFilename = FilenameOrDirectory;
	RelativeFilename.MakeStandardFilename();

	const auto Filename = RelativeFilename.GetFileName();
	const auto Extension = RelativeFilename.GetExtension();

	if (Filename.StartsWith("."_wwise_db))
	{
		WWISE_DB_LOG(Verbose, "[SoundBankVisitor] Skipping: %s", *RelativeFilename);
		return true;
	}

	WWISE_DB_LOG(VeryVerbose, "[SoundBankVisitor] Visiting %s", *RelativeFilename);

	if (bIsDirectory)
	{
		WWISE_DB_LOG(VeryVerbose, "Iterating folder: %s", *RelativeFilename);
		Result.DirectoriesToWatch.Add(RelativeFilename);
		auto DirectoryVisitor = IWwiseDirectoryVisitor<FSoundBankVisitor>(this);
		DirectoryVisitor.IterateDirectory(FilenameOrDirectory);
		return true;
	}

	WwiseGeneratedFiles::FileTuple FileToAdd(RelativeFilename, IWwiseDirectoryVisitor<WwiseDirectoryVisitor>::GetTimeStamp(FilenameOrDirectory));

	if (Extension.Equals("json"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Adding metadata file: %s", *RelativeFilename);
		Result.MetadataFiles.Add(std::move(FileToAdd));
		return true;
	}
	else if (Extension.Equals("bnk"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Adding SoundBank file: %s", *RelativeFilename);
		Result.SoundBankFiles.Add(std::move(FileToAdd));
		return true;
	}
	else if (Extension.Equals("xml"_wwise_db, EStringCompareType::IgnoreCase)
		|| Extension.Equals("txt"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Skipping file: %s", *RelativeFilename);
		return true;
	}
	else
	{
		WWISE_DB_LOG(VeryVerbose, "Adding extra file: %s", *RelativeFilename);
		Result.ExtraFiles.Add(std::move(FileToAdd));
		return true;
	}
}

WwiseGeneratedFiles::FPlatformFiles& WwiseDirectoryVisitor::FSoundBankVisitor::Get()
{
	return Result;
}

//
// FMediaVisitor
//
bool WwiseDirectoryVisitor::FMediaVisitor::Visit(const WwiseDBString& FilenameOrDirectory, bool bIsDirectory)
{
	// make sure all paths are "standardized" so the other end can match up with it's own standardized paths
	WwiseDBString RelativeFilename = FilenameOrDirectory;
	RelativeFilename.MakeStandardFilename();

	WWISE_DB_LOG(VeryVerbose, "[MediaVisitor] Visiting %s", *RelativeFilename);

	if (bIsDirectory)
	{
		Result.DirectoriesToWatch.Add(RelativeFilename);
		auto MediaVisitor = IWwiseDirectoryVisitor<FMediaVisitor>(this);
		MediaVisitor.IterateDirectory(FilenameOrDirectory);
		return true;
	}
	WwiseGeneratedFiles::FileTuple FileToAdd(RelativeFilename, IWwiseDirectoryVisitor<WwiseDirectoryVisitor>::GetTimeStamp(FilenameOrDirectory));
	const auto Extension = RelativeFilename.GetExtension();

	if (Extension.Equals("wem"_wwise_db, EStringCompareType::IgnoreCase))
	{
		WWISE_DB_LOG(VeryVerbose, "Adding media file: %s", *RelativeFilename);
		Result.MediaFiles.Add(std::move(FileToAdd));
	}
	else
	{
		WWISE_DB_LOG(Log, "Adding unexpected extra file: %s", *RelativeFilename);
		Result.ExtraFiles.Add(std::move(FileToAdd));
	}
	return true;
}

WwiseGeneratedFiles::FPlatformFiles& WwiseDirectoryVisitor::FMediaVisitor::Get()
{
	return Result;
}
