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

#include "Wwise/WwiseMediaFileState.h"
#include "Wwise/WwiseMediaManager.h"
#include "Wwise/WwiseSoundEngineUtils.h"
#include "Wwise/WwiseStreamingManagerHooks.h"
#include "Wwise/WwiseTask.h"
#include "Wwise/API/WwiseSoundEngineAPI.h"
#include "Wwise/Stats/FileHandlerMemory.h"

#include "WwiseUnrealHelper.h"

#include "Async/MappedFileHandle.h"

#include <inttypes.h>

FWwiseMediaFileState::FWwiseMediaFileState(const FWwiseMediaCookedData& InCookedData) :
	FWwiseMediaCookedData(InCookedData)
{
	INC_DWORD_STAT(STAT_WwiseFileHandlerKnownMedia);
}

FWwiseMediaFileState::~FWwiseMediaFileState()
{
	DEC_DWORD_STAT(STAT_WwiseFileHandlerKnownMedia);
}

FWwiseInMemoryMediaFileState::FWwiseInMemoryMediaFileState(const FWwiseMediaCookedData& InCookedData) :
	FWwiseMediaFileState(InCookedData)
{
	pMediaMemory = nullptr;
	sourceID = MediaId;
	uMediaSize = 0;
}

void FWwiseInMemoryMediaFileState::OpenFile(FOpenFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryMediaFileState::OpenFile"));
	if (UNLIKELY(uMediaSize || pMediaMemory))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemoryMediaFileState::OpenFile %" PRIu32 " (%s): Seems to be already opened."), MediaId, *DebugName.ToString());
		return OpenFileFailed(MoveTemp(InCallback));
	}

	PackagedFile.GetFullFileToPtr([this, InCallback = MoveTemp(InCallback)](bool bInResult, const uint8* Ptr, int64 Size) mutable
	{
		if (LIKELY(bInResult))
		{
			UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemoryMediaFileState::OpenFile %" PRIu32 " (%s)"), MediaId, *DebugName.ToString());
			pMediaMemory = const_cast<uint8*>(Ptr);
			uMediaSize = Size;
			return OpenFileSucceeded(MoveTemp(InCallback));
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemoryMediaFileState::OpenFile %" PRIu32 " (%s): Failed to open In-Memory Media (%s)."), MediaId, *DebugName.ToString(), *PackagedFile.PathName.ToString());
			pMediaMemory = nullptr;
			Size = 0;
			return OpenFileFailed(MoveTemp(InCallback));
		}
	},
	true, STAT_WwiseMemoryMedia_FName, STAT_WwiseMemoryMediaDevice_FName, WWISE_LLM_GET_NAME(Audio_Wwise_FileHandler_Media));
}

void FWwiseInMemoryMediaFileState::LoadInSoundEngine(FLoadInSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryMediaFileState::LoadInSoundEngine"));
	if (UNLIKELY(!uMediaSize || !pMediaMemory))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemoryMediaFileState::LoadInSoundEngine %" PRIu32 " (%s): No data, but supposed to be loaded."), MediaId, *DebugName.ToString());
		return LoadInSoundEngineFailed(MoveTemp(InCallback));
	}

	auto MediaManager = IWwiseMediaManager::Get();
	if (UNLIKELY(!MediaManager))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemoryMediaFileState::LoadInSoundEngine %" PRIu32 " (%s): Failed loading without the MediaManager."), MediaId, *DebugName.ToString());
		LoadInSoundEngineFailed(MoveTemp(InCallback));
		return;
	}

	MediaManager->SetMedia(*this, [this, InCallback = MoveTemp(InCallback)](bool bResult) mutable
	{
		if (LIKELY(bResult))
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemoryMediaFileState::LoadInSoundEngine: Loaded: %" PRIu32 " (%s) @ %p %" PRIu32 " bytes."), MediaId, *DebugName.ToString(), pMediaMemory, uMediaSize);
			INC_DWORD_STAT(STAT_WwiseFileHandlerLoadedMedia);
			return LoadInSoundEngineSucceeded(MoveTemp(InCallback));
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemoryMediaFileState::LoadInSoundEngine %" PRIu32 " (%s): Failed to load Media."), MediaId, *DebugName.ToString());
			return LoadInSoundEngineFailed(MoveTemp(InCallback));
		}
	});	
}

