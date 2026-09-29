/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

  Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkTransferDeferred.cpp
//
// Transfer object used by the deferred lined-up device.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkTransferDeferred.h"
#include "AkDeviceDeferredLinedUp.h"

using namespace AK::StreamMgr;

// CAkStmMemViewDeferred
// -----------------------------------------------

// Update a memory view when transfer is complete (updates its owner task).
void CAkStmMemViewDeferred::Update( AKRESULT in_eResult, bool in_bRequiredLowLevelXfer ) 
{ 
	m_pOwner->Update( this, in_eResult, in_bRequiredLowLevelXfer ); 
}

void CAkStmMemViewDeferred::Cancel()
{
	CAkDeviceDeferredLinedUp * pDevice = (CAkDeviceDeferredLinedUp*)m_pOwner->GetDevice();

	// Lock device in order to safely access low-level transfer, ask it if it is possible to cancel it,
	// and perform clean up if it is.
	{
		AkAutoLock<CAkIOThread> deviceLock( *pDevice );

		// Get low-level transfer.
		// Note: pLowLevelTransfer may be NULL if this is a transfer of a cached block that is still in the pending
		// list. Cached transfers are in the pending list between calls to PrepareTransfer() and Update() by the 
		// IO thread. This is protected by the task's status lock.
		CAkLowLevelTransfer * pLowLevelTransfer = (CAkLowLevelTransfer*)GetLowLevelTransfer();
		if ( pLowLevelTransfer )
		{
			if ( pLowLevelTransfer->CanCancel() )
			{
				// Need to proceed with cancellation. Untag mem block while still locked.
				pDevice->OnLowLevelTransferCancelled( m_pBlock );
			}
		}
	}
}



// CAkLowLevelTransfer
// -----------------------------------------------

// Callback from Low-Level I/O.
void CAkLowLevelTransfer::LLIOCallback( 
	AkAsyncIOTransferInfo * in_pTransferInfo,	// Pointer to the AkAsyncIOTransferInfo structure that was passed to corresponding Read() or Write() call.
	AKRESULT				in_eResult			// Result of transfer: AK_Success or AK_Fail (stream becomes invalid).
	)
{
	AkAsyncIOTransferInfo* pTransferInfo = in_pTransferInfo;
	AKRESULT eResult = in_eResult;

	AKASSERT(pTransferInfo || !"Invalid transfer info");

	// Low-level transfer object was stored in pCookie.
	CAkLowLevelTransfer* pLowLevelTransfer = (CAkLowLevelTransfer*)pTransferInfo->pCookie;

	AKASSERT(pLowLevelTransfer || !"Invalid transfer object: corrupted cookie");
	AKASSERT(pLowLevelTransfer->IsValid() || !"Calling the callback on an idle transfer!");

	pLowLevelTransfer->Update(eResult);
}

// Called from LL IO completion static callback.
// Pops all observers, destroys itself, then notifies all cached observers that transfer is complete.
// Sync: None. Locks device to safely pop observers, then
void CAkLowLevelTransfer::Update(AKRESULT in_eResult) // Result of transfer: AK_Success, AK_Fail (stream becomes invalid), or any other errorcode.
{
	// Clean error code from Low-Level IO.
	if (in_eResult != AK_Success)
	{
#ifndef AK_OPTIMIZED
		AkStreamRecord streamRecord;
		m_pOwner->GetStreamRecord(streamRecord);
		char szName[AK_MONITOR_STREAMNAME_MAXLENGTH+1];
		AK_UTF16_TO_CHAR(szName, streamRecord.szStreamName, AK_MONITOR_STREAMNAME_MAXLENGTH);
		
		using namespace AK::Monitor;
		if (in_eResult == AK_DeviceNotReady)
		{
			if (streamRecord.idFile != AK_INVALID_FILE_ID)
				PostCodeVarArg(ErrorCode_WwiseIODisconnected, ErrorLevel_Error, MsgContext(), streamRecord.idFile);
			else
				PostCodeVarArg(ErrorCode_WwiseIODisconnectedStr, ErrorLevel_Error, MsgContext(), szName);

			in_eResult = AK_Fail;
		}
		else
		{
			if (streamRecord.idFile != AK_INVALID_FILE_ID)
			{
				PostCodeVarArg(ErrorCode_IODevice, ErrorLevel_Error, MsgContext(),
					m_pOwner->CurrentOp() == CAkStmTask::OpWrite ? "Write" : "Read",
					in_eResult,
					streamRecord.idFile);
			}
			else
			{
				PostCodeVarArg(ErrorCode_IODeviceStr, ErrorLevel_Error, MsgContext(),
					m_pOwner->CurrentOp() == CAkStmTask::OpWrite ? "Write" : "Read",
					in_eResult,
					szName);
			}
		}
#endif
	}

	// Lock device, remove all observers and delete this transfer. After this, new data requests trying to 
	// attach to this memory block will be treated as if it was idle.
	CAkDeviceDeferredLinedUp * pDevice = (CAkDeviceDeferredLinedUp*)m_pOwner->GetDevice();
	AkStmMemObserversList observers;
	{
		AkAutoLock<CAkIOThread> deviceLock( *pDevice );

		// Cache observers list and clear it.
		observers = m_observers;
		AKASSERT( observers.First() );	// Length() >= 1.
		m_observers.RemoveAll();

		if (m_pOwner->CurrentOp() == CAkStmTask::OpWrite)
		{
			//Check if the filesize changed.
			if (info.uFilePosition + info.uRequestedSize > (AkUInt64)m_pOwner->GetFileDesc()->iFileSize)
				m_pOwner->GetFileDesc()->iFileSize = info.uFilePosition + info.uRequestedSize;
		}

		// OnTransferComplete clears block's low-level transfer, and notifies the memory manager if there was 
		// an error (the block would contain invalid data).
		pDevice->OnLowLevelTransferComplete( observers.First(), in_eResult, m_pOwner->CurrentOp() == CAkStmTask::OpRead );
	}

	// At this point, "this" has become invalid. Notify all observers cached on stack.
	// Note: in order to report correct profiling info, pass bRequiredLowLevelXfer = true only 
	// to the first observer.

	// Remove and notify all observers of this transfer.
	bool bRequiredLowLevelXfer = true;
	AkStmMemObserversList::IteratorEx it = observers.BeginEx();
	while ( it != observers.End() )
	{
		CAkStmMemViewDeferred * pMemView = (*it);
		it = observers.Erase( it );
		pMemView->Update( in_eResult, bRequiredLowLevelXfer );
		bRequiredLowLevelXfer = false;
	}
	observers.Term();
}

// Ask the low-level transfer if it can be cancelled. It can if and only if 
// there is only one observer to this transfer, and it is the caller.
// Sync: Owner task must be locked, device must be locked.
bool CAkLowLevelTransfer::CanCancel()
{
	AKASSERT( m_observers.First() || !"No observers for this low-level transfer" );

	// Can cancel a low-level transfer only if there is just one observer, and this observer's owner
	// has to be the same as the low-level transfer's owner, otherwise there would be a locking issue:
	// low-level cancellation is synchronized with task locks.
	AkStmMemObserversList::Iterator it = m_observers.Begin();
	++it;
	if ( it != m_observers.End() )
	{
		// There is more than one observer. Cannot cancel.
		return false;
	}
	else
	{
		// There is just one observer. Check its owner.
		return ( m_observers.First()->GetOwner() == m_pOwner );
	}
}
