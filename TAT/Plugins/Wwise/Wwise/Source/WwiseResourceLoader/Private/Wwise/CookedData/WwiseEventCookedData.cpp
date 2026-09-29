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

#include "Wwise/CookedData/WwiseEventCookedData.h"

#include "Wwise/Stats/ResourceLoader.h"

#include <inttypes.h>

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Serialization/CompactBinaryWriter.h"
#endif

FWwiseEventCookedData::FWwiseEventCookedData():
	EventId(0),
	SoundBanks(),
	Media(),
	ExternalSources(),
	SwitchContainerLeaves(),
	RequiredGroupValueSet(),
	DestroyOptions(EWwiseEventDestroyOptions::WaitForEventEnd)
{}

void FWwiseEventCookedData::Serialize(FArchive& Ar)
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

void FWwiseEventCookedData::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options)
{
	for (auto& SoundBank : SoundBanks)
	{
		SoundBank.SerializeBulkData(Ar, Options);
	}
	for (auto& MediaItem : Media)
	{
		MediaItem.SerializeBulkData(Ar, Options);
	}
	for (auto& Leaf : SwitchContainerLeaves)
	{
		Leaf.SerializeBulkData(Ar, Options);
	}
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
void FWwiseEventCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "Event";
	Writer.BeginObject();

	Writer << "Id" << EventId;

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

	Writer << "Ls";
	Writer.BeginArray();
	for (auto& Leaf : SwitchContainerLeaves)
	{
		Leaf.PreSave(SaveContext, Writer);
	}
	Writer.EndArray();
	Writer.EndObject();
}
#endif

FString FWwiseEventCookedData::GetDebugString() const
{
	bool bFirst = true;
	auto Result = FString::Printf(TEXT("Event %s (%" PRIu32 ")"), *DebugName.ToString(), EventId);
	if (SoundBanks.Num() > 0)
	{
		if (bFirst)
		{
			Result += TEXT(" with ");
			bFirst = false;
		}
		else
		{
			Result += TEXT(", ");
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
			Result += TEXT(", ");
		}
		Result += FString::Printf(TEXT("%d Media"), Media.Num());
	}
	if (ExternalSources.Num() > 0)
	{
		if (bFirst)
		{
			Result += TEXT(" with ");
			bFirst = false;
		}
		else
		{
			Result += TEXT(", ");
		}
		Result += FString::Printf(TEXT("%d ExternalSource%s"), ExternalSources.Num(), ExternalSources.Num() > 1 ? TEXT("s") : TEXT(""));
	}
	if (SwitchContainerLeaves.Num() > 0)
	{
		if (bFirst)
		{
			Result += TEXT(" with ");
			bFirst = false;
		}
		else
		{
			Result += TEXT(", ");
		}
		Result += FString::Printf(TEXT("%d SwitchContainer Lea%s"), SwitchContainerLeaves.Num(), SwitchContainerLeaves.Num() > 1 ? TEXT("ves") : TEXT("f"));
	}
	if (RequiredGroupValueSet.Num() > 0)
	{
		if (bFirst)
		{
			Result += TEXT(" with ");
			bFirst = false;
		}
		else
		{
			Result += TEXT(", ");
		}
		Result += FString::Printf(TEXT("%d GroupValue%s"), RequiredGroupValueSet.Num(), RequiredGroupValueSet.Num() > 1 ? TEXT("s") : TEXT(""));
	}

	Result += FString::Printf(TEXT(" (%s)"), DestroyOptions == EWwiseEventDestroyOptions::StopEventOnDestroy ? TEXT("sod") : TEXT("wfe"));
	return Result;
}
