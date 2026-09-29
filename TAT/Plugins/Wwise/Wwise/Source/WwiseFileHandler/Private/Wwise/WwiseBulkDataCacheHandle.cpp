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

#include "Wwise/WwiseBulkDataCacheHandle.h"

#include "Wwise/WwiseExecutionQueue.h"
#include "Wwise/WwiseFileHandlerModule.h"
#include "Wwise/Stats/AsyncStats.h"
#include "Wwise/Stats/FileHandler.h"
#include "WwiseUnrealDefines.h"

#include "Async/Async.h"
#include "Async/AsyncFileHandle.h"
#include "HAL/PlatformFileManager.h"

#include <inttypes.h>

FWwiseBulkDataCacheHandle::FWwiseBulkDataCacheHandle(const FBulkDataSharedRef& InBulkData) :
	BulkData { InBulkData }
{
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseBulkDataCacheHandle::FWwiseBulkDataCacheHandle (%p) Creating %s."), this, *GetDebugName());
	INC_DWORD_STAT(STAT_WwiseFileHandlerOpenedStreams);
}

FWwiseBulkDataCacheHandle::~FWwiseBulkDataCacheHandle()
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseBulkDataCacheHandle::~FWwiseBulkDataCacheHandle"));

	const auto NumRequests = RequestsInFlight.load(std::memory_order_seq_cst);
	DEC_DWORD_STAT(STAT_WwiseFileHandlerOpenedStreams);
	if (LIKELY(NumRequests == 0))
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseBulkDataCacheHandle::~FWwiseBulkDataCacheHandle (%p): Closing %s."), this, *GetDebugName());
	}
	else
	{
		UE_CLOG(NumRequests > 0, LogWwiseFileHandler, Log, TEXT("FWwiseBulkDataCacheHandle::~FWwiseBulkDataCacheHandle (%p): Closing %s with %" PRIi32 " operation(s) left to process! Use CloseAndDelete!"), this, *GetDebugName(), NumRequests);
		UE_CLOG(NumRequests < 0, LogWwiseFileHandler, Log, TEXT("FWwiseBulkDataCacheHandle::~FWwiseBulkDataCacheHandle (%p): Closing %s and leaking."), this, *GetDebugName());
	}
}

void FWwiseBulkDataCacheHandle::CloseAndDelete()
{
	const auto FileCache = FWwiseFileCache::Get();
	if (LIKELY(FileCache))
	{
		FileCache->DeleteRequestQueue.AsyncAlways(WWISEFILEHANDLER_ASYNC_NAME("FWwiseBulkDataCacheHandle::DeleteRequest"), [this]() mutable
		{
			OnCloseAndDelete();
		});
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseBulkDataCacheHandle::CloseAndDelete (%p): Closing Request for %s after FileCache module is destroyed. Will leak."), this, *GetDebugName());
	}
}

void FWwiseBulkDataCacheHandle::Open(FWwiseFileOperationDone&& OnDone)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseBulkDataCacheHandle::Open"));
	CallDone(BulkData->IsBulkDataLoaded() || BulkData->CanLoadFromDisk(), MoveTemp(OnDone));
}

void FWwiseBulkDataCacheHandle::CallDone(bool bResult, FWwiseFileOperationDone&& OnDone)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseBulkDataCacheHandle::CallDone Callback"));
	OnDone(bResult);
}

FString FWwiseBulkDataCacheHandle::GetDebugName() const
{
#if UE_5_2_OR_LATER
	return BulkData->GetDebugName();
#else
	static const FString EmptyDebugName {"Bulk Data"};
	return EmptyDebugName;
#endif
}

void FWwiseBulkDataCacheHandle::DeleteRequest(IBulkDataIORequest* Request)
{
	const auto FileCache = FWwiseFileCache::Get();
	if (LIKELY(FileCache))
	{
		FileCache->DeleteRequestQueue.AsyncAlways(WWISEFILEHANDLER_ASYNC_NAME("FWwiseBulkDataCacheHandle::DeleteRequest"), [this, Request]() mutable
		{
			OnDeleteRequest(Request);
		});
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseBulkDataCacheHandle::DeleteRequest (%p): Deleting Request for %s after FileCache module is destroyed. Will leak."), this, *GetDebugName());
		if (--RequestsInFlight == 0)
		{
			RequestsInFlight = -1;
		}
	}
}

void FWwiseBulkDataCacheHandle::OnDeleteRequest(IBulkDataIORequest* Request)
{
	if (Request && !Request->WaitCompletion(1))
	{
		return DeleteRequest(Request);
	}

	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseBulkDataCacheHandle::DeleteRequest (%p req:%p) [%" PRIi32 "]: Deleting request."), this, Request, FPlatformTLS::GetCurrentThreadId());
	delete Request;
	RemoveRequestInFlight();
}

