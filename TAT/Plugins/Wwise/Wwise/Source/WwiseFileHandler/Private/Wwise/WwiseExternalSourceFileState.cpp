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

#include "Wwise/WwiseExternalSourceFileState.h"
#include "Wwise/WwiseExternalSourceManager.h"
#include "Wwise/WwiseFileCache.h"
#include "Wwise/WwiseStreamingManagerHooks.h"
#include "Wwise/WwiseTask.h"
#include "Wwise/API/WwiseSoundEngineAPI.h"
#include "Wwise/Stats/FileHandlerMemory.h"

#include "Async/MappedFileHandle.h"

#include <inttypes.h>

FWwiseExternalSourceFileState::FWwiseExternalSourceFileState(uint32 InMemoryAlignment, bool bInDeviceMemory,
                                                             uint32 InMediaId, const FName& InMediaPathName, int32 InCodecId) :
	AkExternalSourceInfo(),
	MediaId(InMediaId),
	PlayCount(0)
{
	const auto ExternalSourceManager = IWwiseExternalSourceManager::Get();
	check(ExternalSourceManager);

	idCodec = InCodecId;
	PackagedFile.MemoryAlignment = InMemoryAlignment;
	PackagedFile.bDeviceMemory = bInDeviceMemory;
	if (!ExternalSourceManager) { return; }

	if (UNLIKELY(InMediaPathName.IsNone()))
	{
		PackagedFile.PathName = {};
#if WITH_EDITORONLY_DATA
		PackagedFile.SourcePathName = {};
#endif
	}
	else
	{
		PackagedFile.PathName = FName(ExternalSourceManager->GetStagingDirectory() / InMediaPathName.ToString());
#if WITH_EDITORONLY_DATA
		PackagedFile.SourcePathName = IWwiseExternalSourceManager::Get()->GetExternalSourcePathFor(InMediaPathName);
#endif
	}
	
	INC_DWORD_STAT(STAT_WwiseFileHandlerKnownExternalSourceMedia);
}

FWwiseExternalSourceFileState::~FWwiseExternalSourceFileState()
{
	DEC_DWORD_STAT(STAT_WwiseFileHandlerKnownExternalSourceMedia);
}

bool FWwiseExternalSourceFileState::GetExternalSourceInfo(AkExternalSourceInfo& OutInfo)
{
	OutInfo = static_cast<AkExternalSourceInfo>(*this);
	return szFile != nullptr || pInMemory != nullptr || idFile != 0;
}

void FWwiseExternalSourceFileState::IncrementPlayCount()
{
	++PlayCount;
}


bool FWwiseExternalSourceFileState::DecrementPlayCount()
{
	const auto NewPlayCount = PlayCount.DecrementExchange() - 1;
	if (PlayCount < 0)
	{
		PlayCount.Store(0);
		UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwiseExternalSourceFileState: Play count went below zero for media %" PRIu32 " (%s)"),
		 MediaId, *PackagedFile.PathName.ToString());
	}
	return NewPlayCount == 0;
}

FWwiseInMemoryExternalSourceFileState::FWwiseInMemoryExternalSourceFileState(uint32 InMemoryAlignment, bool bInDeviceMemory,
		uint32 InMediaId, const FName& InMediaPathName, int32 InCodecId) :
	FWwiseExternalSourceFileState(InMemoryAlignment, bInDeviceMemory, InMediaId, InMediaPathName, InCodecId),
	Ptr(nullptr),
	MappedHandle(nullptr),
	MappedRegion(nullptr)
{
#if WITH_EDITOR
	 if (PackagedFile.bDeviceMemory)
	 {
	 	UE_LOG(LogWwiseFileHandler, Warning, TEXT("FWwiseExternalSourceFileState: Loading External Source Media with DeviceMemory=true while in in editor. Expect to see \"No Device Memory\" errors in the log."));
	 }
#endif
}

void FWwiseInMemoryExternalSourceFileState::OpenFile(FOpenFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryExternalSourceFileState::OpenFile"));
	if (UNLIKELY(uiMemorySize || pInMemory))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemoryExternalSourceFileState::OpenFile %" PRIu32 " (%s): Seems to be already opened."), MediaId, *PackagedFile.PathName.ToString());
		return OpenFileFailed(MoveTemp(InCallback));
	}

	PackagedFile.GetFullFileToPtr([this, InCallback = MoveTemp(InCallback)](bool bInResult, const uint8* InPtr, int64 InSize) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryExternalSourceFileState::OpenFile Callback"));
		if (LIKELY(bInResult))
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemoryExternalSourceFileState::OpenFile %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
			pInMemory = const_cast<uint8*>(InPtr);
			uiMemorySize = InSize;
			return OpenFileSucceeded(MoveTemp(InCallback));
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemoryExternalSourceFileState::OpenFile %" PRIu32 ": Failed to open In-Memory External Source (%s)."), MediaId, *PackagedFile.PathName.ToString());
			pInMemory = nullptr;
			uiMemorySize = 0;
			return OpenFileFailed(MoveTemp(InCallback));
		}
	},
	true,
	STAT_WwiseMemoryExtSrc_FName, STAT_WwiseMemoryExtSrcDevice_FName, WWISE_LLM_GET_NAME(Audio_Wwise_FileHandler_ExternalSources));
}

