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

#include "Wwise/Metadata/WwiseMetadataAcousticTexture.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"
#include "WwiseDefines.h"

WwiseMetadataAcousticTexture::WwiseMetadataAcousticTexture(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader)
{
	Loader.GetPropertyArray(this, FloatProperties);
	Loader.LogParsed("AcousticTexture"_wwise_db, Id, Name);
}

const WwiseDBMap<WwiseDBString, size_t> WwiseMetadataAcousticTexture::FloatProperties = WwiseMetadataAcousticTexture::FillFloatProperties();
const WwiseDBMap<WwiseDBString, size_t> WwiseMetadataAcousticTexture::FillFloatProperties()
{
	WwiseDBMap<WwiseDBString, size_t> Result;
	Result.Add("AbsorptionLow"_wwise_db, offsetof(WwiseMetadataAcousticTexture, AbsorptionLow));
	Result.Add("AbsorptionMidLow"_wwise_db, offsetof(WwiseMetadataAcousticTexture, AbsorptionMidLow));
	Result.Add("AbsorptionMidHigh"_wwise_db, offsetof(WwiseMetadataAcousticTexture, AbsorptionMidHigh));
	Result.Add("AbsorptionHigh"_wwise_db, offsetof(WwiseMetadataAcousticTexture, AbsorptionHigh));
#if WWISE_2023_1_OR_LATER
	Result.Add("Scattering"_wwise_db, offsetof(WwiseMetadataAcousticTexture, Scattering));
#endif
	return Result;
}
