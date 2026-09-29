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

#include "Serialization/BulkData.h"
#include "Wwise/WwiseUnrealVersion.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Cooker/CookDependency.h"
#endif

#include "WwisePackagedFile.generated.h"

class IWwiseFileCacheHandle;

/**
 * @brief Determines how to package this FWwisePackagedFile.
 */
UENUM(BlueprintType)
enum class EWwisePackagingStrategy : uint8
{
	/// No packaging. Retrieve the file directly from the Generated SoundBanks.
	Source,

	/// File is stored outside of the UAsset, through the "CookAdditionalFilesOverride" method. 
	AdditionalFile,

	/// File is stored outside of the UAsset. However, the prefetch is stored in the Bulk Data.
	HybridAdditionalFile,

	/// File is stored in the Bulk Data.
	BulkData,

	/// File is stored externally (in a library, elsewhere.)
	External
};

/**
 * @brief List of options for serialization of FWwisePackagedFile
 * */
struct WWISEFILEHANDLER_API FWwisePackagedFileSerializationOptions
{
	FWwisePackagedFileSerializationOptions(UObject* Owner) :
		Owner(Owner)
	{
	}
	
	/**
	 * Owner of this FWwisePackagedFile. Must be a UAkAudioType or equivalent. (Required)
	 */
	UObject* Owner;

	/**
	 * Whether the Bulk Data being packaged is optional or mandatory.
	 */
	bool bOptional{ false };

	/**
	 * Extra information to add to the logging. For example, the language. Should be followed by a space.
	 */
	FString ExtraLog;
};

/**
 * @brief Defines and handles one packaged file.
 */
USTRUCT(BlueprintType)
struct WWISEFILEHANDLER_API FWwisePackagedFile
{
	GENERATED_BODY()

	using GetFileToPtrCallback = TUniqueFunction<void(bool bResult, const uint8* Ptr, int64 Size)>;

	FWwisePackagedFile();

	/**
	 * Handles reading, writing, and reference collecting using FArchive.
	 * 
	 * @param Ar Archive used for (de)serializing the packaged file
	 */
	void Serialize(FArchive& Ar);

	/**
	 * Handles reading, writing, and reference collecting Bulk Data using FArchive.
	 *
	 * This method is called after the traditional serialization of the assets.
	 * 
	 * @param Ar Archive used for (de)serializing the packaged file
	 * @param Options Options necessary to properly serialize the Bulk Data in this structure.
	 */
	void SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options);

#if WITH_EDITORONLY_DATA
	using WriteAdditionalFileCallback = TFunctionRef<void(const TCHAR* Filename, void* Data, int64 Size)>;

	/**
	 * Handles writing files for CookAdditionalFilesOverride.
	 * 
	 * @param CookRootPath Path that determines where to cook the files.
	 * @param WriteAdditionalFile Callback function to store the files, as passed by CookAdditionalFilesOverride.
	 */
	void CookToSandbox(const FString& CookRootPath,
		const WriteAdditionalFileCallback& WriteAdditionalFile) const;

	/**
	 * Temporarily get the full source file. Used for packaging.
	 *
	 * Will automatically deallocate the source at close-time of the callback.
	 * 
	 * @param InCallback Callback function to use the memory pointer and size. If nullptr, it failed. Buffer will be deallocated at
	 *					 exit of the callback function.
	 */
	void GetSourceFileToPtr(GetFileToPtrCallback&& InCallback) const;
#endif
	
#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
	/**
	 * Retrieve a CookDepencency for the file to be packaged.
	 * 
	 * @return A dependency for the source file of this Packaged File. 
	 */
	[[nodiscard]] UE::Cook::FCookDependency GetCookDependency() const;

	/**
	 * Creates a named Object and fills it with the package information.
	 *
	 * @param Writer The Writer to add the object
	 */
	void FillCbObject(FCbWriter& Writer) const;
#endif

	/**
	 * Reads the entire packaged file, and provides the result to a callback. 
	 * 
	 * @param InCallback Callback containing the pointer and size of the packaged file. 
	 * @param bInEnforceMemoryRequirements Follow platform-specific memory and alignment requirements.
	 * @param InStat Stat to use for memory allocations. 
	 * @param InStatDevice Stat to use for allocations in device-specific memory.
	 * @param InLlmName LLM name to use for allocations.
	 * @param InPriority Priority of the load operation.
	 */
	void GetFullFileToPtr(GetFileToPtrCallback&& InCallback,
		bool bInEnforceMemoryRequirements,
		const FName& InStat, const FName& InStatDevice, const FName& InLlmName,
		EAsyncIOPriorityAndFlags InPriority = AIOP_Normal) const;

	/**
	 * Reads the prefetch part of the packaged file, and provides the result to a callback.
	 *
	 * This calls the callback immediately if there's no prefetch to load.
	 * 
	 * @param InCallback Callback containing the pointer and size of the packaged file. Can be "null/0" even with a valid bResult.
	 * @param bInEnforceMemoryRequirements Follow platform-specific memory and alignment requirements.
	 * @param InStat Stat to use for memory allocations. 
	 * @param InStatDevice Stat to use for allocations in device-specific memory.
	 * @param InLlmName LLM name to use for allocations.
	 * @param InPriority Priority of the load operation.
	 */
	void GetPrefetchToPtr(GetFileToPtrCallback&& InCallback,
		bool bInEnforceMemoryRequirements,
		const FName& InStat, const FName& InStatDevice, const FName& InLlmName,
		EAsyncIOPriorityAndFlags InPriority = AIOP_Normal) const;

	using StreamFileCallback = TUniqueFunction<void(IWwiseFileCacheHandle* Handle)>;

	/**
	 * Streams the file directly from storage.
	 * 
	 * @param InCallback A function called when the file is opened. If the Handle returned is nullptr, the file could not be opened. 
	 */
	void StreamFile(StreamFileCallback&& InCallback) const;

	/**
	 * Deallocate the memory for the file, as allocated by GetFullFileToPtr or GetPrefetchToPtr.
	 * 
	 * @param InMemoryPtr Memory pointer, as returned by the callback.
	 * @param InMemorySize Memory allocation size, as returned by the callback.
	 * @param bInEnforceMemoryRequirements Follow platform-specific memory and alignment requirements.
	 * @param InStat Stat to use for memory allocations. 
	 * @param InStatDevice Stat to use for allocations in device-specific memory.
	 */
	void DeallocateMemory(const uint8* InMemoryPtr, int64 InMemorySize, bool bInEnforceMemoryRequirements,
	                      const FName& InStat, const FName& InStatDevice) const;

