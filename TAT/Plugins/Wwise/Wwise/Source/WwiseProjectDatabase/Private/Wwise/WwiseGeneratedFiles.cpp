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

#include "Wwise/WwiseGeneratedFiles.h"
#include "Wwise/Metadata/WwiseMetadataRootFile.h"

bool WwiseGeneratedFiles::FPlatformFiles::IsValid() const
{
	return !PlatformInfoFile.Get<0>().IsEmpty()
		&& !PluginInfoFile.Get<0>().IsEmpty()
		&& (!SoundbanksInfoFile.Get<0>().IsEmpty() || SoundBankFiles.Size() > 0);
}

void WwiseGeneratedFiles::FPlatformFiles::Append(FPlatformFiles&& Rhs)
{
	SoundBankFiles.Append(std::move(Rhs.SoundBankFiles));
	MediaFiles.Append(std::move(Rhs.MediaFiles));
	MetadataFiles.Append(std::move(Rhs.MetadataFiles));
	ExtraFiles.Append(std::move(Rhs.ExtraFiles));

	DirectoriesToWatch.Append(std::move(Rhs.DirectoriesToWatch));
	LanguageDirectories.Append(std::move(Rhs.LanguageDirectories));
	AutoSoundBankDirectories.Append(std::move(Rhs.AutoSoundBankDirectories));
}

bool WwiseGeneratedFiles::IsValid() const
{
	return !GeneratedRootFiles.ProjectInfoFile.Get<0>().IsEmpty()
		&& ProjectInfo.IsValid() && ProjectInfo->ProjectInfo;
}
