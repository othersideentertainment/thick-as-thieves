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

#include "Wwise/WwisePackagedFile.h"

#include "Wwise/WwiseFileCache.h"
#include "Wwise/Stats/AsyncStats.h"
#include "Wwise/Stats/FileHandler.h"
#include "Wwise/Stats/FileHandlerMemory.h"
#include "WwiseDefines.h"

#include "Async/MappedFileHandle.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/ITargetPlatform.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Cooker/CookDependency.h"
#include "Serialization/CompactBinaryWriter.h"
#endif

#include <inttypes.h>


TMap<uint32, FWwisePackagedFile::FWeakBulkDataArrayPtr> FWwisePackagedFile::KnownBulkDatas;
FCriticalSection FWwisePackagedFile::KnownBulkDatasCriticalSection;
constexpr int WwisePackagedFileLogHeaderAlignment = -90; 

FWwisePackagedFile::FWwisePackagedFile()
{
}

void FWwisePackagedFile::Serialize(FArchive& Ar)
{
	UStruct* Struct = StaticStruct();
	UE_CLOG(UNLIKELY(!Struct), LogWwiseFileHandler, Fatal, TEXT("FWwisePackagedFile Serialize: No StaticStruct."));

	if (Ar.WantBinaryPropertySerialization())
	{
		Struct->SerializeBin(Ar, this);
	}
	else
	{
		Struct->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this), Struct, nullptr);
	}
}

void FWwisePackagedFile::SerializeBulkData(FArchive& Ar, const FWwisePackagedFileSerializationOptions& Options)
{
#if NO_LOGGING
	static const FString HashDebugName;
#endif

	if (Ar.ShouldSkipBulkData())
	{
		return;
	}
	if (UE_SERVER)
	{
		return;
	}
	if (UNLIKELY(!Options.Owner))
	{
		UE_LOG(LogWwiseFileHandler, Error,
			TEXT("FWwisePackagedFile::SerializeBulkData: File serialization owner is unset. Skipping BulkData."));
		return;
	}

	// Only handle Packaged Files with Bulk Data.
	switch(PackagingStrategy)
	{
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::AdditionalFile:
		return;

	case EWwisePackagingStrategy::External:
	case EWwisePackagingStrategy::HybridAdditionalFile:
	case EWwisePackagingStrategy::BulkData:
		;
	}

	SCOPED_WWISEFILEHANDLER_EVENT_2(TEXT("FWwisePackagedFile::SerializeBulkData"));

	bool bIsLoadingFromCookedPlatform;
#if UE_5_2_OR_LATER
	bIsLoadingFromCookedPlatform = Ar.IsLoadingFromCookedPackage();
#else	
	const auto* Package = Options.Owner->GetPackage();
	bIsLoadingFromCookedPlatform = Package && Package->HasAnyPackageFlags(PKG_Cooked);
#endif
	
	if (bIsLoadingFromCookedPlatform)
	{
		UpdateBulkDataArrayToSharedVersion(Options);
	}

	// External files have no predefined Bulk Data. They should use existing ones.
	if (PackagingStrategy == EWwisePackagingStrategy::External)
	{
#if WITH_EDITORONLY_DATA
		const auto Header = FString::Printf(TEXT("Wwise [%s]: Skipping %s %s"), *Options.Owner->GetName(), *FPaths::GetCleanFilename(SourcePathName), *Options.ExtraLog);
		UE_CLOG(!bIsLoadingFromCookedPlatform, LogWwiseFileHandler, Display, TEXT("%*s [Asset Library]"), WwisePackagedFileLogHeaderAlignment, *Header);
#endif
		return;
	}

	if (bIsLoadingFromCookedPlatform)
	{
		auto& StoredBulkDataArray { SharedBulkDataArray.Get() };
		FBulkDataArray IgnoredBulkDataArray;
		const bool IgnoreBulkData { UNLIKELY(StoredBulkDataArray.Num() > 0) };
		auto& BulkDataArray { IgnoreBulkData ? IgnoredBulkDataArray : StoredBulkDataArray };
		
		// Retrieve full file or Prefetch Size as inline data
		if ((PackagingStrategy == EWwisePackagingStrategy::BulkData && !bStreaming) || PrefetchSize > 0)
		{
			auto& BulkData { BulkDataArray[BulkDataArray.Emplace(MakeShared<FByteBulkData, ESPMode::ThreadSafe>())].Get() };
			{
				SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwisePackagedFile::SerializeBulkData.Serialize Inline"));
				BulkData.Serialize(Ar, Options.Owner);
			}
#if UE_5_2_OR_LATER
			const auto DebugName { " ID " + BulkData.GetDebugName() };
#else
			const FString DebugName {};
#endif
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwisePackagedFile::SerializeBulkData: Loaded asset for %s %s[Bulk Data, %s, %" PRIi64 " bytes%s]"),
				*Options.Owner->GetName(), *Options.ExtraLog,
				Options.bOptional ?
					PrefetchSize > 0 ? TEXT("Optional Prefetch") : TEXT("Optional") :
					PrefetchSize > 0 ? TEXT("Prefetch") : TEXT("Inline"),
				BulkData.GetElementCount(),
				*DebugName);
		}

		// Retrieve Streaming data
		if (PackagingStrategy == EWwisePackagingStrategy::BulkData && bStreaming)
		{
			auto& BulkData { BulkDataArray[BulkDataArray.Emplace(MakeShared<FByteBulkData, ESPMode::ThreadSafe>())].Get() };
			{
				SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwisePackagedFile::SerializeBulkData.Serialize Streaming"));
				BulkData.Serialize(Ar, Options.Owner);
			}

#if UE_5_2_OR_LATER
			const auto DebugName { " ID " + BulkData.GetDebugName() };
#else
			const FString DebugName {};
#endif
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwisePackagedFile::SerializeBulkData: Loaded asset for %s %s[Bulk Data, %sStream, %" PRIi64 " bytes%s]"),
				*Options.Owner->GetName(), *Options.ExtraLog,
				Options.bOptional ? TEXT("Optional ") : TEXT(""),
				BulkData.GetElementCount(),
				*DebugName);
		}
	}
	
