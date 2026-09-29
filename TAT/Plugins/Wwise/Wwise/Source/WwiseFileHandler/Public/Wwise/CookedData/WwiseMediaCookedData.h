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

#pragma once
#include "Wwise/WwisePackagedFile.h"
#include "Wwise/WwiseUnrealVersion.h"

#include "WwiseMediaCookedData.generated.h"

USTRUCT(BlueprintType)
struct WWISEFILEHANDLER_API FWwiseMediaCookedData
{
	GENERATED_BODY()

	/**
	 * @brief Short ID for the Media
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	int32 MediaId{ 0 };

	/**
	 * @brief Optional debug name. Can be empty in release, contain the name, or the full path of the asset.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	FName DebugName;

	/**
	 * @brief Packaging information for this file.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	FWwisePackagedFile PackagedFile;

	FWwiseMediaCookedData();

	void Serialize(FArchive& Ar);
	void SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options);
#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
	void PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const;
#endif

	FString GetDebugString() const;

	bool operator<(const FWwiseMediaCookedData& Rhs) const
	{
		return MediaId < Rhs.MediaId;
	}
};

inline uint32 GetTypeHash(const FWwiseMediaCookedData& InCookedData)
{
	return HashCombine(GetTypeHash(InCookedData.MediaId), GetTypeHash(InCookedData.PackagedFile.PathName));
}
inline bool operator==(const FWwiseMediaCookedData& InLhs, const FWwiseMediaCookedData& InRhs)
{
	return InLhs.MediaId == InRhs.MediaId && InLhs.PackagedFile.PathName == InRhs.PackagedFile.PathName;
}
inline bool operator!=(const FWwiseMediaCookedData& InLhs, const FWwiseMediaCookedData& InRhs)
{
	return InLhs.MediaId != InRhs.MediaId || InLhs.PackagedFile.PathName != InRhs.PackagedFile.PathName;
}