void FWwiseInMemoryExternalSourceFileState::LoadInSoundEngine(FLoadInSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryExternalSourceFileState::LoadInSoundEngine"));
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemoryExternalSourceFileState::LoadInSoundEngine %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
	INC_DWORD_STAT(STAT_WwiseFileHandlerLoadedExternalSourceMedia);
	LoadInSoundEngineSucceeded(MoveTemp(InCallback));
}

void FWwiseInMemoryExternalSourceFileState::UnloadFromSoundEngine(FUnloadFromSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryExternalSourceFileState::UnloadFromSoundEngine"));
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemoryExternalSourceFileState::UnloadFromSoundEngine %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
	DEC_DWORD_STAT(STAT_WwiseFileHandlerLoadedExternalSourceMedia);
	UnloadFromSoundEngineDone(MoveTemp(InCallback));
}

void FWwiseInMemoryExternalSourceFileState::CloseFile(FCloseFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemoryExternalSourceFileState::CloseFile"));
	UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemoryExternalSourceFileState::CloseFile %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
	PackagedFile.DeallocateMemory(const_cast<const uint8*&>(reinterpret_cast<uint8*&>(pInMemory)), uiMemorySize, true, STAT_WwiseMemoryExtSrc_FName, STAT_WwiseMemoryExtSrcDevice_FName);
	pInMemory = nullptr;
	uiMemorySize = 0;
	CloseFileDone(MoveTemp(InCallback));
}

FWwiseStreamedExternalSourceFileState::FWwiseStreamedExternalSourceFileState(uint32 InMemoryAlignment, bool bInDeviceMemory,
	uint32 InPrefetchSize, uint32 InStreamingGranularity,
	uint32 InMediaId, const FName& InMediaPathName, int32 InCodecId) :
	FWwiseExternalSourceFileState(InMemoryAlignment, bInDeviceMemory, InMediaId, InMediaPathName, InCodecId),
	PrefetchSize(InPrefetchSize),
	StreamingGranularity(InStreamingGranularity),
	StreamedFile(nullptr)
{
	sourceID = InMediaId;
	pMediaMemory = nullptr;
	uMediaSize = 0;

	idFile = InMediaId;
}

void FWwiseStreamedExternalSourceFileState::CloseStreaming()
{
	auto ExternalSourceManager = IWwiseExternalSourceManager::Get();
	if (UNLIKELY(!ExternalSourceManager))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseStreamedExternalSourceFileState::CloseStreaming %" PRIu32 " (%s): Closing without an ExternalSourceManager."), MediaId, *PackagedFile.PathName.ToString());
		return;
	}
	ExternalSourceManager->GetStreamingHooks().CloseStreaming(MediaId, *this);
}

void FWwiseStreamedExternalSourceFileState::OpenFile(FOpenFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedExternalSourceFileState::OpenFile"));
	if (UNLIKELY(iFileSize != 0 || StreamedFile))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedExternalSourceFileState::OpenFile %" PRIu32 " (%s): Stream seems to be already opened."), MediaId, *PackagedFile.PathName.ToString());
		return OpenFileFailed(MoveTemp(InCallback));
	}

	if (PrefetchSize == 0)
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedExternalSourceFileState::OpenFile %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
		return OpenFileSucceeded(MoveTemp(InCallback));
	}

	PackagedFile.GetPrefetchToPtr([this, InCallback = MoveTemp(InCallback)](bool bResult, const uint8* InPtr, int64 InSize) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedExternalSourceFileState::OpenFile Callback"));
		if (UNLIKELY(!bResult))
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedExternalSourceFileState::OpenFile %" PRIu32 " (%s): Failed to Read prefetch ExternalSource."), MediaId, *PackagedFile.PathName.ToString());
			pMediaMemory = nullptr;
			return OpenFileFailed(MoveTemp(InCallback));
		}
		INC_DWORD_STAT(STAT_WwiseFileHandlerPrefetchedExternalSourceMedia);
		uMediaSize = InSize;

		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedExternalSourceFileState::OpenFile %" PRIu32 " (%s): Prefetched %" PRIu32 " bytes."), MediaId, *PackagedFile.PathName.ToString(), uMediaSize);
		return OpenFileSucceeded(MoveTemp(InCallback));
	},
	false,
	STAT_WwiseMemoryExtSrcPrefetch_FName, STAT_WwiseMemoryExtSrcPrefetchDevice_FName, WWISE_LLM_GET_NAME(Audio_Wwise_FileHandler_ExternalSources),
	AIOP_Normal);
}