#if WITH_EDITORONLY_DATA
	if (Ar.IsCooking() && Ar.IsSaving() && Ar.IsPersistent() && !Ar.CookingTarget()->IsServerOnly() && !Ar.IsObjectReferenceCollector())
	{
		FEventRef EventRef;
		GetSourceFileToPtr([&, this](bool bResult, const uint8* Ptr, int64 Size)
		{
			ON_SCOPE_EXIT
			{
				EventRef->Trigger();
			};
			
			if (UNLIKELY(!bResult))
			{
				return;
			}

			if (UNLIKELY(Size < PrefetchSize))
			{
				UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::SerializeBulkData PrefetchSize [%" PRIi32 " bytes] is bigger than actual file size [%" PRIi64 " bytes] for %s"),
					PrefetchSize, Size, *SourcePathName);
				return;
			}

			// Package full file or Prefetch Size as inline data
			if ((PackagingStrategy == EWwisePackagingStrategy::BulkData && !bStreaming) || PrefetchSize > 0)
			{
				FByteBulkData BulkData;
				const auto* DataPtr = Ptr;
				const int64 DataSize = bStreaming ? PrefetchSize : Size;

				// Optional assets are typically related to Switch Containers and Wwise Asset Libraries, where the file
				// might not always be required depending on the context. They are packaged at the end of the file.
				// Otherwise, smaller assets are packaged inline (typically 128K in default Unreal settings), while
				// bigger assets are packaged at the end of the file.
				if (Options.bOptional)
				{
					BulkData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);
				}
				
				BulkData.RemoveBulkData();
				BulkData.Lock(EBulkDataLockFlags::LOCK_READ_WRITE);
				auto* Dest = BulkData.Realloc(DataSize);
				if (UNLIKELY(!Dest))
				{
					UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::SerializeBulkData Could not copy inline data to Bulk Data [%" PRIi64 " bytes] for %s"), DataSize, *SourcePathName);
					return;
				}
				FPlatformMemory::Memcpy(Dest, DataPtr, DataSize);
				BulkData.Unlock();

				const auto Header = FString::Printf(TEXT("Wwise [%s]: Cooking %s %s"), *Options.Owner->GetName(), *FPaths::GetCleanFilename(SourcePathName), *Options.ExtraLog);
				UE_CLOG(WwiseProjectUsageCount == 1, LogWwiseFileHandler, Display, TEXT("%*s [Bulk Data, %s, %" PRIi64 " bytes]"), WwisePackagedFileLogHeaderAlignment, *Header,
					Options.bOptional ?
						PrefetchSize > 0 ? TEXT("Optional Prefetch") : TEXT("Optional") :
						PrefetchSize > 0 ? TEXT("Prefetch") : TEXT("Inline"),
					DataSize);
				UE_CLOG(WwiseProjectUsageCount != 1, LogWwiseFileHandler, Display, TEXT("%*s [Bulk Data, %s, %" PRIi64 " bytes, %d users]"), WwisePackagedFileLogHeaderAlignment, *Header,
					Options.bOptional ?
						PrefetchSize > 0 ? TEXT("Optional Prefetch") : TEXT("Optional") :
						PrefetchSize > 0 ? TEXT("Prefetch") : TEXT("Inline"),
					DataSize, WwiseProjectUsageCount);
				
				BulkData.Serialize(Ar, Options.Owner);
			}

			// Package Streaming data
			if (PackagingStrategy == EWwisePackagingStrategy::BulkData && bStreaming)
			{
				FByteBulkData BulkData;
				const auto* DataPtr = (const int8*)(Ptr);
				const int64 DataSize = Size;

				// All streaming assets are written at the end of the file. 
				BulkData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);
				
				BulkData.RemoveBulkData();
				BulkData.Lock(EBulkDataLockFlags::LOCK_READ_WRITE);
				auto* Dest = BulkData.Realloc(DataSize);
				if (UNLIKELY(!Dest))
				{
					UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::SerializeBulkData Could not copy streamed data to Bulk Data [%" PRIi64 " bytes] for %s"), DataSize, *SourcePathName);
					return;
				}
				FPlatformMemory::Memcpy(Dest, DataPtr, DataSize);
				BulkData.Unlock();

				const auto Header = FString::Printf(TEXT("Wwise [%s]: Cooking %s %s"), *Options.Owner->GetName(), *FPaths::GetCleanFilename(SourcePathName), *Options.ExtraLog);
				UE_CLOG(WwiseProjectUsageCount == 1, LogWwiseFileHandler, Display, TEXT("%*s [Bulk Data, %sStream, %" PRIi64 " bytes]"), WwisePackagedFileLogHeaderAlignment, *Header,
					Options.bOptional ? TEXT("Optional ") : TEXT(""),
					DataSize);
				UE_CLOG(WwiseProjectUsageCount != 1, LogWwiseFileHandler, Display, TEXT("%*s [Bulk Data, %sStream, %" PRIi64 " bytes, %d users]"), WwisePackagedFileLogHeaderAlignment, *Header,
					Options.bOptional ? TEXT("Optional ") : TEXT(""),
					DataSize, WwiseProjectUsageCount);
				
				BulkData.Serialize(Ar, Options.Owner);
			}
		});
		EventRef->Wait();
	}
