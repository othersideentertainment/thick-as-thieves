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

#pragma once

#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#include "Wwise/Metadata/WwiseMetadataCollections.h"
#include "Wwise/WwiseSharedDBPlatformId.h"

struct WwiseGeneratedFiles
{
	using FileTuple = WwiseDBTuple<WwiseDBString, WwiseDateTime>;
	using FileDateTimeMap = WwiseDBMap<WwiseDBString, WwiseDateTime>;

	struct FGeneratedRootFiles
	{
		FileTuple ProjectInfoFile;
		FileTuple WwiseIDsFile;
	};

	struct FPlatformFiles
	{
		FileTuple PlatformInfoFile;
		FileTuple PluginInfoFile;
		FileTuple SoundbanksInfoFile;

		FileDateTimeMap SoundBankFiles;
		FileDateTimeMap MediaFiles;
		FileDateTimeMap MetadataFiles;
		FileDateTimeMap ExtraFiles;

		WwiseDBArray<WwiseDBString> DirectoriesToWatch;
		WwiseDBArray<WwiseDBString> LanguageDirectories;
		WwiseDBArray<WwiseDBString> AutoSoundBankDirectories;
		WwiseDBString MediaDirectory;

		bool IsValid() const;

		void Append(FPlatformFiles&& Rhs);
	};

	FGeneratedRootFiles GeneratedRootFiles;
	WwiseDBMap<WwiseDBSharedPlatformId, FPlatformFiles> Platforms;
	WwiseMetadataSharedRootFileConstPtr ProjectInfo;

	bool IsValid() const;
};
