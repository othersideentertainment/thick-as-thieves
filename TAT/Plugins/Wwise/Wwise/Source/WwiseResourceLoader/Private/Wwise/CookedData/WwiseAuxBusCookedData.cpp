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

#include "Wwise/CookedData/WwiseAuxBusCookedData.h"

#include "Wwise/Stats/ResourceLoader.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Serialization/CompactBinaryWriter.h"
#endif

#include <inttypes.h>

FWwiseAuxBusCookedData::FWwiseAuxBusCookedData():
	AuxBusId(0),
	SoundBanks(),
	Media(),
	DebugName()
{}

void FWwiseAuxBusCookedData::Serialize(FArchive& Ar)
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

void FWwiseAuxBusCookedData::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options)
{
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
void FWwiseAuxBusCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "AuxBus";
	Writer.BeginObject();
	Writer << "Id" << AuxBusId;
	
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

FString FWwiseAuxBusCookedData::GetDebugString() const
{
	bool bFirst = true;
	auto Result = FString::Printf(TEXT("AuxBus %s (%" PRIu32 ")"), *DebugName.ToString(), AuxBusId);
	if (SoundBanks.Num() > 0)
	{
		if (bFirst)
		{
			Result += TEXT(" with ");
			bFirst = false;
		}
		else
		{
			Result += TEXT(" and ");
		}
		Result += FString::Printf(TEXT("%d SoundBank%s"), SoundBanks.Num(), SoundBanks.Num() > 1 ? TEXT("s") : TEXT(""));
	}
	if (Media.Num() > 0)
	{
		if (bFirst)
		{
			Result += TEXT(" with ");
			bFirst = false;
		}
		else
		{
			Result += TEXT(" and ");
		}
		Result += FString::Printf(TEXT("%d Media"), Media.Num());
	}
	return Result;
}