#endif
}

void FWwisePackagedFile::UpdateBulkDataArrayToSharedVersion(const FWwisePackagedFileSerializationOptions& Options)
{
#if NO_LOGGING
	static const FString HashDebugName;
#endif

#if WITH_EDITORONLY_DATA
	Hash = GetTypeHash(*this);
#endif
	
	if (UNLIKELY(Hash == 0))
	{
		UE_LOG(LogWwiseFileHandler, Error,
			TEXT("FWwisePackagedFile::SerializeBulkData: File serialization hash is unset for %s."),
			*Options.Owner->GetName());

		return;
	}

	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwisePackagedFile::UpdateBulkDataArrayToSharedVersion"));

	FSharedBulkDataArrayPtr SharedPtr;

	FScopeLock Lock{ &KnownBulkDatasCriticalSection };
	if (const auto FoundBulkDataArray { KnownBulkDatas.Find(Hash) })
	{
		SharedPtr = FoundBulkDataArray->Pin();
	}

	if (SharedPtr.IsValid())
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::UpdateBulkDataArrayToSharedVersion: Found shared asset for %s (Hash %" PRIu32 ")"),
			*Options.Owner->GetName(),
			Hash);

		SharedBulkDataArray = SharedPtr.ToSharedRef();
	}
	else
	{
		KnownBulkDatas.Add(Hash, FWeakBulkDataArrayPtr{SharedBulkDataArray});
	}
}

