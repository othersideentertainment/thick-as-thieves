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

#include "Wwise/Ref/WwiseRefAcousticTexture.h"

#include "Wwise/Metadata/WwiseMetadataAcousticTexture.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

const WwiseDBString WwiseRefAcousticTexture::NAME = "AcousticTexture"_wwise_db;

const WwiseMetadataAcousticTexture* WwiseRefAcousticTexture::GetAcousticTexture() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& AcousticTextures = SoundBank->AcousticTextures;
	if (AcousticTextures.IsValidIndex(AcousticTextureIndex))
	{
		return &AcousticTextures.Array[AcousticTextureIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Acoustic Texture index #%zu", AcousticTextureIndex);
		return nullptr;
	}
}

WwiseDBShortId WwiseRefAcousticTexture::AcousticTextureId() const
{
	const auto* AcousticTexture = GetAcousticTexture();
	if (!AcousticTexture) [[unlikely]]
	{
		return 0;
	}
	return AcousticTexture->Id;
}

WwiseDBGuid WwiseRefAcousticTexture::AcousticTextureGuid() const
{
	const auto* AcousticTexture = GetAcousticTexture();
	if (!AcousticTexture) [[unlikely]]
	{
		return {};
	}
	return AcousticTexture->GUID;
}

const WwiseDBString* WwiseRefAcousticTexture::AcousticTextureName() const
{
	const auto* AcousticTexture = GetAcousticTexture();
	if (!AcousticTexture) [[unlikely]]
	{
		return &emptyString;
	}
	return &AcousticTexture->Name;
}

const WwiseDBString* WwiseRefAcousticTexture::AcousticTextureObjectPath() const
{
	const auto* AcousticTexture = GetAcousticTexture();
	if (!AcousticTexture) [[unlikely]]
	{
		return &emptyString;
	}
	return &AcousticTexture->ObjectPath;
}

WwiseDBShortId WwiseRefAcousticTexture::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(AcousticTextureIndex));
	return Result;
}