protected:
	uint8* AllocateMemory(int64 InMemorySize,
		bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InStatDevice) const;

	void GetBulkFileToPtr(GetFileToPtrCallback&& InCallback,
		bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InLlmName,
		const FName& InStatDevice, EAsyncIOPriorityAndFlags InPriority, int64 ReadFirstBytes) const;
	void GetAdditionalFileToPtr(GetFileToPtrCallback&& InCallback, const FString& InFilePathname,
		bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InLlmName,
		const FName& InStatDevice, EAsyncIOPriorityAndFlags InPriority, int64 ReadFirstBytes) const;

public:
	/**
	 * Determines how to package the file.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	EWwisePackagingStrategy PackagingStrategy{ EWwisePackagingStrategy::Source };

	/**
	 * @brief Path name, including the platform's root, relative to the Wwise Staging Directory specified
	 * in the Wwise Integration Settings in Unreal.
	 *
	 * This is used either in Source or AdditionalFile, where the full path is provided here.
	 *
	 * In order to properly include the Project Content directory, use the GetFullPathName() method instead.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	FName PathName;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	FName ModularGameplayName;

	/**
	 * Returns the full path name, including the root Project Content directory.
	 *
	 * Use this instead of PathName to get the full path name to use in Unreal file operations.
	 * 
	 * @return The full path name as a string
	 */
	 FString GetPathName() const;

	/**
	 * @brief When true, the asset is not fully loaded in memory at load time.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	bool bStreaming{ false };

	/**
	 * @brief The number of bytes to retrieve at load time. Only used if streaming, in Source or AdditionalFile.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	int32 PrefetchSize{ 0 };

	/**
	 * @brief Alignment required to load the file on device. Can be 0 if there are no particular requirements.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	int32 MemoryAlignment{ 0 };

	/**
	 * @brief True if the file needs to be loaded in a special memory zone on the device.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Wwise")
	bool bDeviceMemory{ false };

#if WITH_EDITORONLY_DATA
	/**
	 * @brief Path name of the Source file within the generated SoundBanks directory.
	 *
	 * Used while serializing this asset in cooking exclusively.
	 */
	UPROPERTY()
	FString SourcePathName;

	/**
	 * @brief Number of usages in the Wwise project.
	 *
	 * Using the same file in two different Events, or the same Event in two different Switch Containers, will mark the file to 2.
	 * It can be 0 if undefined.
	 *
	 * This is not a ref-count!
	 */
	UPROPERTY(Transient)
	int WwiseProjectUsageCount { 0 };
#endif

	/**
	 * Content identification hash. Typically defined with the asset type, Short ID, and a language.
	 */
	UPROPERTY()
	uint32 Hash { 0 };

	using FBulkDataSharedRef = TSharedRef<FByteBulkData, ESPMode::ThreadSafe>;
	using FBulkDataArray = TArray<FBulkDataSharedRef>;
	using FSharedBulkDataArray = TSharedRef<FBulkDataArray, ESPMode::ThreadSafe>;
	using FSharedBulkDataArrayPtr = TSharedPtr<FBulkDataArray, ESPMode::ThreadSafe>;
	using FWeakBulkDataArrayPtr = TWeakPtr<FBulkDataArray, ESPMode::ThreadSafe>;
	
	/**
	 * @brief Array of the Bulk Data assets necessary for this packaged file.
	 *
	 * There are several options:
	 * - Provided Path Name + No Bulk Data: asset stored in additional file.
	 * - Provided Path Name + One Bulk Data: prefetch stored in Bulk Data, asset stored in additional file.
	 * - One Bulk Data for a fully streamed or fully fetched Bulk Data.
	 * - Two Bulk Data for a prefetched part alongside a streamed part.
	 */
	FSharedBulkDataArray SharedBulkDataArray{ MakeShared<FBulkDataArray, ESPMode::ThreadSafe>() };

	/**
	 * List of all the known Bulk Data currently in memory.
	 */
	static TMap<uint32, FWeakBulkDataArrayPtr> KnownBulkDatas;
	static FCriticalSection KnownBulkDatasCriticalSection;

protected:
	/**
	 * Get a previously acquainted BulkDataArray, or store the new Ref instance in the KnownBulkDatas map. 
	 * @param Options Options necessary to properly serialize the Bulk Data in this structure.
	 */
	void UpdateBulkDataArrayToSharedVersion(const FWwisePackagedFileSerializationOptions& Options);
};

inline uint32 GetTypeHash(const FWwisePackagedFile& PackagedFile)
{
#if WITH_EDITORONLY_DATA
	return GetTypeHash(PackagedFile.SourcePathName);
#else
	check(false);
	return 0;
#endif
}

inline bool operator==(const FWwisePackagedFile& Lhs, const FWwisePackagedFile& Rhs)
{
#if WITH_EDITORONLY_DATA
	return Lhs.SourcePathName == Rhs.SourcePathName;
#else
	check(false);
	return 0;
#endif
}