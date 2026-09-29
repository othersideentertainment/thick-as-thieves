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

#include "Wwise/WwiseSoundBankFileState.h"
#include "Wwise/WwiseSoundEngineUtils.h"
#include "Wwise/API/WwiseSoundEngineAPI.h"
#include "Wwise/Stats/FileHandler.h"
#include "Wwise/Stats/FileHandlerMemory.h"
#include "Async/MappedFileHandle.h"
#include "Async/Async.h"

#include <inttypes.h>

FWwiseSoundBankFileState::FWwiseSoundBankFileState(const FWwiseSoundBankCookedData& InCookedData):
	FWwiseSoundBankCookedData(InCookedData)
{
	INC_DWORD_STAT(STAT_WwiseFileHandlerKnownSoundBanks);
}

FWwiseSoundBankFileState::~FWwiseSoundBankFileState()
{
	DEC_DWORD_STAT(STAT_WwiseFileHandlerKnownSoundBanks);
}

FWwiseInMemorySoundBankFileState::FWwiseInMemorySoundBankFileState(const FWwiseSoundBankCookedData& InCookedData) :
	FWwiseSoundBankFileState(InCookedData),
	Ptr(nullptr),
	FileSize(0)
{
}

bool FWwiseInMemorySoundBankFileState::LoadAsMemoryView() const
{
	return bContainsMedia;
}

void FWwiseInMemorySoundBankFileState::OpenFile(FOpenFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemorySoundBankFileState::OpenFile"));
	if (UNLIKELY(FileSize != 0 || Ptr))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::OpenFile %" PRIu32 " (%s): Seems to be already opened."), SoundBankId, *DebugName.ToString());
		OpenFileFailed(MoveTemp(InCallback));
		return;
	}

	PackagedFile.GetFullFileToPtr([this, InCallback = MoveTemp(InCallback)](bool bInResult, const uint8* InPtr, int64 InSize) mutable
	{
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemorySoundBankFileState::OpenFile Callback"));
		if (LIKELY(bInResult))
		{
			UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemorySoundBankFileState::OpenFile %" PRIu32 " (%s): Loading SoundBank as %s."), SoundBankId, *DebugName.ToString(), LoadAsMemoryView() ? TEXT("View") : TEXT("Copy"));
			Ptr = const_cast<uint8*>(InPtr);
			FileSize = InSize;
			OpenFileSucceeded(MoveTemp(InCallback));
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::OpenFile %" PRIu32 " (%s): Failed to load SoundBank (%s)."), SoundBankId, *DebugName.ToString(), *PackagedFile.PathName.ToString());
			Ptr = nullptr;
			FileSize = 0;
			OpenFileFailed(MoveTemp(InCallback));
		}
	},
	bContainsMedia,
	STAT_WwiseMemorySoundBank_FName, STAT_WwiseMemorySoundBankDevice_FName, WWISE_LLM_GET_NAME(Audio_Wwise_FileHandler_SoundBanks));
}

void FWwiseInMemorySoundBankFileState::LoadInSoundEngine(FLoadInSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine"));
	if (UNLIKELY(!FileSize || !Ptr))
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): No data, but supposed to be loaded."), SoundBankId, *DebugName.ToString());
		LoadInSoundEngineFailed(MoveTemp(InCallback));
		return;
	}

	auto* BankExecutionQueue = GetBankExecutionQueue();
	if (UNLIKELY(!BankExecutionQueue))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): Failed loading without the FileHandler module."), SoundBankId, *DebugName.ToString());
		LoadInSoundEngineFailed(MoveTemp(InCallback));
		return;
	}

	BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseInMemorySoundBankFileState::LoadInSoundEngine LoadBankMemory"), [this, InCallback = MoveTemp(InCallback)]() mutable
	{
		auto* SoundEngine = IWwiseSoundEngineAPI::Get();
		if (UNLIKELY(!SoundEngine))
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): Failed loading without a SoundEngine."), SoundBankId, *DebugName.ToString());
			LoadInSoundEngineFailed(MoveTemp(InCallback));
			return;
		}

		AkBankID LoadedSoundBankId;
		AkBankType LoadedSoundBankType;

		BankLoadCookie* Cookie = new BankLoadCookie(this);
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %p: Cookie %p."), this, Cookie);
		if (!Cookie)
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): Failed to load SoundBank: Could not allocate cookie."), SoundBankId, *DebugName.ToString());
			LoadInSoundEngineFailed(MoveTemp(InCallback));
			return;
		}

		Cookie->Callback = MoveTemp(InCallback);
		AKRESULT LoadResult;

