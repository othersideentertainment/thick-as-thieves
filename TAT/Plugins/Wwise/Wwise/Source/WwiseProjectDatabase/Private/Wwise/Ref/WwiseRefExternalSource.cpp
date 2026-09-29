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


#include "Wwise/Ref/WwiseRefExternalSource.h"
#include "Wwise/Metadata/WwiseMetadataExternalSource.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

const WwiseDBString WwiseRefExternalSource::NAME = "ExternalSource"_wwise_db;

const WwiseMetadataExternalSource* WwiseRefExternalSource::GetExternalSource() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& ExternalSources = SoundBank->ExternalSources;
	if (ExternalSources.IsValidIndex(ExternalSourceIndex))
	{
		return &ExternalSources[ExternalSourceIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get External Source index #%zu", ExternalSourceIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefExternalSource::ExternalSourceCookie() const
{
	const auto* ExternalSource = GetExternalSource();
	if (!ExternalSource) [[unlikely]]
	{
		return {};
	}
	return ExternalSource->Cookie;
}

WwiseDBGuid WwiseRefExternalSource::ExternalSourceGuid() const
{
	const auto* ExternalSource = GetExternalSource();
	if (!ExternalSource) [[unlikely]]
	{
		return {};
	}
	return ExternalSource->GUID;
}

const WwiseDBString*  WwiseRefExternalSource::ExternalSourceName() const
{
	const auto* ExternalSource = GetExternalSource();
	if (!ExternalSource) [[unlikely]]
	{
		return &emptyString;
	}
	return &ExternalSource->Name;
}

const WwiseDBString* WwiseRefExternalSource::ExternalSourceObjectPath() const
{
	const auto* ExternalSource = GetExternalSource();
	if (!ExternalSource) [[unlikely]]
	{
		return &emptyString;
	}
	return &ExternalSource->ObjectPath;
}

WwiseDBShortId WwiseRefExternalSource::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(ExternalSourceIndex));
	return Result;
}
