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

#include "Wwise/WwiseMediaManagerImpl.h"
#include "Wwise/WwiseMediaFileState.h"
#include "Wwise/WwiseSoundEngineUtils.h"
#include "Wwise/API/WwiseSoundEngineAPI.h"
#include "Wwise/Stats/FileHandler.h"
#include "Wwise/Stats/FileHandlerMemory.h"

#include "Misc/ScopeExit.h"

#include <inttypes.h>

FWwiseMediaManagerImpl::FWwiseMediaManagerImpl()
{
}

FWwiseMediaManagerImpl::~FWwiseMediaManagerImpl()
{
}

void FWwiseMediaManagerImpl::LoadMedia(const FWwiseMediaCookedData& InMediaCookedData, FLoadMediaCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseMediaManagerImpl::LoadMedia"));
	IncrementFileStateUseAsync(InMediaCookedData.MediaId, EWwiseFileStateOperationOrigin::Loading, [WeakThis=AsWeak(), InMediaCookedData]() mutable
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_FileHandler_Media);
		auto SharedMediaManager = StaticCastSharedPtr<FWwiseMediaManagerImpl>(WeakThis.Pin());
		if (!SharedMediaManager.IsValid())
		{
			UE_LOG(LogWwiseFileHandler, Error,
			       TEXT("FWwiseMediaManagerImpl::LoadMedia CreateOp callback: Failed to get MediaManager"))
			return FWwiseFileStateSharedPtr(nullptr);
		}
		return SharedMediaManager->CreateOp(InMediaCookedData);
	}, [InCallback = MoveTemp(InCallback)](const FWwiseFileStateSharedPtr, bool bInResult)
	{
		InCallback(bInResult);
	});
}

void FWwiseMediaManagerImpl::UnloadMedia(const FWwiseMediaCookedData& InMediaCookedData, FUnloadMediaCallback&& InCallback)
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseMediaManagerImpl::UnloadMedia"));
	DecrementFileStateUseAsync(InMediaCookedData.MediaId, nullptr, EWwiseFileStateOperationOrigin::Loading, MoveTemp(InCallback));
}

void FWwiseMediaManagerImpl::SetGranularity(uint32 InStreamingGranularity)
{
	SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseMediaManagerImpl::SetGranularity"));
	StreamingGranularity = InStreamingGranularity;
}

void FWwiseMediaManagerImpl::SetMedia(AkSourceSettings& InSource, FLoadMediaCallback&& InCallback)
{
	auto* BankExecutionQueue = GetBankExecutionQueue();
	if (UNLIKELY(!BankExecutionQueue))
	{
		return InCallback(false);
	}

	FScopeLock Lock(&MediaOpCriticalSection);
	++SetMediaCount;
	BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseMediaManagerImpl::SetMedia"), [this, &InSource, InCallback = MoveTemp(InCallback)]() mutable 
	{
		if (SetMediaOps.Num() == 0)
		{
			auto* BankExecutionQueue = GetBankExecutionQueue();
			if (UNLIKELY(!BankExecutionQueue))
			{
				InCallback(false);
				return;
			}

			FScopeLock Lock(&MediaOpCriticalSection);
			const auto FinalMediaCount = SetMediaCount;
			SetMediaCount = 0;

			SetMediaOps.Empty(FinalMediaCount);
			SetMediaCallbacks.Empty(FinalMediaCount);

			SetMediaOps.Emplace(InSource);
			SetMediaCallbacks.Emplace(MoveTemp(InCallback));
			
			BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseMediaManagerImpl::DoSetMedia"), [this]() mutable
			{
				DoSetMedia();
			});
		}
		else
		{
			UE_CLOG(UNLIKELY(SetMediaOps.GetSlack() == 0), LogWwiseFileHandler, Warning,
				TEXT("FWwiseMediaManagerImpl::SetMedia: Emplacing supplemental operations without slack!"));
			SetMediaOps.Emplace(InSource);
			SetMediaCallbacks.Emplace(MoveTemp(InCallback));
		}
	});
}

void FWwiseMediaManagerImpl::UnsetMedia(AkSourceSettings& InSource, FLoadMediaCallback&& InCallback)
{
	auto* BankExecutionQueue = GetBankExecutionQueue();
	if (UNLIKELY(!BankExecutionQueue))
	{
		return InCallback(true);
	}

	FScopeLock Lock(&MediaOpCriticalSection);
	++UnsetMediaCount;
	BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseMediaManagerImpl::UnsetMedia"), [this, &InSource, InCallback = MoveTemp(InCallback)]() mutable 
	{
		if (UnsetMediaOps.Num() == 0)
		{
			auto* BankExecutionQueue = GetBankExecutionQueue();
			if (UNLIKELY(!BankExecutionQueue))
			{
				InCallback(true);
				return;
			}

			FScopeLock Lock(&MediaOpCriticalSection);
			const auto FinalMediaCount = UnsetMediaCount;
			UnsetMediaCount = 0;

			UnsetMediaOps.Empty(FinalMediaCount);
			UnsetMediaCallbacks.Empty(FinalMediaCount);

			UnsetMediaOps.Emplace(InSource);
			UnsetMediaCallbacks.Emplace(MoveTemp(InCallback));
			
			BankExecutionQueue->Async(WWISEFILEHANDLER_ASYNC_NAME("FWwiseMediaManagerImpl::DoUnsetMedia"), [this]() mutable
			{
				DoUnsetMedia();
			});
		}
		else
		{
			UE_CLOG(UNLIKELY(UnsetMediaOps.GetSlack() == 0), LogWwiseFileHandler, Warning,
				TEXT("FWwiseMediaManagerImpl::UnsetMedia: Emplacing supplemental operations without slack!"));
			UnsetMediaOps.Emplace(InSource);
			UnsetMediaCallbacks.Emplace(MoveTemp(InCallback));
		}
	});
}