#if AK_ENABLE_BANK_MGR_THREAD
		if (FPlatformProcess::SupportsMultithreading())
		{
			if (LoadAsMemoryView())
			{
				LoadResult = SoundEngine->LoadBankMemoryView(Ptr, FileSize, &FWwiseInMemorySoundBankFileState::BankLoadCallback, Cookie, LoadedSoundBankId, LoadedSoundBankType);
			}
			else
			{
				LoadResult = SoundEngine->LoadBankMemoryCopy(Ptr, FileSize, &FWwiseInMemorySoundBankFileState::BankLoadCallback, Cookie, LoadedSoundBankId, LoadedSoundBankType);
			}
			
			UE_CLOG(UNLIKELY(LoadedSoundBankType != static_cast<uint8>(SoundBankType)), LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): Incorrect SoundBank type: %" PRIu8 " expected %" PRIu8), SoundBankId, *DebugName.ToString(), (uint8)LoadedSoundBankType, (uint8)SoundBankType);
			
			if(LoadResult != AK_Success)
			{
				UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): Failed to load SoundBank: %d (%s)."), SoundBankId, *DebugName.ToString(), LoadResult, WwiseUnrealHelper::GetResultString(LoadResult));
				FreeMemoryIfNeeded();
				auto Callback = MoveTemp(Cookie->Callback);
				delete Cookie;
				LoadInSoundEngineFailed(MoveTemp(Callback));
				return;
			}
		}
		else
#endif
		{
			if (LoadAsMemoryView())
			{
				LoadResult = SoundEngine->LoadBankMemoryView(Ptr, FileSize, LoadedSoundBankId, LoadedSoundBankType);
			}
			else
			{
				LoadResult = SoundEngine->LoadBankMemoryCopy(Ptr, FileSize, LoadedSoundBankId, LoadedSoundBankType);
			}

			UE_CLOG(UNLIKELY(LoadedSoundBankType != static_cast<uint8>(SoundBankType)), LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::LoadInSoundEngine %" PRIu32 " (%s): Incorrect SoundBank type: %" PRIu8 " expected %" PRIu8), SoundBankId, *DebugName.ToString(), (uint8)LoadedSoundBankType, (uint8)SoundBankType);
			
			BankLoadCallback(LoadedSoundBankId, Ptr, LoadResult, Cookie);
		}
	});
}

void FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine(FUnloadFromSoundEngineCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine"));
	
	auto* BankExecutionQueue = GetBankExecutionQueue();
	if (UNLIKELY(!BankExecutionQueue))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine %" PRIu32 " (%s): Failed unloading without the FileHandler module."), SoundBankId, *DebugName.ToString());
		return UnloadFromSoundEngineToClosedFile(MoveTemp(InCallback));
	}

	BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine UnloadBank"), [this, InCallback = MoveTemp(InCallback)]() mutable
	{
		auto* SoundEngine = IWwiseSoundEngineAPI::Get();
		if (UNLIKELY(!SoundEngine))
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine %" PRIu32 " (%s): Failed unloading without a SoundEngine."), SoundBankId, *DebugName.ToString());
			return UnloadFromSoundEngineToClosedFile(MoveTemp(InCallback));
		}

		if (UNLIKELY(IsEngineExitRequested() && !SoundEngine->IsInitialized()))
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine: Cannot unload %" PRIu32 " (%s) SoundBank with an uninitialized SoundEngine."), SoundBankId, *DebugName.ToString());
			return UnloadFromSoundEngineToClosedFile(MoveTemp(InCallback));
		}

		BankUnloadCookie* Cookie = new BankUnloadCookie(this);
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine %p: Cookie %p."), this, Cookie);
		if(!Cookie)
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine %" PRIu32 " (%s): Could not allocate cookie for unload operation."), SoundBankId, *DebugName.ToString());
			return UnloadFromSoundEngineToClosedFile(MoveTemp(InCallback));
		}

		Cookie->Callback = MoveTemp(InCallback);
		AKRESULT Result;

