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

#include "Wwise/CookedData/WwiseLocalizedEventCookedData.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Serialization/CompactBinaryWriter.h"
#endif

FWwiseLocalizedEventCookedData::FWwiseLocalizedEventCookedData():
	EventLanguageMap(),
	DebugName(),
	EventId(0)
{}

void FWwiseLocalizedEventCookedData::Serialize(FArchive& Ar)
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

void FWwiseLocalizedEventCookedData::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& InOptions)
{
	for (auto& Event : EventLanguageMap)
	{
		auto Options(InOptions);
		if(Event.Key != Event.Key.Sfx)
		{
			Options.bOptional = true;
			Options.ExtraLog += Event.Key.LanguageName.ToString() + " ";
		}
		Event.Value.SerializeBulkData(Ar, Options);
	}
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
void FWwiseLocalizedEventCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "LocEvents";
	Writer.BeginObject();
	Writer << "EventId" << EventId;

	{
		Writer << "Langs";
		Writer.BeginArray();
		TArray<FWwiseLanguageCookedData> Languages;
		EventLanguageMap.GetKeys(Languages);
		Languages.Sort();
	
		for (const auto& Language : Languages)
		{
			EventLanguageMap[Language].PreSave(SaveContext, Writer);
		}
		Writer.EndArray();
	}

	Writer.EndObject();
}
#endif
