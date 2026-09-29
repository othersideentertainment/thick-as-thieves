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

#include "WwiseMockFileState.h"
#include "Wwise/WwiseMediaManager.h"
#include "Wwise/WwiseFileHandlerBase.h"
#include "Wwise/CookedData/WwiseMediaCookedData.h"
#include "Wwise/Stats/FileHandler.h"

class FWwiseMockMediaManager : public IWwiseMediaManager, public FWwiseFileHandlerBase
{
public:
	FWwiseMockMediaManager() {}
	~FWwiseMockMediaManager() override {}

	const TCHAR* GetManagingTypeName() const override { return TEXT("MockMedia"); }

	void LoadMedia(const FWwiseMediaCookedData& InMediaCookedData, FLoadMediaCallback&& InCallback) override
	{
		SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseMockMediaManager::LoadMedia"));
		IncrementFileStateUseAsync(InMediaCookedData.MediaId, EWwiseFileStateOperationOrigin::Loading, [WeakThis=AsWeak(), InMediaCookedData]() mutable
		{
			auto SharedMockMediaManager = StaticCastSharedPtr<FWwiseMockMediaManager>(WeakThis.Pin());
			if (!SharedMockMediaManager.IsValid())
			{
				UE_LOG(LogWwiseFileHandler, Error,
				       TEXT("FWwiseMockSoundBankManager::LoadSoundBank: Failed to get Mock MediaManager"))
				return FWwiseFileStateSharedPtr(nullptr);
			}
			return SharedMockMediaManager->CreateOp(InMediaCookedData);
		}, [InCallback = MoveTemp(InCallback)](const FWwiseFileStateSharedPtr, bool bInResult)
		{
			InCallback(bInResult);
		});
	}
	void UnloadMedia(const FWwiseMediaCookedData& InMediaCookedData, FUnloadMediaCallback&& InCallback) override
	{
		SCOPED_WWISEFILEHANDLER_EVENT_4(TEXT("FWwiseMockMediaManager::UnloadMedia"));
		DecrementFileStateUseAsync(InMediaCookedData.MediaId, nullptr, EWwiseFileStateOperationOrigin::Loading, MoveTemp(InCallback));
	}
	void SetGranularity(uint32 InStreamingGranularity) override {}

	IWwiseStreamingManagerHooks& GetStreamingHooks() override final { return *this; }

	bool IsMediaLoaded(int32 mediaId)
	{
		for (const auto& pair : FileStatesById) {
			const FWwiseFileStateSharedPtr& fileState = pair.Value;
			if (fileState->GetShortId() == mediaId) {
				return true;
			}
		}
		return false;
	}

	bool IsEmpty()
	{
		FRWScopeLock Lock(FileStatesByIdLock, FRWScopeLockType::SLT_ReadOnly);
		return FileStatesById.Num() == 0;
	}

protected:
	virtual FWwiseFileStateSharedPtr CreateOp(const FWwiseMediaCookedData& InMediaCookedData)
	{
		auto* FileState = new FWwiseMockFileState(InMediaCookedData.MediaId);
		if (InMediaCookedData.PackagedFile.bStreaming)
		{
			FileState->bIsStreamedState = FWwiseMockFileState::OptionalBool::True;
		}
		else
		{
			FileState->bIsStreamedState = FWwiseMockFileState::OptionalBool::False;
		}
		return FWwiseFileStateSharedPtr(FileState);
	}

public:
	virtual void SetMedia(AkSourceSettings& InSource, FLoadMediaCallback&& InCallback) override { InCallback(true); }
	virtual void UnsetMedia(AkSourceSettings& InSource, FLoadMediaCallback&& InCallback) override { InCallback(true); }
};