FWwiseFileStateSharedPtr FWwiseMediaManagerImpl::CreateOp(const FWwiseMediaCookedData& InMediaCookedData)
{
	if (InMediaCookedData.PackagedFile.bStreaming)
	{
		return FWwiseFileStateSharedPtr(new FWwiseStreamedMediaFileState(InMediaCookedData, StreamingGranularity));
	}
	else
	{
		return FWwiseFileStateSharedPtr(new FWwiseInMemoryMediaFileState(InMediaCookedData));
	}
}

FWwiseExecutionQueue* FWwiseMediaManagerImpl::GetBankExecutionQueue()
{
	auto* FileHandlerModule = IWwiseFileHandlerModule::GetModule();
	if (UNLIKELY(!FileHandlerModule))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseMediaManagerImpl::GetBankExecutionQueue: WwiseFileHandlerModule is unloaded."));
		return nullptr;
	}
	return FileHandlerModule->GetBankExecutionQueue();
}

void FWwiseMediaManagerImpl::DoSetMedia()
{
	bool bResult = false;
	ON_SCOPE_EXIT
	{
		for (auto& Callback : SetMediaCallbacks)
		{
			Callback(bResult);
		}
		SetMediaOps.Empty();
		SetMediaCallbacks.Empty();
	};
	
	auto* SoundEngine = IWwiseSoundEngineAPI::Get();
	if (UNLIKELY(!SoundEngine))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseMediaManagerImpl::DoSetMedia: Failed loading %" PRIu32 " media without a SoundEngine."), SetMediaOps.Num());
		return;
	}

	const auto SetMediaResult = SoundEngine->SetMedia(SetMediaOps.GetData(), SetMediaOps.Num());
	if (LIKELY(SetMediaResult == AK_Success))
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseMediaManagerImpl::DoSetMedia: Loaded %" PRIu32 " media."), SetMediaOps.Num());
		bResult = true;
	}
	else
	{
		UE_LOG(LogWwiseFileHandler, Error, TEXT("FWwiseMediaManagerImpl::DoSetMedia: Failed to load %" PRIu32 " media: %d (%s)."), SetMediaOps.Num(), SetMediaResult, WwiseUnrealHelper::GetResultString(SetMediaResult));
	}
}

void FWwiseMediaManagerImpl::DoUnsetMedia()
{
	TArray<AKRESULT> Results;
	Results.AddUninitialized(UnsetMediaOps.Num());
	for (auto& Result : Results)
	{
		Result = AK_Success;
	}

	ON_SCOPE_EXIT
	{
		for (int Iter = 0; Iter < UnsetMediaCallbacks.Num(); ++Iter)
		{
			auto& Callback { UnsetMediaCallbacks[Iter] };
			const auto& Result { Results[Iter] };

			Callback(Result != AK_ResourceInUse);
		}
		UnsetMediaOps.Empty();
		UnsetMediaCallbacks.Empty();
	};
	
	auto* SoundEngine = IWwiseSoundEngineAPI::Get();
	if (UNLIKELY(!SoundEngine))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseMediaManagerImpl::DoUnsetMedia: Failed unloading %" PRIu32 " media without a SoundEngine."), UnsetMediaOps.Num());
		return;
	}

	if (UNLIKELY(IsEngineExitRequested() && !SoundEngine->IsInitialized()))
	{
		UE_LOG(LogWwiseFileHandler, Log, TEXT("FWwiseMediaManagerImpl::DoUnsetMedia: Cannot unload %" PRIu32 " media with an uninitialized SoundEngine."), UnsetMediaOps.Num());
		return;
	}

	const auto UnsetMediaResult = SoundEngine->TryUnsetMedia(UnsetMediaOps.GetData(), UnsetMediaOps.Num(), Results.GetData());
	if (LIKELY(UnsetMediaResult == AK_Success))
	{
		UE_LOG(LogWwiseFileHandler, Verbose, TEXT("FWwiseMediaManagerImpl::DoUnsetMedia: Unloaded %" PRIu32 " media."), UnsetMediaOps.Num());
	}
	else
	{
		UE_CLOG(LIKELY(UnsetMediaResult == AK_ResourceInUse), LogWwiseFileHandler, Verbose,
			TEXT("FWwiseMediaManagerImpl::DoUnsetMedia: Failed to unload %" PRIu32 " media: %d (%s)."),
			UnsetMediaOps.Num(), UnsetMediaResult, WwiseUnrealHelper::GetResultString(UnsetMediaResult));
		UE_CLOG(UNLIKELY(UnsetMediaResult != AK_ResourceInUse), LogWwiseFileHandler, Log,
			TEXT("FWwiseMediaManagerImpl::DoUnsetMedia: Failed to unload %" PRIu32 " media: %d (%s)."),
			UnsetMediaOps.Num(), UnsetMediaResult, WwiseUnrealHelper::GetResultString(UnsetMediaResult));
	}
}