void FWwiseBulkDataCacheHandle::RemoveRequestInFlight()
{
	const auto FileCache = FWwiseFileCache::Get();
	if (LIKELY(FileCache))
	{
		FileCache->DeleteRequestQueue.AsyncAlways(WWISEFILEHANDLER_ASYNC_NAME("FWwiseBulkDataCacheHandle::RemoveRequestInFlight"), [this]() mutable
		{
			--RequestsInFlight;
		});
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseBulkDataCacheHandle::RemoveRequestInFlight (%p): Removing Request for %s after FileCache module is destroyed. Will leak."), this, *GetDebugName());
		if (--RequestsInFlight == 0)
		{
			RequestsInFlight = -1;
		}
	}
}

void FWwiseBulkDataCacheHandle::OnCloseAndDelete()
{
	if (RequestsInFlight > 0)
	{
		return CloseAndDelete();
	}
	delete this;
}

void FWwiseBulkDataCacheHandle::ReadData(uint8* OutBuffer, int64 Offset, int64 BytesToRead,
	EAsyncIOPriorityAndFlags Priority, FWwiseFileOperationDone&& OnDone)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseBulkDataCacheHandle::ReadData"));
	FWwiseAsyncCycleCounter Stat(GET_STATID(STAT_WwiseFileHandlerFileOperationLatency));

	++RequestsInFlight;

	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseBulkDataCacheHandle::ReadData (%p): %" PRIi64 "@%" PRIi64 " in %s"), this, BytesToRead, Offset, *GetDebugName());
	FBulkDataIORequestCallBack ReadCallbackFunction = [this, OnDone = new FWwiseFileOperationDone(MoveTemp(OnDone)), BytesToRead, Stat = MoveTemp(Stat)](bool bWasCancelled, IBulkDataIORequest* Request) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseBulkDataCacheHandle::ReadData Callback"));
		if (!bWasCancelled && Request)	// Do not add Request->GetReadResults() since it will break subsequent results retrievals.
		{
			ASYNC_INC_FLOAT_STAT_BY(STAT_WwiseFileHandlerTotalStreamedMB, static_cast<float>(BytesToRead) / 1024 / 1024);
		}
		OnReadDataDone(bWasCancelled, Request, MoveTemp(*OnDone));
		delete OnDone;
		DeleteRequest(Request);
	};
	ASYNC_INC_FLOAT_STAT_BY(STAT_WwiseFileHandlerStreamingKB, static_cast<float>(BytesToRead) / 1024);
	check(BytesToRead > 0);

#if STATS
	switch (Priority & EAsyncIOPriorityAndFlags::AIOP_PRIORITY_MASK)
	{
	case EAsyncIOPriorityAndFlags::AIOP_CriticalPath: INC_DWORD_STAT(STAT_WwiseFileHandlerCriticalPriority); break;
	case EAsyncIOPriorityAndFlags::AIOP_High: INC_DWORD_STAT(STAT_WwiseFileHandlerHighPriority); break;
	case EAsyncIOPriorityAndFlags::AIOP_BelowNormal: INC_DWORD_STAT(STAT_WwiseFileHandlerBelowNormalPriority); break;
	case EAsyncIOPriorityAndFlags::AIOP_Low: INC_DWORD_STAT(STAT_WwiseFileHandlerLowPriority); break;
	case EAsyncIOPriorityAndFlags::AIOP_MIN: INC_DWORD_STAT(STAT_WwiseFileHandlerBackgroundPriority); break;

	default:
	case EAsyncIOPriorityAndFlags::AIOP_Normal: INC_DWORD_STAT(STAT_WwiseFileHandlerNormalPriority); break;
	}
#endif

	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseBulkDataCacheHandle::ReadData Async ReadRequest"));
	const auto* Request = BulkData->CreateStreamingRequest(Offset, BytesToRead, Priority, &ReadCallbackFunction, OutBuffer);
	if (UNLIKELY(!Request))
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseBulkDataCacheHandle::ReadData (%p): ReadRequest %s failed instantiating."), this, *GetDebugName());
		ReadCallbackFunction(true, nullptr);
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseBulkDataCacheHandle::ReadData (%p req:%p) [%" PRIi32 "]: Created request"), this, Request, FPlatformTLS::GetCurrentThreadId());
	}
}

void FWwiseBulkDataCacheHandle::OnReadDataDone(bool bWasCancelled, IBulkDataIORequest* Request,
	FWwiseFileOperationDone&& OnDone)
{
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseBulkDataCacheHandle::OnReadDataDone (%p req:%p) [%" PRIi32 "]: Request done."), this, Request, FPlatformTLS::GetCurrentThreadId());
	OnReadDataDone(!bWasCancelled && Request && Request->GetReadResults(), MoveTemp(OnDone));
}

void FWwiseBulkDataCacheHandle::OnReadDataDone(bool bResult, FWwiseFileOperationDone&& OnDone)
{
	CallDone(bResult, MoveTemp(OnDone));
}