#if AK_ENABLE_BANK_MGR_THREAD
		if (FPlatformProcess::SupportsMultithreading())
		{
			Result = SoundEngine->UnloadBank(SoundBankId, Ptr, &FWwiseInMemorySoundBankFileState::BankUnloadCallback, Cookie, static_cast<AkBankType>(SoundBankType));
		}
		else
#endif
		{
			Result = SoundEngine->UnloadBank(SoundBankId, Ptr, static_cast<AkBankType>(SoundBankType));
			BankUnloadCallback(SoundBankId, Ptr, Result, Cookie); 
		}
		if(Result != AK_Success)
		{
			UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseInMemorySoundBankFileState::UnloadFromSoundEngine %" PRIu32 " (%s): Call to SoundEngine failed with result %s"), SoundBankId, *DebugName.ToString(), WwiseUnrealHelper::GetResultString(Result));
			auto Callback = MoveTemp(Cookie->Callback);
			delete Cookie;
			UnloadFromSoundEngineToClosedFile(MoveTemp(Callback));
			return;
		}
	});
}

bool FWwiseInMemorySoundBankFileState::CanCloseFile() const
{
	// LoadFromSoundEngine will copy and delete the pointer. If succeeded, we are already closed.
	return (State == EState::Opened && Ptr == nullptr) || FWwiseSoundBankFileState::CanCloseFile();
}

void FWwiseInMemorySoundBankFileState::CloseFile(FCloseFileCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemorySoundBankFileState::CloseFile"));
	UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemorySoundBankFileState::CloseFile %" PRIu32 " (%s): Closing Memory Mapped SoundBank. Deallocating @ %p %" PRIi64 " bytes."), SoundBankId, *DebugName.ToString(), Ptr, FileSize);
	PackagedFile.DeallocateMemory(Ptr, FileSize, bContainsMedia,  STAT_WwiseMemorySoundBank_FName, STAT_WwiseMemorySoundBankDevice_FName);
	Ptr = nullptr;
	FileSize = 0;
	CloseFileDone(MoveTemp(InCallback));
}

void FWwiseInMemorySoundBankFileState::FreeMemoryIfNeeded()
{
	// We don't need the memory anymore if we copied it, whether the load succeeded or not.
	if (!LoadAsMemoryView())
	{
		UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemorySoundBankFileState::FreeMemoryIfNeeded %" PRIu32 " (%s): Freeing Pointer"), SoundBankId, *DebugName.ToString());
		PackagedFile.DeallocateMemory(Ptr, FileSize, bContainsMedia, STAT_WwiseMemorySoundBank_FName, STAT_WwiseMemorySoundBankDevice_FName);
		Ptr = nullptr;
		FileSize = 0;
	}
}

FWwiseInMemorySoundBankFileState::BankLoadCookie::BankLoadCookie(BankLoadCookie* InOther)
{
	if(InOther)
	{
		BankFileState = InOther->BankFileState;
		Callback = std::move(InOther->Callback);
	}
}

void FWwiseInMemorySoundBankFileState::BankLoadCallback(
	AkUInt32	InBankID,
	const void*	InMemoryBankPtr,
	AKRESULT	InLoadResult,
	void*		InCookie
	)
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback"));
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback %p: Cookie %p."), ((BankLoadCookie*)InCookie)->BankFileState, InCookie);
	if (!InCookie)
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback %" PRIu32 " (%s): Failed to load SoundBank: %d. Cookie given by SoundEngine is invalid."), InLoadResult, WwiseUnrealHelper::GetResultString(InLoadResult), InBankID);
		return;
	}

	LaunchWwiseTask(WWISEFILEHANDLER_ASYNC_NAME("FWwiseInMemorySoundBankFileState::BankLoadCallback Async"), [=]() {
		BankLoadCookie Cookie((BankLoadCookie*)InCookie);
		delete (BankLoadCookie*)InCookie;
		if (!Cookie.BankFileState)
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback %" PRIu32 " (%s): Failed to load SoundBank: %d. Cookie given by SoundEngine is invalid."), InLoadResult, WwiseUnrealHelper::GetResultString(InLoadResult), InBankID);
			return;
		}

		auto* BankFileState = Cookie.BankFileState;
		if (LIKELY(InLoadResult == AK_Success))
		{
			UE_CLOG(UNLIKELY(InBankID != BankFileState->SoundBankId), LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback: Incorrect SoundBank loaded: %" PRIu32 " expected %" PRIu32 " (%s)"), InBankID, BankFileState->SoundBankId, *BankFileState->DebugName.ToString());
			UE_CLOG(LIKELY(InBankID == BankFileState->SoundBankId), LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback: Loaded: %" PRIu32 " (%s) @ %p %" PRIi64 " bytes."), InBankID, *BankFileState->DebugName.ToString(), BankFileState->Ptr, BankFileState->FileSize);

			INC_DWORD_STAT(STAT_WwiseFileHandlerLoadedSoundBanks);
			BankFileState->FreeMemoryIfNeeded();
			BankFileState->LoadInSoundEngineSucceeded(MoveTemp(Cookie.Callback));
		}
		else
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankLoadCallback %" PRIu32 " (%s): Failed to load SoundBank: %d (%s)."), InBankID, *BankFileState->DebugName.ToString(), InLoadResult, WwiseUnrealHelper::GetResultString(InLoadResult));
			BankFileState->FreeMemoryIfNeeded();
			BankFileState->LoadInSoundEngineFailed([]{});
			Cookie.BankFileState->FileStateExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseInMemorySoundBankFileState::BankLoadCallback Cookie"), [Cookie = MoveTemp(Cookie)]() mutable
			{
				Cookie.Callback();
			});
		}
	});
}

