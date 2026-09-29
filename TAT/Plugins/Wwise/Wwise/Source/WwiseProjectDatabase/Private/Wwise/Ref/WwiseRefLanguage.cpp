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

#include "Wwise/Ref/WwiseRefLanguage.h"

#include "Wwise/Metadata/WwiseMetadataLanguage.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"

const WwiseDBString WwiseRefLanguage::NAME = "Language"_wwise_db;

const WwiseMetadataLanguage* WwiseRefLanguage::GetLanguage() const
{
	const auto* ProjectInfo = GetProjectInfo();
	if (!ProjectInfo) [[unlikely]]
	{
		return nullptr;
	}
	const auto& Platforms = ProjectInfo->Languages;
	if (Platforms.IsValidIndex(LanguageIndex))
	{
		return &Platforms[LanguageIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Language index #%zu", LanguageIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefLanguage::LanguageId() const
{
	const auto* Language = GetLanguage();
	if (!Language) [[unlikely]]
	{
		return 0;
	}
	return Language->Id;
}

WwiseDBGuid WwiseRefLanguage::LanguageGuid() const
{
	const auto* Language = GetLanguage();
	if (!Language) [[unlikely]]
	{
		return {};
	}
	return Language->GUID;
}

WwiseDBString WwiseRefLanguage::LanguageName() const
{
	const auto* Language = GetLanguage();
	if (!Language) [[unlikely]]
	{
		return {};
	}
	return Language->Name;
}

WwiseDBShortId WwiseRefLanguage::Hash() const
{
	auto Result = WwiseRefProjectInfo::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(LanguageIndex));
	return Result;
}