void FWwiseInMemoryMediaFileState::UnloadFromSoundEngine(FUnloadFromSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryMediaFileState::UnloadFromSoundEngine"));

	auto MediaManager = IWwiseMediaManager::Get();
	if (UNLIKELY(!MediaManager))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemoryMediaFileState::UnloadFromSoundEngine %" PRIu32 " (%s): Failed unloading media without the MediaManager."), MediaId, *DebugName.ToString());
		return CloseFileDone(MoveTemp(InCallback));
	}

	MediaManager->UnsetMedia(*this, [this, InCallback = MoveTemp(InCallback)](bool bResult) mutable
	{
		if (UNLIKELY(!bResult))
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemoryMediaFileState::UnloadFromSoundEngine %" PRIu32 " (%s): Deferred (AK_ResourceInUse)."), MediaId, *DebugName.ToString());
			return UnloadFromSoundEngineDefer(MoveTemp(InCallback));
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemoryMediaFileState::UnloadFromSoundEngine: Unloaded: %" PRIu32 " (%s) @ %p %" PRIu32 " bytes."), MediaId, *DebugName.ToString(), pMediaMemory, uMediaSize);
			DEC_DWORD_STAT(STAT_WwiseFileHandlerLoadedMedia);
			return UnloadFromSoundEngineDone(MoveTemp(InCallback));
		}
	});
}

void FWwiseInMemoryMediaFileState::CloseFile(FCloseFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryMediaFileState::CloseFile"));
	UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemoryMediaFileState::CloseFile: Unloaded: %" PRIu32 " (%s). Deallocating @ %p %" PRIu32 " bytes."), MediaId, *DebugName.ToString(), pMediaMemory, uMediaSize);
	PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryMedia_FName, STAT_WwiseMemoryMediaDevice_FName);
	pMediaMemory = nullptr;
	uMediaSize = 0;
	CloseFileDone(MoveTemp(InCallback));
}

FWwiseStreamedMediaFileState::FWwiseStreamedMediaFileState(const FWwiseMediaCookedData& InCookedData,
	uint32 InStreamingGranularity) :
	FWwiseMediaFileState(InCookedData),
	StreamingGranularity(InStreamingGranularity),
	StreamedFile(nullptr)
{
	sourceID = InCookedData.MediaId;
	pMediaMemory = nullptr;
	uMediaSize = 0;
}

void FWwiseStreamedMediaFileState::CloseStreaming()
{
	auto MediaManager = IWwiseMediaManager::Get();
	if (UNLIKELY(!MediaManager))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseStreamedMediaFileState::CloseStreaming %" PRIu32 " (%s): Closing without a MediaManager."), MediaId, *DebugName.ToString());
		return;
	}
	MediaManager->GetStreamingHooks().CloseStreaming(MediaId, *this);
}