#if WITH_EDITORONLY_DATA
void FWwisePackagedFile::CookToSandbox(const FString& CookRootPath,
	const WriteAdditionalFileCallback& WriteAdditionalFile) const
{
	switch(PackagingStrategy)
	{
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::BulkData:
	case EWwisePackagingStrategy::External:
		return;
	default: ;
	}

	const auto CookPath = CookRootPath / PathName.ToString();

	FEventRef EventRef;
	GetSourceFileToPtr([this, &EventRef, &CookPath, &WriteAdditionalFile](bool bResult, const uint8* Ptr, int64 Size)
	{
		ON_SCOPE_EXIT
		{
			EventRef->Trigger();
		};
		if (UNLIKELY(!bResult))
		{
			return;
		}
		const auto Header = FString::Printf(TEXT("Wwise [%s]: Writing %s"), *PathName.ToString(), *FPaths::GetCleanFilename(SourcePathName));
		UE_LOG(LogWwiseFileHandler, Display, TEXT("%*s [Additional File, %" PRIi64 " bytes]"), WwisePackagedFileLogHeaderAlignment, *Header,
			Size);
		WriteAdditionalFile(*CookPath, (void*)Ptr, Size);
	});
	EventRef->Wait();
}

void FWwisePackagedFile::GetSourceFileToPtr(GetFileToPtrCallback&& InCallback) const
{
	GetAdditionalFileToPtr([this, InCallback = MoveTemp(InCallback)](bool bResult, const uint8* Ptr, int64 Size)
	{
		InCallback(bResult, Ptr, Size);
		if (LIKELY(bResult))
		{
			DeallocateMemory(Ptr, Size, false, STAT_WwiseMemoryCooking_FName, NAME_None);
		}
	},
		SourcePathName, false,
		STAT_WwiseMemoryCooking_FName, NAME_None, WWISE_LLM_GET_NAME(Audio_Wwise_FileHandler_Cooking),
		EAsyncIOPriorityAndFlags::AIOP_Normal, -1);

}

#endif

