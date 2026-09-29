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

#include "Wwise/WwiseFileCache.h"

#include "Wwise/WwiseBulkDataCacheHandle.h"
#include "Wwise/WwiseFileHandlerModule.h"
#include "Wwise/WwiseFileSystemCacheHandle.h"

#include <inttypes.h>


void IWwiseFileCacheHandle::ReadAkData(uint8* OutBuffer, int64 Offset, int64 BytesToRead, int8 AkPriority, FWwiseFileOperationDone&& OnDone)
{
	// Wwise priority is what we expect our priority to be. Audio will skip if "our normal" is not met.
	constexpr const auto bHigherAudioPriority = true;

	EAsyncIOPriorityAndFlags Priority;
	if (LIKELY(AkPriority == AK_DEFAULT_PRIORITY))
	{
		Priority = bHigherAudioPriority ? AIOP_High : AIOP_Normal;
	}
	else if (AkPriority <= AK_MIN_PRIORITY)
	{
		Priority = bHigherAudioPriority ? AIOP_BelowNormal : AIOP_Low;
	}
	else if (AkPriority >= AK_MAX_PRIORITY)
	{
		Priority = AIOP_CriticalPath;
	}
	else if (AkPriority < AK_DEFAULT_PRIORITY)
	{
		Priority = bHigherAudioPriority ? AIOP_Normal : AIOP_Low;
	}
	else
	{
		Priority = bHigherAudioPriority ? AIOP_CriticalPath : AIOP_High;
	}
	ReadData(OutBuffer, Offset, BytesToRead, Priority, MoveTemp(OnDone));
}

void IWwiseFileCacheHandle::ReadAkData(const AkIoHeuristics& Heuristics, AkAsyncIOTransferInfo& TransferInfo,
	FWwiseAkFileOperationDone&& Callback)
{
	ReadAkData(
		static_cast<uint8*>(TransferInfo.pBuffer),
		static_cast<int64>(TransferInfo.uFilePosition),
		static_cast<int64>(TransferInfo.uRequestedSize),
		Heuristics.priority,
		[TransferInfo = &TransferInfo, FileOpDoneCallback = MoveTemp(Callback)](bool bResult)
		{
			FileOpDoneCallback(TransferInfo, bResult ? AK_Success : AK_UnknownFileError);
		});
}

FWwiseFileCache* FWwiseFileCache::Get()
{
	if (auto* Module = IWwiseFileHandlerModule::GetModule())
	{
		if (auto* FileCache = Module->GetFileCache())
		{
			return FileCache;
		}
	}
	return nullptr;
}

FWwiseFileCache::FWwiseFileCache():
	OpenQueue(TEXT("FWwiseFileCache OpenQueue"), EWwiseTaskPriority::BackgroundHigh),
	DeleteRequestQueue(TEXT("FWwiseFileCache DeleteRequestQueue"), EWwiseTaskPriority::BackgroundLow)
{
}

FWwiseFileCache::~FWwiseFileCache()
{
}

void FWwiseFileCache::CreateFileCacheHandle(
	IWwiseFileCacheHandle*& OutHandle,
	const FString& Pathname,
	FWwiseFileOperationDone&& OnDone)
{
	OutHandle = new FWwiseFileSystemCacheHandle(Pathname);
	if (UNLIKELY(!OutHandle))
	{
		OnDone(false);
	}
	OutHandle->Open(MoveTemp(OnDone));
}

void FWwiseFileCache::CreateFileCacheHandle(IWwiseFileCacheHandle*& OutHandle,
	const FBulkDataSharedRef& BulkData, FWwiseFileOperationDone&& OnDone)
{
	OutHandle = new FWwiseBulkDataCacheHandle(BulkData);
	if (UNLIKELY(!OutHandle))
	{
		OnDone(false);
	}
	OutHandle->Open(MoveTemp(OnDone));
}
