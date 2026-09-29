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

#include "AcousticTextureParamLookup.h"
#include "AkSettings.h"
#include "AkAcousticTexture.h"
#include "WwiseUnrealHelper.h"
#include "IAudiokineticTools.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseTask.h"

void AkAcousticTextureParamLookup::LoadAllTextures()
{
	FWwiseProjectDatabase* ProjectDatabase = FWwiseProjectDatabase::Get();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("LoadAllTextures: ProjectDatabase not loaded"));
		return;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto& AcousticTextures = DataStructure.GetAcousticTextures();

	if (AcousticTextures.Size() == 0)
	{
		return;
	}

	UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();

	if (UNLIKELY(!AkSettings))
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("No AkSettings while loading Acoustic Textures"));
		return;
	}

	for (auto& AcousticTexture : AcousticTextures)
	{
		WwiseDBPair<const WwiseDatabaseLocalizableIdKey, WwiseRefAcousticTexture> AcousticTexturePair(AcousticTexture);
		const FString& TextureName = FWwiseStringConverter::ToFString(*AcousticTexturePair.GetSecond().AcousticTextureName());
		float AbsorptionLow = AcousticTexturePair.GetSecond().GetAcousticTexture()->AbsorptionLow;
		float AbsorptionMidLow = AcousticTexturePair.GetSecond().GetAcousticTexture()->AbsorptionMidLow;
		float AbsorptionMidHigh = AcousticTexturePair.GetSecond().GetAcousticTexture()->AbsorptionMidHigh;
		float AbsorptionHigh = AcousticTexturePair.GetSecond().GetAcousticTexture()->AbsorptionHigh;
		uint32 TextureShortID = AcousticTexturePair.GetFirst().Id;

		UE_LOG(LogAudiokineticTools, VeryVerbose, TEXT("Properties for texture %s (%" PRIu32 "): Absorption High: %.0f%%, MidHigh: %.0f%%, MidLow: %.0f%%, Low: %.0f%%"),
			*TextureName, TextureShortID, AbsorptionHigh, AbsorptionMidHigh, AbsorptionMidLow, AbsorptionLow);

		auto Id = AcousticTexturePair.GetSecond().AcousticTextureGuid();
		
		const FVector4 AbsorptionValues = FVector4(AbsorptionLow, AbsorptionMidLow, AbsorptionMidHigh, AbsorptionHigh) / 100.0f;

		int A, B, C, D;
		Id.GetGuidValues(A, B, C, D);
		AkSettings->SetAcousticTextureParams(FGuid(A, B, C, D),{AbsorptionValues, TextureShortID});
	}
}

void AkAcousticTextureParamLookup::UpdateParamsMap() const
{
	UAkSettings* AkSettings = GetMutableDefault<UAkSettings>();
	if (AkSettings != nullptr)
	{
		AkSettings->ClearTextureParamsMap();
		// Loading Textures requires a lookup to the Asset Registry that must be made on the Game thread
		LaunchWwiseTask(TEXT("AkAcousticTextureParamLookup::UpdateParamsMap"), EWwiseTaskPriority::GameThread, [this]
		{
			LoadAllTextures();
		});
	}
}
