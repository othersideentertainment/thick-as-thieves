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

#include "Wwise/Ref/WwiseRefGameParameter.h"
#include "Wwise/Metadata/WwiseMetadataGameParameter.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

const WwiseDBString WwiseRefGameParameter::NAME = "GameParameter"_wwise_db;

const WwiseMetadataGameParameter* WwiseRefGameParameter::GetGameParameter() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& GameParameters = SoundBank->GameParameters;
	if (GameParameters.IsValidIndex(GameParameterIndex))
	{
		return &GameParameters[GameParameterIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get GameParameter index #%zu", GameParameterIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefGameParameter::GameParameterId() const
{
	const auto* GameParameter = GetGameParameter();
	if (!GameParameter) [[unlikely]]
	{
		return 0;
	}
	return GameParameter->Id;
}

WwiseDBGuid WwiseRefGameParameter::GameParameterGuid() const
{
	const auto* GameParameter = GetGameParameter();
	if (!GameParameter) [[unlikely]]
	{
		return {};
	}
	return GameParameter->GUID;
}

const WwiseDBString*  WwiseRefGameParameter::GameParameterName() const
{
	const auto* GameParameter = GetGameParameter();
	if (!GameParameter) [[unlikely]]
	{
		return &emptyString;
	}
	return &GameParameter->Name;
}

const WwiseDBString*  WwiseRefGameParameter::GameParameterObjectPath() const
{
	const auto* GameParameter = GetGameParameter();
	if (!GameParameter) [[unlikely]]
	{
		return &emptyString;
	}
	return &GameParameter->ObjectPath;
}

WwiseDBShortId WwiseRefGameParameter::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(GameParameterIndex));
	return Result;
}
