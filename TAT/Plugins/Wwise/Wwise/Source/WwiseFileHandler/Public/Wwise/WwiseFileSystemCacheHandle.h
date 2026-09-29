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

#include "WwiseFileCache.h"

/**
 * File streamer for the contents of a specified system path. It can be used with external files or packaged files.
 */
class WWISEFILEHANDLER_API FWwiseFileSystemCacheHandle : public IWwiseFileCacheHandle
{
public:
	FWwiseFileSystemCacheHandle(const FString& Pathname);
	virtual ~FWwiseFileSystemCacheHandle() override;
	virtual void CloseAndDelete() override;

	virtual void Open(FWwiseFileOperationDone&& OnDone) override;
		
	virtual void ReadData(uint8* OutBuffer, int64 Offset, int64 BytesToRead, EAsyncIOPriorityAndFlags Priority, FWwiseFileOperationDone&& OnDone) override;

	virtual int64 GetFileSize() const override { return FileSize; }

protected:
	FString Pathname;

	IAsyncReadFileHandle* FileHandle;
	int64 FileSize;

	FWwiseFileOperationDone InitializationDone;
	FWwiseAsyncCycleCounter* InitializationStat;

	static constexpr const int32 DeleteOnLastRequest{ 0x40000000 };
	std::atomic<int32> RequestsInFlight { 0 };

	void DeleteRequest(IAsyncReadRequest* Request);
	virtual void OnDeleteRequest(IAsyncReadRequest* Request);
	virtual void RemoveRequestInFlight();
	virtual void OnCloseAndDelete();
	virtual void OnSizeRequestDone(bool bWasCancelled, IAsyncReadRequest* Request);
	virtual void OnReadDataDone(bool bWasCancelled, IAsyncReadRequest* Request, FWwiseFileOperationDone&& OnDone);
	virtual void OnReadDataDone(bool bResult, FWwiseFileOperationDone&& OnDone);
	virtual void CallDone(bool bResult, FWwiseFileOperationDone&& OnDone);
};
