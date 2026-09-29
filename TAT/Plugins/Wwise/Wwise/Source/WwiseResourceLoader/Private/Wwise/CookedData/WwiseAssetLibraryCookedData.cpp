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

#include "Wwise/CookedData/WwiseAssetLibraryCookedData.h"

#include "Serialization/CompactBinaryWriter.h"

#if !UE_SERVER && WITH_EDITORONLY_DATA
#include "Interfaces/ITargetPlatform.h"
#endif

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Cooker/CookDependency.h"
#include "UObject/ObjectSaveContext.h"
#endif

void FWwiseAssetLibraryCookedData::Serialize(FArchive& Ar)
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

#if !UE_SERVER
#if WITH_EDITORONLY_DATA 
	if (Ar.IsCooking() && Ar.IsSaving() && !Ar.CookingTarget()->IsServerOnly())
	{
		uint32 NumPackagedFiles = PackagedFiles.Num();
		Ar.SerializeInt(NumPackagedFiles, UINT32_MAX);
		for (auto& PackagedFile : PackagedFiles)
		{
			PackagedFile.Serialize(Ar);
		}
	}
#else
	if (Ar.IsLoading())
	{
		uint32 NumPackagedFiles{ 0 };
		Ar.SerializeInt(NumPackagedFiles, UINT32_MAX);
		PackagedFiles.Empty(NumPackagedFiles);
		for (uint32 i = 0; i < NumPackagedFiles; ++i)
		{
			auto& PackagedFile { PackagedFiles[PackagedFiles.AddDefaulted()] };
			PackagedFile.Serialize(Ar);
		}
	}
#endif
#endif
}

void FWwiseAssetLibraryCookedData::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options)
{
	DebugName = Options.Owner->GetFName();
	
	for (auto& PackagedFile : PackagedFiles)
	{
		PackagedFile.SerializeBulkData(Ar, Options);
	}
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
void FWwiseAssetLibraryCookedData::PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const
{
	Writer << "Files";
	Writer.BeginArray();
	for (auto& PackagedFile : PackagedFiles)
	{
		SaveContext.AddCookBuildDependency(PackagedFile.GetCookDependency());
		PackagedFile.FillCbObject(Writer);
	}
	Writer.EndArray();
}
#endif

FString FWwiseAssetLibraryCookedData::GetDebugString() const
{
	return FString::Printf(TEXT("AssetLibrary %s"), *DebugName.ToString());
}
