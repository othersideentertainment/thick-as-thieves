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

#include "Wwise/WwiseFileSystemCacheHandle.h"

#include "Wwise/WwiseExecutionQueue.h"
#include "Wwise/WwiseFileHandlerModule.h"
#include "Wwise/Stats/AsyncStats.h"
#include "Wwise/Stats/FileHandler.h"
#include "WwiseUnrealDefines.h"

#include "Async/Async.h"
#include "Async/AsyncFileHandle.h"
#include "HAL/PlatformFileManager.h"

#include <inttypes.h>

FWwiseFileSystemCacheHandle::FWwiseFileSystemCacheHandle(const FString& InPathname) :
	Pathname { InPathname },
	FileHandle { nullptr },
	FileSize { 0 },
	InitializationStat { nullptr }
{
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::FWwiseFileSystemCacheHandle (%p) Creating %s."), this, *Pathname);
}

FWwiseFileSystemCacheHandle::~FWwiseFileSystemCacheHandle()
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::~FWwiseFileSystemCacheHandle"));

	const auto* FileHandleToDestroy = FileHandle; FileHandle = nullptr;

	if (FileHandleToDestroy)
	{
		DEC_DWORD_STAT(STAT_WwiseFileHandlerOpenedStreams);
	}
	const auto NumRequests = RequestsInFlight.load(std::memory_order_seq_cst);
	if (LIKELY(NumRequests == 0))
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::~FWwiseFileSystemCacheHandle (%p): Closing %s."), this, *Pathname);
		delete FileHandleToDestroy;
	}
	else
	{
		UE_CLOG(NumRequests > 0, LogWwiseFileHandler, Log, TEXT("FWwiseFileSystemCacheHandle::~FWwiseFileSystemCacheHandle (%p): Closing %s with %" PRIi32 " operation(s) left to process! Use CloseAndDelete!"), this, *Pathname, NumRequests);
		UE_CLOG(NumRequests < 0, LogWwiseFileHandler, Log, TEXT("FWwiseFileSystemCacheHandle::~FWwiseFileSystemCacheHandle (%p): Closing %s and leaking."), this, *Pathname);
	}
}

void FWwiseFileSystemCacheHandle::CloseAndDelete()
{
	int32 CurRequestsInFlight;
	do
	{
		CurRequestsInFlight = RequestsInFlight.load(std::memory_order_seq_cst);
		if (CurRequestsInFlight == 0)
		{
			return OnCloseAndDelete();
		}
	} while (!RequestsInFlight.compare_exchange_strong(
		CurRequestsInFlight, CurRequestsInFlight | DeleteOnLastRequest));
}

void FWwiseFileSystemCacheHandle::Open(FWwiseFileOperationDone&& OnDone)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::Open"));
	check(!InitializationStat);
	check(!FileHandle);

	InitializationStat = new FWwiseAsyncCycleCounter(GET_STATID(STAT_WwiseFileHandlerFileOperationLatency));
	InitializationDone = MoveTemp(OnDone);

	FWwiseAsyncCycleCounter Stat(GET_STATID(STAT_WwiseFileHandlerFileOperationLatency));

	const auto FileCache = FWwiseFileCache::Get();
	if (UNLIKELY(!FileCache))
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseFileSystemCacheHandle::Open (%p): FileCache not available while opening %s."), this, *Pathname);
		delete InitializationStat; InitializationStat = nullptr;
		CallDone(false, MoveTemp(InitializationDone));
		return;
	}

	++RequestsInFlight;
	FileCache->OpenQueue.Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseFileSystemCacheHandle::Open async"), [this, OnDone = MoveTemp(OnDone)]() mutable
	{
		check(!FileHandle);

		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::Open (%p): Opening %s."), this, *Pathname);
		IAsyncReadFileHandle* CurrentFileHandle;
		INC_DWORD_STAT(STAT_WwiseFileHandlerOpenedStreams);
		{
			SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::Open OpenAsyncRead"));
			CurrentFileHandle = FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenAsyncRead(*Pathname);
		}
		if (UNLIKELY(!CurrentFileHandle))
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseFileSystemCacheHandle::Open (%p): OpenAsyncRead %s failed instantiating."), this, *Pathname);
			delete InitializationStat; InitializationStat = nullptr;
			CallDone(false, MoveTemp(InitializationDone));
			RemoveRequestInFlight();
			return;
		}

		FAsyncFileCallBack SizeCallbackFunction = [this](bool bWasCancelled, IAsyncReadRequest* Request) mutable
		{
			OnSizeRequestDone(bWasCancelled, Request);
		};
		IAsyncReadRequest* Request;
		{
			SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::Open SizeRequest"));
			// ++RequestsInFlight; already done
			Request = CurrentFileHandle->SizeRequest(&SizeCallbackFunction);
		}
		if (UNLIKELY(!Request))
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseFileSystemCacheHandle::Open (%p): SizeRequest %s failed instantiating."), this, *Pathname);
			delete InitializationStat; InitializationStat = nullptr;
			CallDone(false, MoveTemp(InitializationDone));
			RemoveRequestInFlight();
		}
	});
}

