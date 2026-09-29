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

#include "Wwise/CookedData/WwiseLocalizedShareSetCookedData.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Serialization/CompactBinaryWriter.h"
#endif

FWwiseLocalizedShareSetCookedData::FWwiseLocalizedShareSetCookedData():
	ShareSetLanguageMap(),
	DebugName(),
	ShareSetId(0)
{}

void FWwiseLocalizedShareSetCookedData::Serialize(FArchive& Ar)
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

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
void FWwiseLocalizedShareSetCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "SS";
	Writer.BeginObject();
	Writer << "Id" << ShareSetId;
	
	{
		Writer << "Langs";
		Writer.BeginArray();
		TArray<FWwiseLanguageCookedData> Languages;
		ShareSetLanguageMap.GetKeys(Languages);
		Languages.Sort();
	
		for (const auto& Language : Languages)
		{
			ShareSetLanguageMap[Language].PreSave(SaveContext, Writer);
		}
		Writer.EndArray();
	}

	Writer.EndObject();
}
#endif
