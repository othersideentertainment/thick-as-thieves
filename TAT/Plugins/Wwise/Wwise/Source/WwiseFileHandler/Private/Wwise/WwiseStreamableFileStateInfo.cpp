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

#include "Wwise/WwiseStreamableFileStateInfo.h"

bool FWwiseStreamableFileStateInfo::CanProcessFileOp() const
{
	UE_LOG(LogWwiseFileHandler, Error, TEXT("Cannot process read on a non-streaming asset"));
	return false;
}

AKRESULT FWwiseStreamableFileStateInfo::ProcessRead(AkFileDesc& InFileDesc, const AkIoHeuristics& InHeuristics, AkAsyncIOTransferInfo& OutTransferInfo, FWwiseAkFileOperationDone&& InFileOpDoneCallback)
{
	UE_LOG(LogWwiseFileHandler, Error, TEXT("Cannot process read on a non-streaming asset"));
	return AK_Fail;
}

AKRESULT FWwiseStreamableFileStateInfo::ProcessWrite(AkFileDesc& InFileDesc, const AkIoHeuristics& InHeuristics, AkAsyncIOTransferInfo& OutTransferInfo, FWwiseAkFileOperationDone&& InFileOpDoneCallback)
{
	UE_LOG(LogWwiseFileHandler, Error, TEXT("Cannot process write on a non-writable asset"));
	return AK_Fail;
}