void FWwiseStreamedMediaFileState::OpenFile(FOpenFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedMediaFileState::OpenFile"));
	if (UNLIKELY(iFileSize != 0 || StreamedFile))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s): Stream seems to be already opened."), MediaId, *DebugName.ToString());
		return OpenFileFailed(MoveTemp(InCallback));
	}

	if (PackagedFile.PrefetchSize == 0)
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s)"), MediaId, *DebugName.ToString());
		return OpenFileSucceeded(MoveTemp(InCallback));
	}

	// Process PrefetchSize and send as SetMedia
	PackagedFile.GetPrefetchToPtr([this, InCallback = MoveTemp(InCallback)](bool bResult, const uint8* Ptr, int64 Size) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedMediaFileState::OpenFile Callback"));

		if (UNLIKELY(!bResult))
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s): Failed to Read prefetch media (%s)."), MediaId, *DebugName.ToString(), *PackagedFile.PathName.ToString());
			pMediaMemory = nullptr;
			return OpenFileFailed(MoveTemp(InCallback));
		}
		pMediaMemory = const_cast<uint8*>(Ptr);
		uMediaSize = Size;

		auto* BankExecutionQueue = GetBankExecutionQueue();
		if (UNLIKELY(!BankExecutionQueue))
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s): Failed prefetch media without the FileHandler module."), MediaId, *DebugName.ToString());
			PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryMediaPrefetch_FName, STAT_WwiseMemoryMediaPrefetchDevice_FName);
			pMediaMemory = nullptr;
			uMediaSize = 0;
			return OpenFileFailed(MoveTemp(InCallback));
		}

		BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseStreamedMediaFileState::OpenFile SetMedia"), [this, InCallback = MoveTemp(InCallback)]() mutable
		{
			auto* SoundEngine = IWwiseSoundEngineAPI::Get();
			if (UNLIKELY(!SoundEngine))
			{
				UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s): Failed prefetch media without a SoundEngine."), MediaId, *DebugName.ToString());
				PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryMediaPrefetch_FName, STAT_WwiseMemoryMediaPrefetchDevice_FName);
				pMediaMemory = nullptr;
				uMediaSize = 0;
				return OpenFileFailed(MoveTemp(InCallback));
			}

			const auto SetMediaResult = SoundEngine->SetMedia(this, 1);
			if (LIKELY(SetMediaResult == AK_Success))
			{
				UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s): Prefetched @ %p %" PRIu32 " bytes."), MediaId, *DebugName.ToString(), pMediaMemory, uMediaSize);
				INC_DWORD_STAT(STAT_WwiseFileHandlerPrefetchedMedia);
				return OpenFileSucceeded(MoveTemp(InCallback));
			}
			else
			{
				UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::OpenFile %" PRIu32 " (%s): Failed to prefetch media: %d (%s)."), MediaId, *DebugName.ToString(), SetMediaResult, WwiseUnrealHelper::GetResultString(SetMediaResult));
				PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryMediaPrefetch_FName, STAT_WwiseMemoryMediaPrefetchDevice_FName);
				pMediaMemory = nullptr;
				uMediaSize = 0;
				return OpenFileFailed(MoveTemp(InCallback));
			}
		});
	},
		true,
		STAT_WwiseMemoryMediaPrefetch_FName, STAT_WwiseMemoryMediaPrefetchDevice_FName, WWISE_LLM_GET_NAME(Audio_Wwise_FileHandler_Media),
		AIOP_Normal);
}

void FWwiseStreamedMediaFileState::LoadInSoundEngine(FLoadInSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedMediaFileState::LoadInSoundEngine"));
	if (UNLIKELY(iFileSize != 0 || StreamedFile))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::LoadInSoundEngine %" PRIu32 " (%s): Stream seems to be already loaded."), MediaId, *DebugName.ToString());
		return LoadInSoundEngineFailed(MoveTemp(InCallback));
	}


	LLM_SCOPE_BYTAG(Audio_Wwise_FileHandler_Media);
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedMediaFileState::LoadInSoundEngine %" PRIu32 " (%s): Opening file"), MediaId, *DebugName.ToString());
	PackagedFile.StreamFile([this, Callback = MoveTemp(InCallback)](IWwiseFileCacheHandle* Result) mutable
	{
		if (UNLIKELY(!Result))
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::LoadInSoundEngine %" PRIu32 ": Failed to load Streaming Media (%s)."), MediaId, *DebugName.ToString());
			StreamedFile = nullptr;
			return LoadInSoundEngineFailed(MoveTemp(Callback));
		}

		StreamedFile = Result;
		iFileSize = StreamedFile->GetFileSize();
		UE_CLOG(pMediaMemory == nullptr, LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedMediaFileState::LoadInSoundEngine: Loaded: %" PRIu32 " (%s)"), MediaId, *DebugName.ToString());
		UE_CLOG(pMediaMemory != nullptr, LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedMediaFileState::LoadInSoundEngine: Loaded: %" PRIu32 " (%s) with prefetch @ %p %" PRIu32 " bytes."), MediaId, *DebugName.ToString(), pMediaMemory, uMediaSize);
		INC_DWORD_STAT(STAT_WwiseFileHandlerLoadedMedia);
		return LoadInSoundEngineSucceeded(MoveTemp(Callback));
	});
}

