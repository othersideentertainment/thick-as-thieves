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

#include "Wwise/CookedData/WwiseMediaCookedData.h"

#include "Wwise/Stats/FileHandler.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Cooker/CookDependency.h"
#include "Serialization/CompactBinaryWriter.h"
#include "UObject/ObjectSaveContext.h"
#endif

#include <inttypes.h>

FWwiseMediaCookedData::FWwiseMediaCookedData()
{}

void FWwiseMediaCookedData::Serialize(FArchive& Ar)
{
	UStruct* Struct = StaticStruct();
	UE_CLOG(UNLIKELY(!Struct), LogWwiseFileHandler, Fatal, TEXT("MediaCookedData SerializeBulkData: No StaticStruct."));

	if (Ar.WantBinaryPropertySerialization())
	{
		UE_CLOG(Ar.IsSaving(), LogWwiseFileHandler, VeryVerbose, TEXT("Serializing to binary archive %s MediaCookedData %" PRIu32 " %s"), *Ar.GetArchiveName(), MediaId, *DebugName.ToString());
		Struct->SerializeBin(Ar, this);
		UE_CLOG(Ar.IsLoading(), LogWwiseFileHandler, VeryVerbose, TEXT("Serializing from binary archive %s MediaCookedData %" PRIu32 " %s"), *Ar.GetArchiveName(), MediaId, *DebugName.ToString());
	}
	else
	{
		UE_CLOG(Ar.IsSaving(), LogWwiseFileHandler, VeryVerbose, TEXT("Serializing to tagged archive %s MediaCookedData %" PRIu32 " %s"), *Ar.GetArchiveName(), MediaId, *DebugName.ToString());
		Struct->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this), Struct, nullptr);
		UE_CLOG(Ar.IsLoading(), LogWwiseFileHandler, VeryVerbose, TEXT("Serializing from tagged archive %s MediaCookedData %" PRIu32 " %s"), *Ar.GetArchiveName(), MediaId, *DebugName.ToString());
	}
}

void FWwiseMediaCookedData::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options)
{
	PackagedFile.SerializeBulkData(Ar, Options);
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
void FWwiseMediaCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "M";
	Writer.BeginObject();
	Writer << "Media" << MediaId;
	SaveContext.AddCookBuildDependency(PackagedFile.GetCookDependency());
	PackagedFile.FillCbObject(Writer);
	Writer.EndObject();
}
#endif

FString FWwiseMediaCookedData::GetDebugString() const
{
	return FString::Printf(TEXT("Media %s (%" PRIu32 ") @ %s (p:%" PRIi32 " ma:%" PRIi32 " %sdm %ss)"),
		*DebugName.ToString(), MediaId, *PackagedFile.PathName.ToString(), PackagedFile.PrefetchSize, PackagedFile.MemoryAlignment,
		PackagedFile.bDeviceMemory ? TEXT("") : TEXT("!"), PackagedFile.bStreaming ? TEXT("") : TEXT("!"));
}
