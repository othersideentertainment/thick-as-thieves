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

#include "Wwise/CookedData/WwiseInitBankCookedData.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Serialization/CompactBinaryWriter.h"
#endif

#include <inttypes.h>

FWwiseInitBankCookedData::FWwiseInitBankCookedData():
	Media()
{}

void FWwiseInitBankCookedData::Serialize(FArchive& Ar)
{
	UStruct* Struct = StaticStruct();
	check(Struct);

	if (Ar.WantBinaryPropertySerialization())
	{
		Struct->SerializeBin(Ar, this);
	}
	else
	{
		Struct->SerializeTaggedProperties(Ar, (uint8*)this, Struct, nullptr);
	}
}

void FWwiseInitBankCookedData::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options)
{
	FWwiseSoundBankCookedData::SerializeBulkData(Ar, Options);

	for (auto& SoundBank : SoundBanks)
	{
		SoundBank.SerializeBulkData(Ar, Options);
	}
	for (auto& MediaItem : Media)
	{
		MediaItem.SerializeBulkData(Ar, Options);
	}
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
void FWwiseInitBankCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "InitBank";
	Writer.BeginObject();

	Writer << "Id" << SoundBankId;

	Writer << "SBs";
	Writer.BeginArray();
	for (auto& SoundBank : SoundBanks)
	{
		SoundBank.PreSave(SaveContext, Writer);
	}
	Writer.EndArray();

	Writer << "Ms";
	Writer.BeginArray();
	for (auto& MediaItem : Media)
	{
		MediaItem.PreSave(SaveContext, Writer);
	}
	Writer.EndArray();
	Writer.EndObject();
}
#endif

FString FWwiseInitBankCookedData::GetDebugString() const
{
	if (SoundBanks.Num() > 0 || Media.Num() > 0)
	{
		return FString::Printf(TEXT("InitBank %s (%" PRIu32 ") with %d additional SoundBanks and %d Media @ %s (ma:%" PRIi32 " %sdm %smedia %suser)"),
			*DebugName.ToString(), SoundBankId,
			SoundBanks.Num(), Media.Num(),
			*PackagedFile.PathName.ToString(), PackagedFile.MemoryAlignment,
			PackagedFile.bDeviceMemory ? TEXT("") : TEXT("!"), bContainsMedia ? TEXT("") : TEXT("!"),
			SoundBankType == EWwiseSoundBankType::User ? TEXT("") : TEXT("!"));
	}
	else
	{
		return FString::Printf(TEXT("InitBank %s (%" PRIu32 ") @ %s (ma:%" PRIi32 " %sdm %smedia %suser)"),
			*DebugName.ToString(), SoundBankId,
			*PackagedFile.PathName.ToString(), PackagedFile.MemoryAlignment,
			PackagedFile.bDeviceMemory ? TEXT("") : TEXT("!"), bContainsMedia ? TEXT("") : TEXT("!"),
			SoundBankType == EWwiseSoundBankType::User ? TEXT("") : TEXT("!"));
	}
}
