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

#include "AkInclude.h"
#include "Wwise/WwisePackagedFile.h"
#include "Wwise/WwiseUnrealVersion.h"

#include "WwiseSoundBankCookedData.generated.h"

UENUM(BlueprintType)
enum class EWwiseSoundBankType : uint8
{
	User, // = AKCODECID_BANK,
	Event = AKCODECID_BANK_EVENT,
	Bus = AKCODECID_BANK_BUS
};

/**
 * @brief Required data to load a SoundBank
*/
USTRUCT(BlueprintType)
struct WWISEFILEHANDLER_API FWwiseSoundBankCookedData
{
	GENERATED_BODY()

	/**
	 * @brief Short ID for the SoundBank.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	int32 SoundBankId{ 0 };

	/**
	 * @brief True if the SoundBank contains media or media parts. False means a data-only SoundBank.
	 *
	 * Useful to load the SoundBank as a copy instead of keeping it Memory-mapped, as the SoundEngine will decode
	 * data from the SoundBank, and has no use for the file itself.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	bool bContainsMedia{ false };

	/**
	 * @brief User-created SoundBank, Event Auto-defined SoundBank, or Bus Auto-defined SoundBank.
	 *
	 * Useful for loading by file name.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	EWwiseSoundBankType SoundBankType{ EWwiseSoundBankType::User };

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

	FWwiseSoundBankCookedData();

	void Serialize(FArchive& Ar);
	void SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options);
#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
	void PreSave(FObjectPreSaveContext& SaveContext, FCbWriter& Writer) const;
#endif
	
	FString GetDebugString() const;

	bool operator<(const FWwiseSoundBankCookedData& Rhs) const
	{
		return SoundBankId < Rhs.SoundBankId;
	}
};

inline uint32 GetTypeHash(const FWwiseSoundBankCookedData& InCookedData)
{
#if WITH_EDITORONLY_DATA
	return HashCombine(GetTypeHash(InCookedData.PackagedFile.SourcePathName),
		HashCombine(GetTypeHash(InCookedData.SoundBankId), GetTypeHash(InCookedData.PackagedFile.PathName)));
#else
	return HashCombine(GetTypeHash(InCookedData.SoundBankId), GetTypeHash(InCookedData.PackagedFile.PathName));
#endif
}

inline bool operator==(const FWwiseSoundBankCookedData& InLhs, const FWwiseSoundBankCookedData& InRhs)
{
#if WITH_EDITORONLY_DATA
	return InLhs.PackagedFile.SourcePathName == InRhs.PackagedFile.SourcePathName &&
		InLhs.SoundBankId == InRhs.SoundBankId && InLhs.PackagedFile.PathName == InRhs.PackagedFile.PathName;
#else
	return InLhs.SoundBankId == InRhs.SoundBankId && InLhs.PackagedFile.PathName == InRhs.PackagedFile.PathName;
#endif
}

inline bool operator!=(const FWwiseSoundBankCookedData& InLhs, const FWwiseSoundBankCookedData& InRhs)
{
#if WITH_EDITORONLY_DATA
	return InLhs.PackagedFile.SourcePathName != InRhs.PackagedFile.SourcePathName ||
		InLhs.SoundBankId != InRhs.SoundBankId || InLhs.PackagedFile.PathName != InRhs.PackagedFile.PathName;
#else
	return InLhs.SoundBankId != InRhs.SoundBankId || InLhs.PackagedFile.PathName != InRhs.PackagedFile.PathName;
#endif
}
