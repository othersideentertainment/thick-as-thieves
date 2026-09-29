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

#include "GenericPlatform/GenericPlatformFile.h"
#include "AkInclude.h"
#include "Serialization/BulkData.h"

#include "Wwise/WwiseExecutionQueue.h"

class FWwiseAsyncCycleCounter;
class FWwiseFileSystemCacheHandle;

class IAsyncReadRequest;

using FWwiseFileOperationDone = TUniqueFunction<void(bool bResult)>;
using FWwiseAkFileOperationDone = TUniqueFunction<void(AkAsyncIOTransferInfo* TransferInfo, AKRESULT AkResult)>;

/**
 * Wwise's individual File Cache Handle interface, used to stream an individual file.
 *
 * This is returned by FWwiseFileCache to process streamed files asynchronously.
 *
 * Do not call the destructor. Instead, use CloseAndDelete when necessary.
 */
class WWISEFILEHANDLER_API IWwiseFileCacheHandle
{
public:
	using FBulkDataSharedRef = TSharedRef<FByteBulkData, ESPMode::ThreadSafe>;

	virtual ~IWwiseFileCacheHandle() {}
	virtual void CloseAndDelete() = 0;
	virtual void Open(FWwiseFileOperationDone&& OnDone) = 0;
	virtual void ReadData(uint8* OutBuffer, int64 Offset, int64 BytesToRead, EAsyncIOPriorityAndFlags Priority, FWwiseFileOperationDone&& OnDone) = 0;

	virtual void ReadAkData(uint8* OutBuffer, int64 Offset, int64 BytesToRead, int8 AkPriority, FWwiseFileOperationDone&& OnDone);
	virtual void ReadAkData(const AkIoHeuristics& Heuristics, AkAsyncIOTransferInfo& TransferInfo, FWwiseAkFileOperationDone&& Callback);

	virtual int64 GetFileSize() const = 0;
};

/**
 * Wwise File Cache manager, used to stream files.
 *
 * This is a simple Wwise version of Unreal's complex FFileCache.
 *
 * WwiseFileHandler module already opens any file only once, so we don't need a global cache.
 *
 * Compared to Unreal's FFileCache, we want to process everything asynchronously,
 * including file opening in the unlikely possibility the file is not accessible or present.
 * This interface allows for a fully asynchronous process.
 */
class WWISEFILEHANDLER_API FWwiseFileCache
{
public:
	using FBulkDataSharedRef = IWwiseFileCacheHandle::FBulkDataSharedRef;

	static FWwiseFileCache* Get();

	FWwiseFileCache();
	virtual ~FWwiseFileCache();
	virtual void CreateFileCacheHandle(IWwiseFileCacheHandle*& OutHandle, const FString& Pathname, FWwiseFileOperationDone&& OnDone);
	virtual void CreateFileCacheHandle(IWwiseFileCacheHandle*& OutHandle, const FBulkDataSharedRef& BulkData, FWwiseFileOperationDone&& OnDone);

	FWwiseExecutionQueue OpenQueue;
	FWwiseExecutionQueue DeleteRequestQueue;
};
