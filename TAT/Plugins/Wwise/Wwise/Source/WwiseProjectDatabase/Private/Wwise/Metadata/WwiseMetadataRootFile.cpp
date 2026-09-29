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

#include "Wwise/Metadata/WwiseMetadataRootFile.h"

#include "Wwise/AdapterTypes/WwiseSharedPtr.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Metadata/WwiseMetadataPluginInfo.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"
#include "Wwise/Metadata/WwiseMetadataSoundBanksInfo.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataRootFile::WwiseMetadataRootFile(WwiseMetadataLoader& Loader) :
	PlatformInfo(Loader.GetObjectPtr<WwiseMetadataPlatformInfo>(this, "PlatformInfo"_wwise_db)),
	PluginInfo(Loader.GetObjectPtr<WwiseMetadataPluginInfo>(this, "PluginInfo"_wwise_db)),
	ProjectInfo(Loader.GetObjectPtr<WwiseMetadataProjectInfo>(this, "ProjectInfo"_wwise_db)),
	SoundBanksInfo(Loader.GetObjectPtr<WwiseMetadataSoundBanksInfo>(this, "SoundBanksInfo"_wwise_db))
{
	if (Loader.bResult && !PlatformInfo && !PluginInfo && !ProjectInfo && !SoundBanksInfo)
	{
		Loader.Fail("WwiseMetadataRootFile"_wwise_db);
	}
	IncLoadedSize(sizeof(WwiseMetadataRootFile));
}

WwiseMetadataRootFile::~WwiseMetadataRootFile()
{
	if (PlatformInfo)
	{
		delete PlatformInfo;
		PlatformInfo = nullptr;
	}
	if (PluginInfo)
	{
		delete PluginInfo;
		PluginInfo = nullptr;
	}
	if (ProjectInfo)
	{
		delete ProjectInfo;
		ProjectInfo = nullptr;
	}
	if (SoundBanksInfo)
	{
		delete SoundBanksInfo;
		SoundBanksInfo = nullptr;
	}
}

WwiseMetadataSharedRootFilePtr WwiseMetadataRootFile::LoadFile(const WwiseDBString& FilePath)
{
	SCOPED_WWISEPROJECTDATABASE_EVENT_F_2(TEXT("FWwiseMetadataRootFile::LoadFile: %s"), *FilePath);
	WWISE_DB_LOG(Verbose, "Parsing file in: %s", *FilePath);

	WwiseDBJsonObject RootJsonObject(FilePath);
	WwiseMetadataLoader Loader(RootJsonObject);
	auto Result = WwiseDBSharedPtr<WwiseMetadataRootFile>();
	Result.Make_Shared(Loader);

	if (!Loader.bResult)
	{
		Loader.LogParsed("LoadFile"_wwise_db, 0, WwiseDBString(FilePath));
		return {};
	}

	return Result;
}

WwiseMetadataFileMap WwiseMetadataRootFile::LoadFiles(const WwiseDBArray<WwiseDBString>& FilePaths)
{
	WwiseMetadataFileMap Result;

	for(const auto& FilePath : FilePaths)
	{
		Result.Add(FilePath, nullptr);
	}
	
	WwiseParallelFor(FilePaths,
	[&Result, &FilePaths](const auto& n)
	{
		Result.AddOrReplace(FilePaths(n), WwiseMetadataRootFile::LoadFile(FilePaths(n)));
	});

	return Result;
}