void FWwiseStreamedExternalSourceFileState::LoadInSoundEngine(FLoadInSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedExternalSourceFileState::LoadInSoundEngine"));
	if (UNLIKELY(iFileSize != 0 || StreamedFile))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedExternalSourceFileState::LoadInSoundEngine %" PRIu32 " (%s): Stream seems to be already loaded."), MediaId, *PackagedFile.PathName.ToString());
		return LoadInSoundEngineFailed(MoveTemp(InCallback));
	}


	LLM_SCOPE_BYTAG(Audio_Wwise_FileHandler_ExternalSources);
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedExternalSourceFileState::LoadInSoundEngine %" PRIu32 " (%s): Opening file"), MediaId, *PackagedFile.PathName.ToString());
	PackagedFile.StreamFile([this, Callback = MoveTemp(InCallback)](IWwiseFileCacheHandle* Result) mutable
	{
		if (UNLIKELY(!Result))
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedExternalSourceFileState::LoadInSoundEngine %" PRIu32 ": Failed to load Streaming ExternalSource (%s)."), MediaId, *PackagedFile.PathName.ToString());
			delete StreamedFile;
			StreamedFile = nullptr;
			return LoadInSoundEngineFailed(MoveTemp(Callback));
		}

		StreamedFile = Result;
		iFileSize = StreamedFile->GetFileSize();
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedExternalSourceFileState::LoadInSoundEngine %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
		INC_DWORD_STAT(STAT_WwiseFileHandlerLoadedExternalSourceMedia);
		return LoadInSoundEngineSucceeded(MoveTemp(Callback));
	});
}

void FWwiseStreamedExternalSourceFileState::UnloadFromSoundEngine(FUnloadFromSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedExternalSourceFileState::UnloadFromSoundEngine"));
	UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedExternalSourceFileState::UnloadFromSoundEngine %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());

	if (StreamedFile)
	{
		StreamedFile->CloseAndDelete();
		StreamedFile = nullptr;
	}
	iFileSize = 0;

	DEC_DWORD_STAT(STAT_WwiseFileHandlerLoadedExternalSourceMedia);
	UnloadFromSoundEngineDone(MoveTemp(InCallback));
}

void FWwiseStreamedExternalSourceFileState::CloseFile(FCloseFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseStreamedExternalSourceFileState::CloseFile"));
	if (pMediaMemory == nullptr)
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedExternalSourceFileState::CloseFile %" PRIu32 " (%s)"), MediaId, *PackagedFile.PathName.ToString());
		return CloseFileDone(MoveTemp(InCallback));
	}

	UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseStreamedExternalSourceFileState::CloseFile %" PRIu32 " (%s): Unloaded prefetch."), MediaId, *PackagedFile.PathName.ToString());
	PackagedFile.DeallocateMemory(pMediaMemory, uMediaSize, true, STAT_WwiseMemoryExtSrcPrefetch_FName, STAT_WwiseMemoryExtSrcPrefetchDevice_FName);
	pMediaMemory = nullptr;
	uMediaSize = 0;
	DEC_DWORD_STAT(STAT_WwiseFileHandlerPrefetchedExternalSourceMedia);
	return CloseFileDone(MoveTemp(InCallback));
}

bool FWwiseStreamedExternalSourceFileState::CanProcessFileOp() const
{
	if (UNLIKELY(State != EState::Loaded))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseStreamedExternalSourceFileState::CanProcessFileOp %" PRIu32 " (%s): IO Hook asked for a file operation, but state is not ready."), MediaId, *PackagedFile.PathName.ToString());
		return false;
	}
	return true;
}

AKRESULT FWwiseStreamedExternalSourceFileState::ProcessRead(AkFileDesc& InFileDesc, const AkIoHeuristics& InHeuristics, AkAsyncIOTransferInfo& OutTransferInfo, FWwiseAkFileOperationDone&& InFileOpDoneCallback)
{
	if (pMediaMemory && OutTransferInfo.uFilePosition + OutTransferInfo.uRequestedSize <= uMediaSize)
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedExternalSourceFileState::ProcessRead: Reading prefetch %" PRIu32 " bytes @ %" PRIu64 " in file %" PRIu32 " (%s)"),
			OutTransferInfo.uRequestedSize, OutTransferInfo.uFilePosition, MediaId, *PackagedFile.PathName.ToString());
		FMemory::Memcpy(OutTransferInfo.pBuffer, pMediaMemory + OutTransferInfo.uFilePosition, OutTransferInfo.uRequestedSize);
		SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseStreamedExternalSourceFileState::ProcessRead Callback"));
		InFileOpDoneCallback(&OutTransferInfo, AK_Success);
		return AK_Success;
	}

	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseStreamedExternalSourceFileState::ProcessRead: Reading %" PRIu32 " bytes @ %" PRIu64 " in file %" PRIu32 " (%s)"),
		OutTransferInfo.uRequestedSize, OutTransferInfo.uFilePosition, MediaId, *PackagedFile.PathName.ToString());

	StreamedFile->ReadAkData(InHeuristics, OutTransferInfo, MoveTemp(InFileOpDoneCallback));
	return AK_Success;
}