FString FWwisePackagedFile::GetPathName() const
{
#if WITH_EDITORONLY_DATA
	if (PackagingStrategy == EWwisePackagingStrategy::Source)
	{
		return SourcePathName;
	}
#endif

	if (PathName.IsNone())
	{
		return FString();
	}
#if WITH_EDITORONLY_DATA
	return PathName.ToString();
#else
	if(!ModularGameplayName.IsNone() && ModularGameplayName.ToString() != "Game")
	{
		return FPaths::ProjectPluginsDir() /  ModularGameplayName.ToString() / "Content" / PathName.ToString();
	}
	else
	{
		return FPaths::ProjectContentDir() / PathName.ToString();
	}
#endif
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
UE::Cook::FCookDependency FWwisePackagedFile::GetCookDependency() const
{
	return UE::Cook::FCookDependency::File(SourcePathName);
}

void FWwisePackagedFile::FillCbObject(FCbWriter& Writer) const
{
	Writer << "F";
	Writer.BeginObject();
	Writer << "S" << (uint8)PackagingStrategy
		<< "PN" << PathName
		<< "M" << ModularGameplayName
		<< "bS" << bStreaming
		<< "Pf" << PrefetchSize
		<< "A" << MemoryAlignment
		<< "D" << bDeviceMemory;
	Writer.EndObject();
}
#endif

void FWwisePackagedFile::GetFullFileToPtr(GetFileToPtrCallback&& InCallback,
                                          bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InStatDevice,
                                          const FName& InLlmName, EAsyncIOPriorityAndFlags InPriority) const
{
	switch( PackagingStrategy )
	{
	case EWwisePackagingStrategy::HybridAdditionalFile:
	case EWwisePackagingStrategy::BulkData:
	case EWwisePackagingStrategy::External:
		GetBulkFileToPtr(MoveTemp(InCallback),
						   bInEnforceMemoryRequirements, InStat, InStatDevice, InLlmName,
						   InPriority, -1);
		break;
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::AdditionalFile:
		{
			const auto FullPathName = GetPathName();
			if (UNLIKELY(FullPathName.IsEmpty()))
			{
				UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetFullFileToPtr: Trying to open a nameless file."));
				return InCallback(false, nullptr, 0);
			}

			GetAdditionalFileToPtr(MoveTemp(InCallback),
							   FullPathName, bInEnforceMemoryRequirements, InStat, InStatDevice, InLlmName,
							   InPriority, -1);
		}
		break;
	}
}

void FWwisePackagedFile::GetPrefetchToPtr(GetFileToPtrCallback&& InCallback,
	bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InStatDevice, const FName& InLlmName,
	EAsyncIOPriorityAndFlags InPriority) const
{
	switch( PackagingStrategy )
	{
	case EWwisePackagingStrategy::HybridAdditionalFile:
	case EWwisePackagingStrategy::BulkData:
	case EWwisePackagingStrategy::External:
		GetBulkFileToPtr(MoveTemp(InCallback),
						   bInEnforceMemoryRequirements, InStat, InStatDevice, InLlmName,
						   InPriority, PrefetchSize);
		break;
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::AdditionalFile:
		{
			const auto FullPathName = GetPathName();
			if (UNLIKELY(FullPathName.IsEmpty()))
			{
				UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetPrefetchToPtr: Trying to prefetch a nameless file."));
				return InCallback(false, nullptr, 0);
			}

			check(!FullPathName.IsEmpty());
			GetAdditionalFileToPtr(MoveTemp(InCallback),
							   FullPathName, bInEnforceMemoryRequirements, InStat, InStatDevice, InLlmName,
							   InPriority, PrefetchSize);
		}
	}
}

void FWwisePackagedFile::StreamFile(StreamFileCallback&& InCallback) const
{
	const auto FullPathName = GetPathName();

	FWwiseFileCache* FileCache = FWwiseFileCache::Get();
	if (UNLIKELY(!FileCache))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::StreamFile (%s): WwiseFileCache not available."), *FullPathName);
		return InCallback(nullptr);
	}
	auto** StreamedFile = new IWwiseFileCacheHandle*;
	if (UNLIKELY(!StreamedFile))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::StreamFile (%s): Could not create temporary handle in memory."), *FullPathName);
		return InCallback(nullptr);
	}
	
	*StreamedFile = nullptr;
	switch( PackagingStrategy )
	{
	case EWwisePackagingStrategy::HybridAdditionalFile:
	case EWwisePackagingStrategy::BulkData:
	case EWwisePackagingStrategy::External:
		{
			const auto& BulkDataArray { SharedBulkDataArray.Get() };

			if (UNLIKELY(BulkDataArray.Num() == 0))
			{
				UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::StreamFile: No Bulk Data loaded while asked for Bulk Data operation"));
				return InCallback(nullptr);
			}
			const auto& BulkData{ BulkDataArray.Last() };

			FileCache->CreateFileCacheHandle(*StreamedFile, BulkData, [StreamedFile, Callback = MoveTemp(InCallback)](bool bResult) mutable
			{
				// Contractually don't have to wait for OutHandle to be populated, it's done before the OnDone can be called. 
				auto Result = *StreamedFile;
				delete StreamedFile;

				if (!bResult)
				{
					Result->CloseAndDelete();
					Result = nullptr;
				}

				return Callback(Result);
			});
		}
		break;
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::AdditionalFile:
		{
			if (UNLIKELY(FullPathName.IsEmpty()))
			{
				UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::StreamFile: Trying to stream a nameless file."));
				return InCallback(nullptr);
			}

			FileCache->CreateFileCacheHandle(*StreamedFile, FullPathName, [StreamedFile, Callback = MoveTemp(InCallback)](bool bResult) mutable
			{
				// Contractually don't have to wait for OutHandle to be populated, it's done before the OnDone can be called. 
				auto Result = *StreamedFile;
				delete StreamedFile;

				if (!bResult)
				{
					Result->CloseAndDelete();
					Result = nullptr;
				}

				return Callback(Result);
			});
		}
	}
}

void FWwisePackagedFile::DeallocateMemory(const uint8* InMemoryPtr, int64 InMemorySize,
                                          bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InStatDevice) const
{
	if (UNLIKELY(!InMemoryPtr))
	{
		return;
	}

	if (bDeviceMemory && bInEnforceMemoryRequirements)
	{
#if AK_SUPPORT_DEVICE_MEMORY && !WWISE_2024_1_OR_LATER
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::DeallocateMemory: Deallocating %" PRIi64 " (%" PRIi32 ") bytes in Device Memory for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		ASYNC_DEC_MEMORY_STAT_BY_FName(InStatDevice, InMemorySize);
		AKPLATFORM::FreeDevice((void*)InMemoryPtr, InMemorySize, 0, true);
#else
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::DeallocateMemory: No Device Memory, but trying to deallocate %" PRIi64 " (%" PRIi32 ") bytes for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		return;
#endif
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::DeallocateMemory: Deallocating %" PRIi64 " (%" PRIi32 ") bytes in Unreal memory for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		FMemory::Free(const_cast<uint8*>(InMemoryPtr));
		ASYNC_DEC_MEMORY_STAT_BY_FName(InStat, InMemorySize);
	}
}

