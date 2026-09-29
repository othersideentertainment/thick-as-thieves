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

#include "Wwise/Ref/WwiseRefMedia.h"
#include "Wwise/Metadata/WwiseMetadataMedia.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

const WwiseDBString WwiseRefMedia::NAME = "Media"_wwise_db;

const WwiseMetadataMedia* WwiseRefMedia::GetMedia() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}

	const auto& Media = SoundBank->Media;
	if (Media.IsValidIndex(MediaIndex))
	{
		return &Media[MediaIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Media index #%zu", MediaIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefMedia::MediaId() const
{
	const auto* Media = GetMedia();
	if (!Media) [[unlikely]]
	{
		return 0;
	}
	return Media->Id;
}

const WwiseDBString* WwiseRefMedia::MediaShortName() const
{
	const auto* Media = GetMedia();
	if (!Media) [[unlikely]]
	{
		return {};
	}
	return &Media->ShortName;
}

const WwiseDBString* WwiseRefMedia::MediaPath() const
{
	const auto* Media = GetMedia();
	if (!Media) [[unlikely]]
	{
		return {};
	}
	return &Media->Path;
}

WwiseDBShortId WwiseRefMedia::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(MediaIndex));
	return Result;
}