void FWwiseFileSystemCacheHandle::OnSizeRequestDone(bool bWasCancelled, IAsyncReadRequest* Request)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::OnSizeRequestDone"));
	FileSize = Request->GetSizeResults();

	const bool bSizeOpSuccess = LIKELY(FileSize > 0);

	UE_CLOG(!bSizeOpSuccess, LogWwiseFileHandler, Log, TEXT("FWwiseFileSystemCacheHandle::OnSizeRequestDone (%p): Streamed file \"%s\" could not be opened."), this, *Pathname);
	UE_CLOG(bSizeOpSuccess, LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::OnSizeRequestDone (%p): Initializing %s succeeded."), this, *Pathname);
	delete InitializationStat; InitializationStat = nullptr;
	CallDone(bSizeOpSuccess, MoveTemp(InitializationDone));
	DeleteRequest(Request);
}

void FWwiseFileSystemCacheHandle::CallDone(bool bResult, FWwiseFileOperationDone&& OnDone)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::CallDone Callback"));
	OnDone(bResult);
}

void FWwiseFileSystemCacheHandle::DeleteRequest(IAsyncReadRequest* Request)
{
	LaunchWwiseTask(TEXT("FWwiseFileSystemCacheHandle::OnDeleteRequest"), EWwiseTaskPriority::GameThread,
		[this, Request]() mutable
	{
		OnDeleteRequest(Request);
	});
}

void FWwiseFileSystemCacheHandle::OnDeleteRequest(IAsyncReadRequest* Request)
{
	if (Request && !Request->PollCompletion())
	{
		return DeleteRequest(Request);
	}

	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::DeleteRequest (%p req:%p) [%" PRIi32 "]: Deleting request."), this, Request, FPlatformTLS::GetCurrentThreadId());
	delete Request;
	RemoveRequestInFlight();
}

void FWwiseFileSystemCacheHandle::RemoveRequestInFlight()
{
	auto CurRequestsInFlight { --RequestsInFlight };

	if (CurRequestsInFlight == DeleteOnLastRequest &&
		RequestsInFlight.compare_exchange_strong(CurRequestsInFlight, 0))
	{
		OnCloseAndDelete();
	}
}

void FWwiseFileSystemCacheHandle::OnCloseAndDelete()
{
	if (RequestsInFlight > 0)
	{
		return CloseAndDelete();
	}
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::OnCloseAndDelete (%p) [%" PRIi32 "]: Deleting object."), this, FPlatformTLS::GetCurrentThreadId());
	delete this;
}

void FWwiseFileSystemCacheHandle::ReadData(uint8* OutBuffer, int64 Offset, int64 BytesToRead,
	EAsyncIOPriorityAndFlags Priority, FWwiseFileOperationDone&& OnDone)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::ReadData"));
	FWwiseAsyncCycleCounter Stat(GET_STATID(STAT_WwiseFileHandlerFileOperationLatency));
	++RequestsInFlight;

	IAsyncReadFileHandle* CurrentFileHandle = FileHandle;
	if (UNLIKELY(!CurrentFileHandle))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseFileSystemCacheHandle::ReadData (%p): Trying to read in file %s while it was not properly initialized."), this, *Pathname);
		OnReadDataDone(false, MoveTemp(OnDone));
		return;
	}

	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::ReadData (%p): %" PRIi64 "@%" PRIi64 " in %s"), this, BytesToRead, Offset, *Pathname);
	FAsyncFileCallBack ReadCallbackFunction = [this, OnDone = new FWwiseFileOperationDone(MoveTemp(OnDone)), BytesToRead, Stat = MoveTemp(Stat)](bool bWasCancelled, IAsyncReadRequest* Request) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::ReadData Callback"));
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
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseFileSystemCacheHandle::ReadData Async ReadRequest"));
	const auto* Request = CurrentFileHandle->ReadRequest(Offset, BytesToRead, Priority, &ReadCallbackFunction, OutBuffer);
	if (UNLIKELY(!Request))
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseFileSystemCacheHandle::ReadData (%p): ReadRequest %s failed instantiating."), this, *Pathname);
		ReadCallbackFunction(true, nullptr);
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::ReadData (%p req:%p) [%" PRIi32 "]: Created request"), this, Request, FPlatformTLS::GetCurrentThreadId());
	}
}

void FWwiseFileSystemCacheHandle::OnReadDataDone(bool bWasCancelled, IAsyncReadRequest* Request,
	FWwiseFileOperationDone&& OnDone)
{
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseFileSystemCacheHandle::OnReadDataDone (%p req:%p) [%" PRIi32 "]: Request done."), this, Request, FPlatformTLS::GetCurrentThreadId());
	OnReadDataDone(!bWasCancelled && Request && Request->GetReadResults(), MoveTemp(OnDone));
}

void FWwiseFileSystemCacheHandle::OnReadDataDone(bool bResult, FWwiseFileOperationDone&& OnDone)
{
	CallDone(bResult, MoveTemp(OnDone));
}