void FWwiseStreamedMediaFileState::UnloadFromSoundEngine(FUnloadFromSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedMediaFileState::UnloadFromSoundEngine"));
	UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedMediaFileState::UnloadFromSoundEngine %" PRIu32 " (%s)"), MediaId, *DebugName.ToString());

	if (StreamedFile)
	{
		StreamedFile->CloseAndDelete();
		StreamedFile = nullptr;
	}
	iFileSize = 0;

	DEC_DWORD_STAT(STAT_WwiseFileHandlerLoadedMedia);
	UnloadFromSoundEngineDone(MoveTemp(InCallback));
}

void FWwiseStreamedMediaFileState::CloseFile(FCloseFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedMediaFileState::CloseFile"));
	if (pMediaMemory == nullptr)
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedMediaFileState::CloseFile: Unloaded: %" PRIu32 " (%s)"), MediaId, *DebugName.ToString());
		return CloseFileDone(MoveTemp(InCallback));
	}

	auto* BankExecutionQueue = GetBankExecutionQueue();
	if (UNLIKELY(!BankExecutionQueue))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseStreamedMediaFileState::CloseFile %" PRIu32 " (%s): Failed closing prefetch without the FileHandler module. Leaking."), MediaId, *DebugName.ToString());
		PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryMediaPrefetch_FName, STAT_WwiseMemoryMediaPrefetchDevice_FName);
		pMediaMemory = nullptr;
		uMediaSize = 0;
		return CloseFileDone(MoveTemp(InCallback));
	}

	BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseStreamedMediaFileState::CloseFile TryUnsetMedia"), [this, InCallback = MoveTemp(InCallback)]() mutable
	{
		auto* SoundEngine = IWwiseSoundEngineAPI::Get();
		if (UNLIKELY(!SoundEngine))
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseStreamedMediaFileState::CloseFile %" PRIu32 " (%s): Failed closing prefetch without a SoundEngine. Leaking."), MediaId, *DebugName.ToString());
			pMediaMemory = nullptr;
			uMediaSize = 0;
			return CloseFileDone(MoveTemp(InCallback));
		}

		const auto Result = SoundEngine->TryUnsetMedia(this, 1, nullptr);
		if (UNLIKELY(Result == AK_ResourceInUse))
		{
			return CloseFileDefer(MoveTemp(InCallback));
		}
		else
		{
			if (LIKELY(Result == AK_Success))
			{
				DEC_DWORD_STAT(STAT_WwiseFileHandlerPrefetchedMedia);
				UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedMediaFileState::CloseFile: Unloaded: %" PRIu32 " (%s). Prefetch Deallocating @ %p %" PRIu32 " bytes."), MediaId, *DebugName.ToString(), pMediaMemory, uMediaSize);
				PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryMediaPrefetch_FName, STAT_WwiseMemoryMediaPrefetchDevice_FName);
			}
			else
			{
				UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::CloseFile %" PRIu32 " (%s): TryUnsetMedia of prefetch failed: %d (%s). Leaking."), MediaId, *DebugName.ToString(), Result, WwiseUnrealHelper::GetResultString(Result));
			}
			pMediaMemory = nullptr;
			uMediaSize = 0;
			return CloseFileDone(MoveTemp(InCallback));
		}
	});
}

bool FWwiseStreamedMediaFileState::CanProcessFileOp() const
{
	if (UNLIKELY(State != EState::Loaded))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedMediaFileState::CanProcessFileOp %" PRIu32 " (%s): IO Hook asked for a file operation, but state is not ready."), MediaId, *DebugName.ToString());
		return false;
	}
	return true;
}

AKRESULT FWwiseStreamedMediaFileState::ProcessRead(
	AkFileDesc& InFileDesc, const AkIoHeuristics& InHeuristics,
	AkAsyncIOTransferInfo& OutTransferInfo, FWwiseAkFileOperationDone&& InFileOpDoneCallback)
{
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedMediaFileState::ProcessRead: Reading %" PRIu32 " bytes @ %" PRIu64 " in file %" PRIu32 " (%s)"),
		OutTransferInfo.uRequestedSize, OutTransferInfo.uFilePosition, MediaId, *DebugName.ToString());

	StreamedFile->ReadAkData(InHeuristics, OutTransferInfo, MoveTemp(InFileOpDoneCallback));
	return AK_Success;
}