void FWwiseInMemorySoundBankFileState::BankUnloadCallback(
	AkUInt32	InBankID,
	const void* InMemoryBankPtr,
	AKRESULT	InUnloadResult,
	void*		InCookie
)
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback"));
	UE_LOG(LogWwiseFileHandler, VeryVerbose, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback %p: Cookie %p."), ((BankUnloadCookie*)InCookie)->BankFileState, InCookie);
	if (!InCookie)
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback %" PRIu32 ": Cookie given by SoundEngine is invalid."), InBankID);
		return;
	}

	LaunchWwiseTask(WWISEFILEHANDLER_ASYNC_NAME("FWwiseInMemorySoundBankFileState::BankLoadCallback Async"), [=]() {
		SCOPED_WWISEFILEHANDLER_EVENT_3(TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback Async"));
		BankUnloadCookie Cookie((BankUnloadCookie*)InCookie);
		delete (BankUnloadCookie*)InCookie;

		if (!Cookie.BankFileState)
		{
			UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback %" PRIu32 ": Cookie given by SoundEngine is invalid."), InBankID);
			return;
		}

		auto* BankFileState = Cookie.BankFileState;
		if (UNLIKELY(InUnloadResult == AK_ResourceInUse))
		{
			UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback %" PRIu32 " (%s): Deferred (AK_ResourceInUse)."), InBankID, *BankFileState->DebugName.ToString());
			BankFileState->UnloadFromSoundEngineDefer(MoveTemp(Cookie.Callback));
		}
		else
		{
			UE_CLOG(InUnloadResult != AK_Success, LogWwiseFileHandler, Error, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback %" PRIu32 " (%s): UnloadBank failed: %d (%s)"), InBankID, *BankFileState->DebugName.ToString(), InUnloadResult, WwiseUnrealHelper::GetResultString(InUnloadResult));
			UE_CLOG(LIKELY(InUnloadResult == AK_Success) && BankFileState->Ptr == nullptr, LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback: Unloaded: %" PRIu32 " (%s) as MemoryCopy."), InBankID, *BankFileState->DebugName.ToString());
			UE_CLOG(LIKELY(InUnloadResult == AK_Success) && BankFileState->Ptr != nullptr, LogWwiseFileHandler, Verbose, TEXT("FWwiseInMemorySoundBankFileState::BankUnloadCallback: Unloaded: %" PRIu32 " (%s) as MemoryView. Deallocating @ %p %" PRIi64 " bytes."), InBankID, *BankFileState->DebugName.ToString(), BankFileState->Ptr, BankFileState->FileSize);
			DEC_DWORD_STAT(STAT_WwiseFileHandlerLoadedSoundBanks);
			if (InMemoryBankPtr)
			{
				BankFileState->UnloadFromSoundEngineDone(MoveTemp(Cookie.Callback));
			}
			else
			{
				BankFileState->UnloadFromSoundEngineToClosedFile(MoveTemp(Cookie.Callback));
			}
		}
	});
}