uint8* FWwisePackagedFile::AllocateMemory(int64 InMemorySize,
	bool bInEnforceMemoryRequirements, const FName& InStat, const FName& InStatDevice) const
{
	uint8* Result;

	if (bDeviceMemory && bInEnforceMemoryRequirements)
	{
#if AK_SUPPORT_DEVICE_MEMORY && !WWISE_2024_1_OR_LATER
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::AllocateMemory: Allocating %" PRIi64 " (%" PRIi32 ") bytes in Device Memory for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		Result = static_cast<uint8*>(AKPLATFORM::AllocDevice((size_t)InMemorySize, nullptr));
		if (Result)
		{
			ASYNC_INC_MEMORY_STAT_BY_FName(InStatDevice, InMemorySize);
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("Could not allocate %" PRIi64 " (%" PRIi32 ") bytes in Device Memory for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		}
#else
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::AllocateMemory: No Device Memory, but trying to allocate %" PRIi64 " (%" PRIi32 ") bytes for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		Result = nullptr;
#endif
	}
	else
	{
		if (UNLIKELY((MemoryAlignment & (MemoryAlignment - 1)) != 0))
		{
			UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::AllocateMemory: Invalid non-2^n Memory Alignment (%" PRIi32 ") for file %s."), MemoryAlignment, *PathName.ToString());
		}

		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::AllocateMemory: Allocating %" PRIi64 " (%" PRIi32 ") bytes in Unreal memory for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
		Result = static_cast<uint8*>(FMemory::Malloc(InMemorySize, bInEnforceMemoryRequirements ? MemoryAlignment : 0));
		if (Result)
		{
			ASYNC_INC_MEMORY_STAT_BY_FName(InStat, InMemorySize);
		}
		UE_CLOG(UNLIKELY(!Result), LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::AllocateMemory: Could not allocate %" PRIi64 " (%" PRIi32 ") bytes in Unreal memory for file %s"), InMemorySize, MemoryAlignment, *PathName.ToString());
	}
	return Result;
}

void FWwisePackagedFile::GetBulkFileToPtr(GetFileToPtrCallback&& InCallback, bool bInEnforceMemoryRequirements,
	const FName& InStat, const FName& InStatDevice, const FName& InLlmName, EAsyncIOPriorityAndFlags InPriority, int64 ReadFirstBytes) const
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwisePackagedFile::GetBulkFileToPtr"));
	const auto& BulkDataArray { SharedBulkDataArray.Get() };
	
	if (UNLIKELY(BulkDataArray.Num() == 0))
	{
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: No Bulk Data loaded while asked for Bulk Data operation"));
		return InCallback(false, nullptr, 0);
	}
	const auto& BulkData{ BulkDataArray[0] };
	if (UNLIKELY(ReadFirstBytes > BulkData->GetElementCount()))
	{
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Trying to read first %" PRIi64 " bytes when %" PRIi64 " are available as first chunk."),
			ReadFirstBytes, BulkData->GetElementCount());
		return InCallback(false, nullptr, 0);
	}

	const bool bReadEntireFile{ ReadFirstBytes == -1 || BulkData->GetElementCount() == ReadFirstBytes };
#if UE_5_2_OR_LATER
	const auto DebugName { BulkData->GetDebugName() };
#else
	const FString DebugName {"Bulk Data"};
#endif

	if (BulkData->IsBulkDataLoaded())
	{
		const auto Size{ bReadEntireFile ? BulkData->GetElementCount() : ReadFirstBytes };
		uint8* Ptr = AllocateMemory(Size, bInEnforceMemoryRequirements, InStat, InStatDevice);
		if (UNLIKELY(!Ptr))
		{
			UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Could not Allocate %" PRIi64 " for %s"), Size, *DebugName);
			InCallback(false, nullptr, 0);
			return;
		}

		UE_CLOG(!bReadEntireFile, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Getting the first %" PRIi64 " bytes of in-memory file %s (%" PRIi64 " bytes)"), ReadFirstBytes, *DebugName, Size);
		UE_CLOG(bReadEntireFile, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Getting the entire in-memory file %s (%" PRIi64 " bytes). Will be set as streamed afterwards."), *DebugName, Size);

		if (LIKELY(bReadEntireFile))
		{
			BulkData->SetBulkDataFlags(BULKDATA_SingleUse);
		}
		const auto* Data{ BulkData->LockReadOnly() };
		if (LIKELY(Data))
		{
			FPlatformMemory::Memcpy(Ptr, Data, Size);
			BulkData->Unlock();
			InCallback(true, Ptr, Size);
			return;
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Could not lock in-memory data of %" PRIi64 " for %s. Will try streaming it."), Size, *DebugName);
			DeallocateMemory(Ptr, Size, bInEnforceMemoryRequirements, InStat, InStatDevice);
		}
	}
	
	auto* FileCache = FWwiseFileCache::Get();
	if (UNLIKELY(!FileCache))
	{
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Failed to get FileCache instance while reading %s"), *DebugName);
		return InCallback(false, nullptr, 0);
	}

	auto* HandlePtr = new IWwiseFileCacheHandle*;
	if (UNLIKELY(!HandlePtr))
	{
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Failed to allocate FileCacheHandle pointer while reading %s"), *DebugName);
		return InCallback(false, nullptr, 0);
	}

	FileCache->CreateFileCacheHandle(*HandlePtr, BulkData,
		[this, BulkData, DebugName, HandlePtr, InCallback = MoveTemp(InCallback), bInEnforceMemoryRequirements, InStat, InStatDevice, InLlmName, InPriority, ReadFirstBytes, bReadEntireFile](bool bResult) mutable
		{
			SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwisePackagedFile::GetBulkFileToPtr Opened"));
			auto* Handle = *HandlePtr;
			
			if (UNLIKELY(!bResult) || UNLIKELY(!Handle))
			{
				UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Open failed while reading Bulk Data %s"), *DebugName);
				delete HandlePtr;
				InCallback(false, nullptr, 0);
				if (Handle) Handle->CloseAndDelete();
				return;
			}
			
			auto Size = Handle->GetFileSize();
		
			if (UNLIKELY(!Size))
			{
				UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::GetBulkFileToPtr: File not found %s"), *DebugName);
				delete HandlePtr;
				InCallback(false, nullptr, 0);
				Handle->CloseAndDelete();
				return;
			}

			if (!bReadEntireFile)
			{
				Size = ReadFirstBytes;
			}

			LLM_SCOPE_BYNAME(InLlmName);
			uint8* Ptr = AllocateMemory(Size, bInEnforceMemoryRequirements, InStat, InStatDevice);
			if (UNLIKELY(!Ptr))
			{
				UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Could not Allocate %" PRIi64 " for %s"), Size, *DebugName);
				delete HandlePtr;
				InCallback(false, nullptr, 0);
				Handle->CloseAndDelete();
				return;
			}

			UE_CLOG(!bReadEntireFile, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Getting the first %" PRIi64 " bytes of file %s (%" PRIi64 " bytes)"), Size, *DebugName, Handle->GetFileSize());
			UE_CLOG(bReadEntireFile, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Getting the entire file %s (%" PRIi64 " bytes)"), *DebugName, Size);

			Handle->ReadData(Ptr, 0, Size, InPriority,
				[this, HandlePtr, InCallback = MoveTemp(InCallback), BulkData, DebugName, Ptr, Size, bInEnforceMemoryRequirements, InStat, InStatDevice, ReadFirstBytes, bReadEntireFile](bool bResult) mutable
				{
					SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwisePackagedFile::GetBulkFileToPtr Done"));
					auto* Handle = *HandlePtr;
					delete HandlePtr;

					if (LIKELY(bResult))
					{
						UE_CLOG(!bReadEntireFile, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Done getting the first bytes of file %s (%" PRIi64 " bytes)"), *DebugName, Size);
						UE_CLOG(!bReadEntireFile, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Done getting the entire file %s (%" PRIi64 " bytes)"), *DebugName, Size);
					}
					else
					{
						UE_CLOG(!bReadEntireFile, LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Failed getting the first bytes of file %s (%" PRIi64 " bytes)"), *DebugName, Size);
						UE_CLOG(bReadEntireFile, LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::GetBulkFileToPtr: Failed getting the entire file %s (%" PRIi64 " bytes)"), *DebugName, Size);
						DeallocateMemory(Ptr, Size, bInEnforceMemoryRequirements, InStat, InStatDevice);
					}

					LaunchWwiseTask(WWISEFILEHANDLER_ASYNC_NAME("FWwisePackagedFile::GetBulkFileToPtr Callback"), [InCallback = MoveTemp(InCallback), bResult, Ptr, Size]
					{
						InCallback(bResult, Ptr, Size);
					});
			
					Handle->CloseAndDelete();
				});
		});
}

void FWwisePackagedFile::GetAdditionalFileToPtr(GetFileToPtrCallback&& InCallback, const FString& InFilePathname, bool bInEnforceMemoryRequirements,
	const FName& InStat, const FName& InStatDevice, const FName& InLlmName, EAsyncIOPriorityAndFlags InPriority, int64 ReadFirstBytes) const
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwisePackagedFile::GetAdditionalFileToPtr"));

	auto* FileCache = FWwiseFileCache::Get();
	if (UNLIKELY(!FileCache))
	{
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Failed to get FileCache instance while reading %s"), *InFilePathname);
		return InCallback(false, nullptr, 0);
	}

	auto* HandlePtr = new IWwiseFileCacheHandle*;
	if (UNLIKELY(!HandlePtr))
	{
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Failed to allocate FileCacheHandle pointer while reading %s"), *InFilePathname);
		return InCallback(false, nullptr, 0);
	}

	FileCache->CreateFileCacheHandle(*HandlePtr, InFilePathname,
	[this, HandlePtr, InCallback = MoveTemp(InCallback), InFilePathname, bInEnforceMemoryRequirements, InStat, InStatDevice, InLlmName, InPriority, ReadFirstBytes](bool bResult) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwisePackagedFile::GetAdditionalFileToPtr Opened"));
		auto* Handle = *HandlePtr;

		if (UNLIKELY(!bResult) || UNLIKELY(!Handle))
		{
			UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Open failed while reading %s"), *InFilePathname);
			delete HandlePtr;
			InCallback(false, nullptr, 0);
			if (Handle) Handle->CloseAndDelete();
			return;
		}
			
		auto Size = Handle->GetFileSize();
		
		if (UNLIKELY(!Size))
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: File not found %s"), *InFilePathname);
			delete HandlePtr;
			InCallback(false, nullptr, 0);
			Handle->CloseAndDelete();
			return;
		}

		if (ReadFirstBytes > 0 && Size > ReadFirstBytes)
		{
			Size = ReadFirstBytes;
		}

		LLM_SCOPE_BYNAME(InLlmName);
		uint8* Ptr = AllocateMemory(Size, bInEnforceMemoryRequirements, InStat, InStatDevice);
		if (UNLIKELY(!Ptr))
		{
			UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Could not Allocate %" PRIi64 " for %s"), Size, *InFilePathname);
			delete HandlePtr;
			InCallback(false, nullptr, 0);
			Handle->CloseAndDelete();
			return;
		}

		UE_CLOG(ReadFirstBytes >= 0, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Getting the first bytes of file %s (%" PRIi64 " bytes)"), *InFilePathname, Size);
		UE_CLOG(ReadFirstBytes == 0, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Getting the entire file %s (%" PRIi64 " bytes)"), *InFilePathname, Size);

		Handle->ReadData(Ptr, 0, Size, InPriority,
		[this, HandlePtr, InCallback = MoveTemp(InCallback), InFilePathname, Ptr, Size, bInEnforceMemoryRequirements, InStat, InStatDevice, ReadFirstBytes](bool bResult) mutable
		{
			SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwisePackagedFile::GetAdditionalFileToPtr Done"));
			auto* Handle = *HandlePtr;
			delete HandlePtr;

			if (LIKELY(bResult))
			{
				UE_CLOG(ReadFirstBytes >= 0, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Done getting the first bytes of file %s (%" PRIi64 " bytes)"), *InFilePathname, Size);
				UE_CLOG(ReadFirstBytes == 0, LogWwiseFileHandler, VeryVerbose, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Done getting the entire file %s (%" PRIi64 " bytes)"), *InFilePathname, Size);
			}
			else
			{
				UE_CLOG(ReadFirstBytes >= 0, LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Failed getting the first bytes of file %s (%" PRIi64 " bytes)"), *InFilePathname, Size);
				UE_CLOG(ReadFirstBytes == 0, LogWwiseFileHandler, Error, TEXT("FWwisePackagedFile::GetAdditionalFileToPtr: Failed getting the entire file %s (%" PRIi64 " bytes)"), *InFilePathname, Size);
				DeallocateMemory(Ptr, Size, bInEnforceMemoryRequirements, InStat, InStatDevice);
			}

			LaunchWwiseTask(WWISEFILEHANDLER_ASYNC_NAME("FWwisePackagedFile::GetAdditionalFileToPtr Callback"), [InCallback = MoveTemp(InCallback), bResult, Ptr, Size]
			{
				InCallback(bResult, Ptr, Size);
			});
			
			Handle->CloseAndDelete();
		});
	});
}
