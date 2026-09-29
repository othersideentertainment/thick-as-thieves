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
// AkDeviceBase.cpp
//
// Device implementation that is common across all IO devices.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkDeviceBase.h"
#include "AkStreamingDefaults.h"

#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#ifdef AK_SUPPORT_WCHAR
#include <wchar.h>
#endif //AK_SUPPORT_WCHAR
#include <stdio.h>

using namespace AK;
using namespace AK::StreamMgr;

#ifdef _DEBUG
//For the starvation test tool.
#include <AK/Tools/Common/AkRng.h>
extern bool g_bSimulateStreamStarvation;
static CAkRng g_DebugRNG(1234567890);		//Yes, the rng is not random, we just need a sequence.  
#endif

//-------------------------------------------------------------------
// Defines.
//-------------------------------------------------------------------

#define AK_MINIMAL_THROUGHPUT	(1.f)    // 1 byte/second.
#define AK_CACHING_STREAM_MIN_BUFFER_SIZE (2048) //See AK_WORST_CASE_MIN_STREAM_BUFFER_SIZE in AkSrcFileBase.h

CAkStmTask::AkDeferredOpenData::AkDeferredOpenData() : AkAsyncFileOpenData()
{
}

CAkStmTask::AkDeferredOpenData::~AkDeferredOpenData()
{
	if (pszFileName)
	{
		AkFree(AkMemID_Streaming, (void*)pszFileName);
	}
}

AKRESULT CAkStmTask::AkDeferredOpenData::Init(const AkFileOpenData& in_FileOpenParams)
{
	eOpenMode = in_FileOpenParams.eOpenMode;
	uDeviceID = AK_INVALID_DEVICE_ID;

	pFlags = &flagsStorage;
	if (in_FileOpenParams.pFlags)
		flagsStorage = *in_FileOpenParams.pFlags;

	fileID = in_FileOpenParams.fileID;
	pszFileName = NULL;
	bOriginalLanguageFlag = flagsStorage.bIsLanguageSpecific;

	if (in_FileOpenParams.pszFileName)
	{
		// Important: Need to set this first for proper freeing in case of error.
		bByString = true;

		// Allocate string buffer for user defined stream name.
		size_t uStrLen = AKPLATFORM::OsStrLen(in_FileOpenParams.pszFileName) + 1;
		pszFileName = (AkOSChar*)AkAlloc(AkMemID_Streaming, (AkUInt32)sizeof(AkOSChar) * (uStrLen));
		if (!pszFileName)
			return AK_InsufficientMemory;

		// Copy.
		AKPLATFORM::SafeStrCpy(const_cast<AkOSChar*>(pszFileName), in_FileOpenParams.pszFileName, uStrLen);
	}

	return AK_Success;
}

//-------------------------------------------------------------------
// CAkDeviceBase implementation.
//-------------------------------------------------------------------

// Device construction/destruction.
CAkDeviceBase::CAkDeviceBase( 
	IAkLowLevelIOHook *	in_pLowLevelHook
	)
: m_pLowLevelHook( in_pLowLevelHook )
, m_uMaxCachePinnedBytes(AkUInt32(-1))
, m_uCurrentCachePinnedData(0)
#ifndef AK_OPTIMIZED
, m_uNumActiveStreams( 0 )
, m_uBytesLowLevelThisInterval( 0 )
, m_uBytesThisInterval( 0 )
, m_uNumLowLevelRequests( 0 )
, m_uNumLowLevelRequestsCancelled( 0 )
, m_uBytesThisSession(0)
, m_uCacheBytesThisSession(0)
, m_bIsMonitoring( false )
, m_bIsNew( true )
, m_bTasksListLockAcquired(false)
#endif
{
}

CAkDeviceBase::~CAkDeviceBase( )
{
}

// Init.
AKRESULT CAkDeviceBase::Init( 
    const AkDeviceSettings &    in_settings,
    AkDeviceID                  in_deviceID,
    bool                        in_bUseIOThread
    )
{
    if ( 0 == in_settings.uGranularity )
    {
#ifndef AK_OPTIMIZED
        AKASSERT( !"Invalid I/O granularity: must be non-zero" );
		AK::Monitor::PostCodeVarArg(AK::Monitor::ErrorCode_InvalidParameter, AK::Monitor::ErrorLevel_Error, AK::Monitor::MsgContext(), "AkDeviceSettings::uGranularity", "AK::StreamMgr::CreateDevice");
#endif
        return AK_InvalidParameter;
    }
    if ( in_settings.uIOMemorySize && 
         in_settings.fTargetAutoStmBufferLength < 0 )
    {
#ifndef AK_OPTIMIZED
        AKASSERT( !"Invalid automatic stream average buffering value" );
		AK::Monitor::PostCodeVarArg(AK::Monitor::ErrorCode_InvalidParameter, AK::Monitor::ErrorLevel_Error, AK::Monitor::MsgContext(), "AkDeviceSettings::fTargetAutoStmBufferLength", "AK::StreamMgr::CreateDevice");
#endif
        return AK_InvalidParameter;
    }
	if ( in_settings.uMaxConcurrentIO < 1 || in_settings.uMaxConcurrentIO > 1024 )
    {
#ifndef AK_OPTIMIZED
        AKASSERT( !"Invalid maximum number of concurrent I/O" );
		AK::Monitor::PostCodeVarArg(AK::Monitor::ErrorCode_InvalidParameter, AK::Monitor::ErrorLevel_Error, AK::Monitor::MsgContext(), "AkDeviceSettings::uMaxConcurrentIO", "AK::StreamMgr::CreateDevice");
#endif
        return AK_InvalidParameter;
    }

    m_uGranularity			= in_settings.uGranularity;
    m_fTargetAutoStmBufferLength  = in_settings.fTargetAutoStmBufferLength;
	m_uMaxConcurrentIO		= in_settings.uMaxConcurrentIO;
	
	m_deviceID				= in_deviceID;

	m_uMaxCachePinnedBytes = in_settings.uMaxCachePinnedBytes;

	AKRESULT res = m_mgrMemIO.Init(in_settings, this);
	if ( res != AK_Success )
	{
#ifndef AK_OPTIMIZED
		AKASSERT( !"Failed creating IO memory manager." );
		AK::Monitor::PostCodeVarArg(AK::Monitor::ErrorCode_IODeviceInitFailed, AK::Monitor::ErrorLevel_Error, AK::Monitor::MsgContext());
#endif
		return res;
	}

	AK_MONITOR_STREAMING_DEVICE_INIT(in_deviceID, in_settings);
	
	// Create I/O scheduler thread objects.
	return CAkIOThread::Init(in_settings.threadProperties, in_bUseIOThread);
}

// Destroy.
void CAkDeviceBase::Destroy()
{
	AK_MONITOR_STREAMING_DEVICE_DESTROYED(m_deviceID);
	
	if (!AK::StreamMgr::UsingIOThread())
	{
		while (!ClearStreams())
			PerformIO();
	}

	CAkIOThread::Term();

#ifndef AK_OPTIMIZED
    m_arStreamProfiles.Term();
#endif
	m_mgrMemIO.Term();

    AkDelete( AkMemID_Streaming, this );
}

// Device ID. Virtual method defined in IAkDevice.
AkDeviceID CAkDeviceBase::GetDeviceID()
{
    return m_deviceID;
}

// Destroys all streams remaining to be destroyed.
bool CAkDeviceBase::ClearStreams()
{
	AkAutoLock<CAkLock> gate(m_lockTasksList);
	bool emptyTasks			= ClearTaskList( m_listTasks );
	bool emptyCachingTasks	= ClearTaskList( m_listCachingTasks );
	return emptyTasks && emptyCachingTasks;
}

bool CAkDeviceBase::ClearTaskList(TaskList& in_listTasks)
{
	TaskList::IteratorEx it = in_listTasks.BeginEx();
	while ( it != in_listTasks.End() )
	{
		CAkStmTask * pTask = (*it);
		if ( pTask->IsToBeDestroyed() )
		{
			if ( pTask->CanBeDestroyed() )
			{
				it = in_listTasks.Erase( it );
				pTask->InstantDestroy();
			}
			else
			{
				// Cannot be destroyed now (has I/O pending). Try again later.
				++it;
			}
		}
		else
		{
			// Set status as "error" and wait until client destroys us.
			pTask->Kill();
			++it;
		}
	}

	if ( in_listTasks.IsEmpty() )
	{
		in_listTasks.Term();
		return true;
	}
	return false;
}

void CAkDeviceBase::CleanDeadTasks(TaskList& in_listTasks)
{
	TaskList::IteratorEx it = in_listTasks.BeginEx();
	while (it != in_listTasks.End())
	{
		CAkStmTask* pTask = (*it);
		if (pTask->IsToBeDestroyed() && pTask->CanBeDestroyed())
		{
			it = in_listTasks.Erase(it);
			pTask->InstantDestroy();
		}
		else
		{
			// Cannot be destroyed now (has I/O pending). Try again later.
			++it;
		}
	}
}

// Called once when I/O thread starts.
void CAkDeviceBase::OnThreadStart()
{
	// Stamp time the first time.
    AKPLATFORM::PerformanceCounter( &m_time );
}

void CAkDeviceBase::AddTask(CAkStmTask* in_pStmTask)
{
	AkAutoLock<CAkLock> gate(m_lockTasksList);

	if (in_pStmTask->StmType() == AK_StmTypeCaching)
		m_listCachingTasks.AddFirst(in_pStmTask);
	else
		m_listTasks.AddFirst(in_pStmTask);

	in_pStmTask->SetDevice(this);

	//Increment the proper counter, we're ready to go.
	if (in_pStmTask->StmType() == AK_StmTypeStandard)
		StdSemIncr();
	else
		AutoSemIncr();	//Auto and Caching go here.
}

void CAkDeviceBase::RemoveTask(CAkStmTask* in_pStmTask)
{
	AkAutoLock<CAkLock> gate(m_lockTasksList);
	if (in_pStmTask->IsCachingStream())
		m_listCachingTasks.Remove(in_pStmTask);
	else
		m_listTasks.Remove(in_pStmTask);
	
	in_pStmTask->SetDevice(nullptr);
	in_pStmTask->pNextLightItem = nullptr;
}

void CAkDeviceBase::UpdateCachingPriority(CAkStmTask * in_pStmTask, AkPriority in_uNewPriority)
{
	AkPriority uOldPriority = in_pStmTask->Priority();

	if (uOldPriority != in_uNewPriority)
	{
		in_pStmTask->SetPriority(in_uNewPriority);

		// Restart all the tasks to re-evaluate the priorities.
		AkAutoLock<CAkLock> scheduling( m_lockTasksList );
		for ( TaskList::IteratorEx it = m_listCachingTasks.BeginEx(); it != m_listCachingTasks.End(); ++it )
		{
			(*it)->StartCaching();
		}
	}
}

CAkStmTask * CAkDeviceBase::SchedulerFindNextCachingTask()
{
	CAkStmTask * pTaskToSchedule = NULL;

	AkUInt32 uCurrentCachePinned = 0;
	bool bStreamDestroyed = false;

	for (TaskList::IteratorEx it = m_listCachingTasks.BeginEx(); it != m_listCachingTasks.End();)
	{
		if ( (*it)->IsToBeDestroyed() && (*it)->CanBeDestroyed() )
		{
			CAkStmTask * pTaskToDestroy = (*it);
			it = m_listCachingTasks.Erase( it );
			pTaskToDestroy->InstantDestroy();
			bStreamDestroyed = true;
		}
		else
		{
			uCurrentCachePinned += (*it)->GetVirtualBufferingSize();
			++it;
		}
	}
	
	// Find the highest priority task that has been serviced the least recently.
	for (TaskList::IteratorEx it = m_listCachingTasks.BeginEx(); it != m_listCachingTasks.End(); ++it )
	{
		if ( bStreamDestroyed ) 
		{
			// A stream was destroyed.  Start any caching streams that may have been stopped 
			//	due to hitting the memory limit. We may be able to fit them in now.
			(*it)->StartCaching();
		}

		if ( (*it)->RequiresScheduling() )
		{
			// Choose task with highest priority to schedule
			if ( !pTaskToSchedule || (*it)->Priority() > pTaskToSchedule->Priority() )
			{
				pTaskToSchedule = (*it);
			}
		}
	}
	
	if ( pTaskToSchedule != NULL )
	{
		//Now check to see that if the amount of memory needed fits under our cache locked memory limit.
		AkUInt32 uMemNeededForTask = pTaskToSchedule->GetNominalBuffering() - pTaskToSchedule->GetVirtualBufferingSize();

		if ((uCurrentCachePinned + uMemNeededForTask) > m_uMaxCachePinnedBytes )
		{
			// Choose task with lowest priority to bump, if possible
			CAkStmTask * pTaskToBump;
			do
			{
				pTaskToBump = NULL;
				for (TaskList::IteratorEx it = m_listCachingTasks.BeginEx(); it != m_listCachingTasks.End(); ++it )
				{
					if ( (*it)->GetVirtualBufferingSize() > 0 &&
						(	( (*it)->Priority() < pTaskToSchedule->Priority() ) &&
							( !pTaskToBump || (*it)->Priority() < pTaskToBump->Priority() )
						 ) )
					{
						pTaskToBump = (*it);
					}
				}

				if (pTaskToBump)
				{
					uCurrentCachePinned -= pTaskToBump->StopCaching(uMemNeededForTask);
					if ( (uCurrentCachePinned + uMemNeededForTask) <= m_uMaxCachePinnedBytes )
						break; //we have enough free space
				}
			}
			while ( pTaskToBump != NULL );

			// If we could not free enough memory, do not schedule a new task 
			if ( (uCurrentCachePinned + uMemNeededForTask) > m_uMaxCachePinnedBytes  )
			{
				pTaskToSchedule->StopCaching(0);
				pTaskToSchedule = NULL;
			}
		}
	}

	m_uCurrentCachePinnedData = uCurrentCachePinned;

	return pTaskToSchedule;
}

////////////////////////////////////////

// Scheduler algorithm.
// Finds the next task for which an I/O request should be issued.
// Return: If a task is found, a valid pointer to a task is returned, as well
// as the operation's deadline (for low-level IO heuristics).
// Otherwise, returns NULL.
// Sync: 
// 1. Locks task list ("scheduler lock").
// 2. If it chooses a standard stream task, the stream becomes "I/O locked" before the scheduler lock is released.
CAkStmTask * CAkDeviceBase::SchedulerFindNextTask( 
	AkReal32 &		out_fOpDeadline		// Returned deadline for this transfer.
    )
{
    // Start scheduling.
    // ------------------------------------
	
	// Lock tasks list.
    AkAutoLock<CAkLock> scheduling( m_lockTasksList );

    // Stamp time.
    AKPLATFORM::PerformanceCounter( &m_time );

    // If m_bDoWaitMemoryChange, no automatic stream operation can be scheduled because memory is full
    // and will not be reassigned until someone calls NotifyMemChange().
    // Therefore, we only look for a pending standard stream (too bad if memory is freed in the meantime).
    if ( CannotScheduleAutoStreams() )
        return ScheduleStdStmOnly( out_fOpDeadline );

    TaskList::IteratorEx it = m_listTasks.BeginEx();
    CAkStmTask * pTask = NULL;
	AkReal32 fSmallestDeadline = 0.f;
	bool bLeastBufferedTaskRequiresScheduling = false;
    
    // Get first valid task's values for comparison.
    while ( it != m_listTasks.End() )
    {
		// Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed() )
        {
			if ( (*it)->CanBeDestroyed() )
			{
				// Clean up.
				CAkStmTask * pTaskToDestroy = (*it);
	            it = m_listTasks.Erase( it );
				pTaskToDestroy->InstantDestroy();
	        }
			else 
			{
				// Not ready to be destroyed: wait until next turn.
				++it;
			}
        }
        else if ( !(*it)->ReadyForIO() )
            ++it;   // Skip this one.
        else
        {
            // Current iterator refers to a task that is not scheduled to be destroyed, and is ready for I/O. Proceed.
			// pTask and fSmallestDeadline are the current task and associated effective deadline chosen for I/O.
            pTask = (*it);
			fSmallestDeadline = pTask->EffectiveDeadline();
			bLeastBufferedTaskRequiresScheduling = pTask->RequiresScheduling();
			break;  
        }
    }    

    if ( !pTask )
    {
        // No task was ready for I/O. Leave.
        return SchedulerFindNextCachingTask();
    }

    // Find task with smallest effective deadline.
    // If a task has a deadline equal to 0, this means we are starving; user throughtput is greater than
    // low-level bandwidth. In that situation, starving streams are chosen according to their priority.
    // If more than one starving stream has the same priority, the scheduler chooses the one that has been 
    // waiting for I/O for the longest time.
    // Note 1: This scheduler does not have any idea of the actual low-level throughput, nor does it try to
    // know it. It just reacts to the status of its streams at a given moment.
    // Note 2: By choosing the highest priority stream only when we encounter starvation, we take the bet
    // that the transfer will complete before the user has time consuming its data. Therefore it remains 
    // possible that high priority streams starve.
    // Note 3: Automatic streams that just started are considered starving. They are chosen according to
    // their priority first, in a round robin fashion (starving mechanism).
    // Note 4: If starving mode lasts for a long time, low-priority streams will stop being chosen for I/O.
	// Note 5: Tasks that are actually signaled (RequireScheduling) have priority over other tasks. A task 
	// that is unsignaled may still have a smaller deadline than other tasks (because tasks must be double-
	// buffered at least). However, an unsignaled task will only be chosen if there are no signaled task.

    // Start with the following task.
    AKASSERT( pTask );
    ++it;

    while ( it != m_listTasks.End() )
    {
        // Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed() )
        {
			if ( (*it)->CanBeDestroyed() )
			{
				// Clean up.
				CAkStmTask * pTaskToDestroy = (*it);
	            it = m_listTasks.Erase( it );
				pTaskToDestroy->InstantDestroy();
	        }
			else 
			{
				// Not ready to be destroyed: wait until next turn.
				++it;
			}
        }
        else if ( (*it)->ReadyForIO() )
        {
			AkReal32 fDeadline = (*it)->EffectiveDeadline();

			if ( !bLeastBufferedTaskRequiresScheduling && (*it)->RequiresScheduling() )
			{
				// This is the first task that we run into which actually requires action from 
				// scheduler: pick it. 
				pTask = (*it);
				fSmallestDeadline = fDeadline;
				bLeastBufferedTaskRequiresScheduling = true;
			}
			else if ( !bLeastBufferedTaskRequiresScheduling || (*it)->RequiresScheduling() )
			{
				if ( fDeadline == 0 )
				{
					// Deadline is zero: starvation mode.
					// Choose task with highest priority among those that are starving.
					if ( (*it)->Priority() > pTask->Priority() || fSmallestDeadline > 0 )
					{
						pTask = (*it);
						fSmallestDeadline = fDeadline;
						bLeastBufferedTaskRequiresScheduling = pTask->RequiresScheduling();
					}
					else if ( (*it)->Priority() == pTask->Priority() )
					{
						// Same priority: choose the one that has waited the most.
						if ( (*it)->TimeSinceLastTransfer( GetTime() ) > pTask->TimeSinceLastTransfer( GetTime() ) )
						{
							pTask = (*it);
							fSmallestDeadline = fDeadline;
							bLeastBufferedTaskRequiresScheduling = pTask->RequiresScheduling();
						}
					}
				}
				else if ( fDeadline < fSmallestDeadline )
				{
					// Deadline is not zero: low-level has enough bandwidth. Just take the task with smallest deadline.
					// We take the bet that this transfer will have time to occur fast enough to properly service
					// the others on next pass.
					pTask = (*it);
					fSmallestDeadline = fDeadline;
					bLeastBufferedTaskRequiresScheduling = pTask->RequiresScheduling();
				}
			}
            ++it;
        }
        else
            ++it;   // Skip this task: it is not waiting for I/O.
	}

	out_fOpDeadline = fSmallestDeadline;

	if ( bLeastBufferedTaskRequiresScheduling )
	    return pTask;
	return SchedulerFindNextCachingTask();
}

// Scheduler algorithm: standard stream-only version.
// Finds next task among standard streams only (typically when there is no more memory).
// Note: standard streams that are ready for IO are always signaled.
CAkStmTask * CAkDeviceBase::ScheduleStdStmOnly(
	AkReal32 &	out_fOpDeadline	// Returned deadline for this transfer.
    )
{
    TaskList::IteratorEx it = m_listTasks.BeginEx();
    CAkStmTask * pTask = NULL;
    
    // Get first valid task's values for comparison.
    while ( it != m_listTasks.End() )
    {
        // Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed() )
        {
            if ( (*it)->CanBeDestroyed() )
			{
				// Clean up.
				CAkStmTask * pTaskToDestroy = (*it);
	            it = m_listTasks.Erase( it );
				pTaskToDestroy->InstantDestroy();
	        }
			else 
			{
				// Not ready to be destroyed: wait until next turn.
				++it;
			}
        }
        else if ( (*it)->StmType() == AK_StmTypeStandard &&
                    (*it)->ReadyForIO() )
        {
            // Current iterator refers to a standard stream task that is not scheduled to be destroyed, 
            // and that is pending. Proceed.
            pTask = (*it);
            break;
        }
        else
            ++it;
    }    

    if ( !pTask )
    {
        // No task was ready for I/O. Leave.
        return NULL;
    }

    // fSmallestDeadline is the smallest effective deadline found to date. Used to find the next task for I/O.
    AkReal32 fSmallestDeadline = pTask->EffectiveDeadline();
    
    // Find task with smallest effective deadline.
    // See note in SchedulerFindNextTask(). It is the same algorithm, except that automatic streams are excluded.
    
    // Start with the following task.
    AKASSERT( pTask );
    ++it;

    while ( it != m_listTasks.End() )
    {
        // Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed() )
        {
            if ( (*it)->CanBeDestroyed() )
			{
				// Clean up.
				CAkStmTask * pTaskToDestroy = (*it);
	            it = m_listTasks.Erase( it );
				pTaskToDestroy->InstantDestroy();
	        }
			else 
			{
				// Not ready to be destroyed: wait until next turn.
				++it;
			}
        }
        else if ( (*it)->StmType() == AK_StmTypeStandard &&
                    (*it)->ReadyForIO() )
        {
            AkReal32 fDeadline = (*it)->EffectiveDeadline(); 

            if ( fDeadline == 0 )
            {
                // Deadline is zero. Starvation mode: user throughput is greater than low-level bandwidth.
                // Choose task with highest priority among those that are starving.
                if ( (*it)->Priority() > pTask->Priority() || fSmallestDeadline > 0 )
                {
                    pTask = (*it);
                    fSmallestDeadline = fDeadline;
                }
                else if ( (*it)->Priority() == pTask->Priority() )
                {
                    // Same priority: choose the one that has waited the most.
                    if ( (*it)->TimeSinceLastTransfer( GetTime() ) > pTask->TimeSinceLastTransfer( GetTime() ) )
                    {
                        pTask = (*it);
                        fSmallestDeadline = fDeadline;
                    }
                }
            }
            else if ( fDeadline < fSmallestDeadline )
            {
                // Deadline is not zero: low-level has enough bandwidth. Just take the task with smallest deadline.
                // We take the bet that this transfer will have time to occur fast enough to properly service
                // the others.
                pTask = (*it);
                fSmallestDeadline = fDeadline;
            }
            ++it;
        }
        else
            ++it;   // Skip this task; not waiting for I/O.
    }

    AKASSERT( pTask );

	out_fOpDeadline = fSmallestDeadline;

    return pTask;

}

void CAkDeviceBase::ForceCleanup(
								 bool in_bKillLowestPriorityTask,				// True if the device should kill the task with lowest priority.
								 AkPriority in_priority							// Priority of the new task if applicable. Pass AK_MAX_PRIORITY to ignore.
								 )
{
	bool cachingTaskKilled = ForceTaskListCleanup( in_bKillLowestPriorityTask, in_priority, m_listCachingTasks );
	ForceTaskListCleanup( in_bKillLowestPriorityTask && !cachingTaskKilled, in_priority, m_listTasks );
}

// Forces the device to clean up dead tasks. 
bool CAkDeviceBase::ForceTaskListCleanup(
	bool in_bKillLowestPriorityTask,				// True if the device should kill the task with lowest priority.
	AkPriority in_priority,							// Priority of the new task if applicable. Pass AK_MAX_PRIORITY to ignore.
	TaskList& in_listTasks
	)
{
	AkAutoLock<CAkLock> scheduling( m_lockTasksList );

	CAkStmTask * pTaskToKill = NULL;
	TaskList::IteratorEx it = in_listTasks.BeginEx();
    while ( it != in_listTasks.End() )
    {
        // Cleanup while we're at it.
        if ( (*it)->IsToBeDestroyed() )
        {
            if ( (*it)->CanBeDestroyed() )
			{
				// Clean up.
				CAkStmTask * pTaskToDestroy = (*it);
	            it = in_listTasks.Erase( it );
				pTaskToDestroy->InstantDestroy();
	        }
			else
			{
				// Not ready to be destroyed: wait until next turn.
				++it;
			}			
        }
		// Otherwise, check if it is a candidate to be killed.
		else if ( in_bKillLowestPriorityTask
				&& ( !pTaskToKill || (*it)->Priority() < pTaskToKill->Priority() )
				&& (*it)->Priority() < in_priority
				&& (*it)->ReadyForIO() )
        {
            // Current iterator refers to a standard stream task that is not scheduled to be destroyed, 
            // and that is pending. Proceed.
            pTaskToKill = (*it);
			++it;
        }
        else
            ++it;
    }

	// Kill the task if any.
	if ( pTaskToKill )
	{
		pTaskToKill->Kill();
		return true;
	}

	return false;
}

// Cache management.
void CAkDeviceBase::FlushCache()
{
	AkAutoLock<CAkIOThread> lock( *this );
	m_mgrMemIO.FlushCache();
}

// Perform a direct transfer from the cache, from client thread.
// Sync: task must be locked. Device is locked herein.
bool CAkDeviceBase::ExecuteCachedTransfer( CAkStmTask * in_pTask )
{
	AKASSERT( in_pTask != NULL );

	AkAutoLock<CAkIOThread> lock(*this);

	// Get info for IO.
	AkFileDesc * pFileDesc;
	CAkLowLevelTransfer * pNewLowLevelXfer;
	bool bTrasferExists;
	CAkStmMemView * pMemView = in_pTask->PrepareTransfer( 
		pFileDesc, 
		pNewLowLevelXfer,
		bTrasferExists,
		true );
	
	AKASSERT( !pNewLowLevelXfer || !"Asked for cached transfer only" );
	AKASSERT( !pMemView || !bTrasferExists || !"PrepareTransfer() should only return mem views that do not have pending transfers.");

	// Update task after transfer.  Returns true if buffer was added
	return in_pTask->Update(pMemView, AK_Success, false);
}

// Device Profile Ex interface.
// --------------------------------------------------------
#ifndef AK_OPTIMIZED

void CAkDeviceBase::OnProfileStart()
{
	m_lockTasksList.Lock();
	m_bTasksListLockAcquired = true;
}

void CAkDeviceBase::OnProfileEnd()
{
	m_bTasksListLockAcquired = false;
	m_lockTasksList.Unlock();
}

// Caps/desc.
void CAkDeviceBase::GetDesc( 
    AkDeviceDesc & out_deviceDesc 
    )
{
    m_pLowLevelHook->GetDeviceDesc( out_deviceDesc );
}

void CAkDeviceBase::GetData(
    AkDeviceData &  out_deviceData
    )
{
	AkAutoLock<CAkIOThread> lock(*this);

	m_mgrMemIO.GetProfilingData( m_uGranularity, out_deviceData );
	out_deviceData.deviceID = m_deviceID;
	out_deviceData.uGranularity = m_uGranularity;
	out_deviceData.uNumActiveStreams = m_uNumActiveStreams;
	out_deviceData.uTotalBytesTransferred = m_uBytesThisInterval;
	out_deviceData.uLowLevelBytesTransferred = m_uBytesLowLevelThisInterval;
	
	m_uBytesThisSession += m_uBytesThisInterval;
	AKASSERT( m_uBytesThisInterval >= m_uBytesLowLevelThisInterval );
	m_uCacheBytesThisSession += (m_uBytesThisInterval - m_uBytesLowLevelThisInterval);
	AKASSERT( m_uCacheBytesThisSession <= m_uBytesThisSession );

	out_deviceData.fAvgCacheEfficiency = 0.f;
	if (m_uBytesThisSession > 0)
		out_deviceData.fAvgCacheEfficiency = ((AkReal32)m_uCacheBytesThisSession / (AkReal32)m_uBytesThisSession) * 100.f;
	
	AKASSERT(out_deviceData.fAvgCacheEfficiency >= 0.f && out_deviceData.fAvgCacheEfficiency <= 100.f);

	out_deviceData.uNumLowLevelRequestsCompleted = m_uNumLowLevelRequests;
	out_deviceData.uNumLowLevelRequestsCancelled = m_uNumLowLevelRequestsCancelled;
	out_deviceData.uNumLowLevelRequestsPending = AkMin(m_uMaxConcurrentIO,GetNumConcurrentIO());
	out_deviceData.uCustomParam = m_pLowLevelHook->GetDeviceData();
	out_deviceData.uCachePinnedBytes = m_uCurrentCachePinnedData;
	
	m_uBytesLowLevelThisInterval = 0;
	m_uBytesThisInterval = 0;
	m_uNumLowLevelRequests = 0;
	m_uNumLowLevelRequestsCancelled = 0;
}

bool CAkDeviceBase::IsNew( )
{
    return m_bIsNew;
}

void CAkDeviceBase::ClearNew( )
{
    m_bIsNew = false;
}


AKRESULT CAkDeviceBase::StartMonitoring( )
{
    m_bIsMonitoring = true;
	// Reset transfer statistics.
	m_uBytesLowLevelThisInterval	= 0;
	m_uBytesThisInterval = 0;
	m_uNumLowLevelRequests = 0;
	m_uNumLowLevelRequestsCancelled = 0;

	m_uBytesThisSession = 0;
	m_uCacheBytesThisSession = 0;

    return AK_Success;
}

void CAkDeviceBase::StopMonitoring( )
{
    m_bIsMonitoring = false;
}

// Stream profiling: GetNumStreams.
// Clears profiling array.
// Inspects all streams. 
// Grants permission to destroy if scheduled for destruction and AND not new.
// Copies all stream profile interfaces into its array, that will be accessed
// by IAkStreamProfile methods.
AkUInt32 CAkDeviceBase::GetNumStreams()
{
	// m_lockTasksList (OnProfileStart/OnProfileEnd) must be acquired before calling this 
	AKASSERT(m_bTasksListLockAcquired);

	m_arStreamProfiles.RemoveAll();
	AkZeroMemLarge(m_arStreamProfiles.Data(), sizeof(AK::IAkStreamProfile*) * m_arStreamProfiles.Reserved());

	m_uNumActiveStreams = 0;

	CountStreamsInTaskList(m_listTasks);
	CountStreamsInTaskList(m_listCachingTasks);

	return m_arStreamProfiles.Length();
}

void CAkDeviceBase::CountStreamsInTaskList(TaskList& in_listTasks)
{
    TaskList::Iterator it = in_listTasks.Begin();
    while ( it != in_listTasks.End() )
    {
        // Profiler will let the device destroy this stream if it is not "new" (was already
		// read). If it is ready to be destroyed and not new, it could be because the file could not 
		// be opened, so the client will have destroyed the stream. In such a case the profiler allow  
		if ( (*it)->ProfileIsToBeDestroyed() 
			&& ( !(*it)->IsProfileNew() 
				|| !(*it)->IsProfileReady() ) )
		{
			(*it)->ProfileAllowDestruction();
		}
        else
        {
            // Copy into profiler list.
            m_arStreamProfiles.AddLast( (*it)->GetStreamProfile() );
			// Also, increment the number of active streams for device profiling.
			m_uNumActiveStreams += ( (*it)->WasActive() ) ? 1 : 0;
        }

        ++it;
    }
    
}

// Note. The following functions refer to streams by index, which must honor the call to GetNumStreams().
AK::IAkStreamProfile* CAkDeviceBase::GetStreamProfile(AkUInt32 in_uStreamIndex)
{
	// m_lockTasksList (OnProfileStart/OnProfileEnd) must be acquired before calling this 
	AKASSERT(m_bTasksListLockAcquired);
	
	// Get stream profile and return.
	if (in_uStreamIndex < m_arStreamProfiles.Length())
	{
		return m_arStreamProfiles[in_uStreamIndex];
	}
	else
	{
		return nullptr;
	}
}
#endif


//-----------------------------------------------------------------------------
//
// Stream objects implementation.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: class CAkStmTask
// Desc: Base implementation common to streams. Defines the interface used by
//       the device scheduler.
//-----------------------------------------------------------------------------
CAkStmTask::CAkStmTask()
: m_pFileDesc(NULL)
, m_pDevice(NULL)
, m_pszStreamName( NULL )
, m_uLLBlockSize(1)
#ifndef AK_OPTIMIZED
//Not set m_ulStreamID;
, m_uBytesTransfered( 0 )
#endif
, m_priority(AK_DEFAULT_PRIORITY)
, m_eOp(OpOpen)
, m_eOpenStatus(AK_StmOpenStatus_Idle)
, m_bHasReachedEof( false )
, m_bIsToBeDestroyed( false )
, m_bRequiresScheduling( false )
, m_bLanguageSpecific( false )
, m_bIsReadyForIO( false )
#ifndef AK_OPTIMIZED
, m_bIsNew( true )
, m_bIsProfileDestructionAllowed( false )
, m_bWasActive( false )
, m_bCanClearActiveProfile( false )
#endif
{
}

CAkStmTask::~CAkStmTask()
{
	if (m_pOpenData)
		AkDelete(AkMemID_Streaming, m_pOpenData);

	if (m_pFileDesc)
	    m_pDevice->GetLowLevelHook()->Close( m_pFileDesc );

	if (m_pszStreamName)
	{
		AkFree(AkMemID_Streaming, m_pszStreamName);
		m_pszStreamName = nullptr;
	}
}


#ifndef AK_OPTIMIZED
// Profiling: Get stream information. This information should be queried only once, since it is unlikely to change.
void CAkStmTask::GetStreamRecord( 
    AkStreamRecord & out_streamRecord
    )
{
	out_streamRecord.deviceID = m_pDevice ? m_pDevice->GetDeviceID( ) : AK_INVALID_DEVICE_ID;
	if ( m_pszStreamName != NULL )
	{
		AK_OSCHAR_TO_UTF16( out_streamRecord.szStreamName, m_pszStreamName, AK_MONITOR_STREAMNAME_MAXLENGTH );    
		out_streamRecord.szStreamName[AK_MONITOR_STREAMNAME_MAXLENGTH - 1] = '\0';
		out_streamRecord.uStringSize = (AkUInt32)AKPLATFORM::AkUtf16StrLen( out_streamRecord.szStreamName ) + 1;
	}
	else
	{
		out_streamRecord.uStringSize = 0;    
		out_streamRecord.szStreamName[0] = 0;
	}

	out_streamRecord.idFile = m_idFile;
	out_streamRecord.uStreamID = m_uStreamID;
	out_streamRecord.bIsAutoStream = (m_eStmType == AK_StmTypeAutomatic || m_eStmType == AK_StmTypeCaching);
	out_streamRecord.bIsCachingStream = (m_eStmType == AK_StmTypeCaching);
	if (m_pFileDesc)
	{
		out_streamRecord.uFileSize = m_pFileDesc->iFileSize;
	}
	else
	{
		//Monitoring of AkOpenTask will go here.
		out_streamRecord.uFileSize = 0;
	}
}	 
#endif


// Stream info access.
// Sync: None.
void CAkStmTask::GetInfo(AkStreamInfo& out_info)
{
	out_info.deviceID = m_pDevice ? m_pDevice->GetDeviceID() : AK_INVALID_DEVICE_ID;
	out_info.pszName = m_pszStreamName;	
	out_info.bIsLanguageSpecific = m_bLanguageSpecific;	
	out_info.uSize = m_pFileDesc ? m_pFileDesc->iFileSize : 0;
	out_info.bIsOpen = (m_pFileDesc != NULL);
}

AKRESULT CAkStmTask::SetStreamName(const AkOSChar* in_pszStreamName)
{
	AkAutoLock<CAkLock> stmLock(m_lockStatus);

	if (m_pszStreamName != NULL)
	{
		AkFree(AkMemID_Streaming, m_pszStreamName);
		m_pszStreamName = nullptr;
	}

	if (in_pszStreamName != NULL)
	{
		size_t len = AKPLATFORM::OsStrLen(in_pszStreamName) + 1;
		size_t size = AkMin(len, AK_MONITOR_STREAMNAME_MAXLENGTH);
		m_pszStreamName = (AkOSChar*)AkAlloc(AkMemID_Streaming, size * sizeof(AkOSChar));
		if (m_pszStreamName == NULL)
			return AK_InsufficientMemory;

		if (len < AK_MONITOR_STREAMNAME_MAXLENGTH)
			AKPLATFORM::SafeStrCpy(m_pszStreamName, in_pszStreamName, size);
		else
		{
			// Truncate first characters. GetFilename() is the whole file path pushed by the WAL, and it may be longer
			// than AK_MONITOR_STREAMNAME_MAXLENGTH.
			const AkOSChar* pSrcStr = in_pszStreamName + len - AK_MONITOR_STREAMNAME_MAXLENGTH;
			AKPLATFORM::SafeStrCpy(m_pszStreamName, pSrcStr, size);
			m_pszStreamName[0] = '.';
			m_pszStreamName[1] = '.';
			m_pszStreamName[2] = '.';
		}
	}
	return AK_Success;
}

AKRESULT CAkStmTask::Open(const AkFileOpenData& in_FileOpen, bool in_bSync, bool in_bAuto)
{
	m_pOpenData = AkNew(AkMemID_Streaming, AkDeferredOpenData());
	if (!m_pOpenData)
		return AK_InsufficientMemory;

	AKRESULT res = m_pOpenData->Init(in_FileOpen);
	if (res == AK_Success)
	{
		if (in_bSync)
			SetBlockedStatus();

#ifndef AK_OPTIMIZED
		m_uStreamID = CAkStreamMgr::GetNewStreamID();
		m_idFile = m_pOpenData->fileID;
#endif

		m_pOpenData->flagsStorage.bIsAutomaticStream = in_bAuto;
		m_pOpenData->pCallback = CAkStmTask::OnDeferredOpen;
		m_pOpenData->pCookie = this;
		m_eOp = OpOpen;

		if (m_pOpenData->pszFileName)
		{
			SetStreamName(m_pOpenData->pszFileName);
		}
		else
		{
			const unsigned long MAX_NUMBER_STR_SIZE = 11;
			AkOSChar szName[MAX_NUMBER_STR_SIZE];
			AK_OSPRINTF(szName, MAX_NUMBER_STR_SIZE, AKTEXT("%u"), m_pOpenData->fileID);
			SetStreamName(szName);
		}

#ifdef AK_SUPPORT_THREAD
		AkEvent event;
		if (in_bSync)
		{
			if (AKPLATFORM::AkCreateEvent(event) != AK_Success)
			{
				AKASSERT(!"Could not create synchronization event");
				return AK_Fail;
			}

			m_pIOEvent = &event;
		}
#endif

		// This will start the task.
		res = MoveToNextDevice();

#ifdef AK_SUPPORT_THREAD
		if (in_bSync)
		{
			if (res == AK_Success)
			{
				AKPLATFORM::AkWaitForEvent(event);

				res = IsFileOpened() ? AK_Success : AK_Fail;
			}

			AKPLATFORM::AkDestroyEvent(event);
			m_pIOEvent = nullptr;
		}
#else
		if (in_bSync && res == AK_Success)
		{
			m_pDevice->WaitForIOCompletion(this);
			res = IsFileOpened() ? AK_Success : AK_Fail;
		}
#endif
	}
	return res;
}

bool AK::StreamMgr::CAkStmTask::CanBeDestroyed()
{
	// A task cannot be destroyed while there is an open request in flight.
	// When we receive the response, the stream will transition to Cancelled state, at which point it can get cleaned up.
	return m_eOpenStatus != AK_StmOpenStatus_InFlight;
}

//Move this task to a different device
AKRESULT CAkStmTask::MoveToNextDevice()
{
	AKASSERT(CurrentOp() == CAkStmTask::OpOpen && m_pOpenData != nullptr);
	if (CurrentOp() != CAkStmTask::OpOpen || m_pOpenData == nullptr)
		return AK_InvalidParameter;

	IAkFileLocationResolver* pResolver = GetFileLocationResolver();

	AkDeviceID uDeviceID = m_pDevice ? m_pDevice->GetDeviceID() : AK_INVALID_DEVICE_ID;

	AKRESULT eRes = AK_Success;

	eRes = pResolver->GetNextPreferredDevice(*m_pOpenData, uDeviceID);
	if (eRes == AK_NotImplemented)
	{
		if (uDeviceID != 0)
			uDeviceID = 0;	//In this AKStreamMgr implementation, the AKDeviceID is simply the index of the device. So the only device available is 0.
		else
			return AK_NotImplemented;
	}
	else if (eRes != AK_Success)
		return eRes;

	if (uDeviceID == AK_INVALID_DEVICE_ID)
	{
		AKASSERT(eRes != AK_Success);	//Resolver should return a valid id when returning AK_Success
		return AK_NotImplemented;
	}

	if (m_pDevice)
		m_pDevice->RemoveTask(this);

	//At this point we are detached from any device.
	{
		AkAutoLock<CAkLock> stmGate(m_lockStatus);
		SetReadyForIO(true);
		m_bRequiresScheduling = true;
		m_pOpenData->uDeviceID = uDeviceID;
	}

	if (!CAkStreamMgr::AddTask(this, uDeviceID))
	{
		AKASSERT(!"Unknown device ID returned by IAkFileLocationResolver.");
		return AK_InvalidID;
	}

	return AK_Success;
}

/*static*/ void CAkStmTask::OnDeferredOpen(AkAsyncFileOpenData* in_pOpenInfo, AKRESULT in_eResult)
{
	AKASSERT(in_eResult != AK_Deferred);

	CAkStmTask* pTask = (CAkStmTask *)in_pOpenInfo->pCookie;
	pTask->HandleFileOpenResult(in_pOpenInfo, in_eResult);

#ifdef AK_SUPPORT_THREAD
	if (pTask->m_pIOEvent != nullptr && pTask->m_eOpenStatus == AK_StmOpenStatus_Complete)
	{
		AKPLATFORM::AkSignalEvent(*pTask->m_pIOEvent);
	}
#endif
}

AKRESULT AK::StreamMgr::CAkStmTask::PrepareOpen(AkAsyncFileOpenData*& in_pLLOpen)
{
	AkAutoLock<CAkLock> status(m_lockStatus);

	AKASSERT(m_eOpenStatus == AK_StmOpenStatus_Idle);

	m_eOpenStatus = AK_StmOpenStatus_InFlight;

	//This task does not requires any action from the I/O thread anymore.
	SetReadyForIO(false);
	if (m_bRequiresScheduling)
	{
		m_bRequiresScheduling = false;
		if (m_pOpenData->pFlags->bIsAutomaticStream)
			m_pDevice->AutoSemDecr();
		else
			m_pDevice->StdSemDecr();
	}

	m_pDevice->IncrementIOCount();
	if (m_pOpenData)
	{
		in_pLLOpen = m_pOpenData;
		return AK_Success;
	}
	else
	{
		// This stream will never receive a callback from LLIO, so mark the open as complete.
		m_eOpenStatus = AK_StmOpenStatus_Complete;
		return AK_Fail;
	}
}

void CAkStmTask::HandleFileOpenResult(AkAsyncFileOpenData* in_pOpenData, AKRESULT in_eResult)
{
	AKASSERT(in_pOpenData == m_pOpenData);	//This should be OUR structure.
	AKASSERT(m_pFileDesc == nullptr);

	m_lockStatus.Lock();

	m_pDevice->DecrementIOCount();	//Increment done in PrepareOpen

	bool bFinished = in_eResult == AK_Success;
	if (!bFinished && !m_bIsToBeDestroyed)
	{
		// File open failed. Regardless of reason, we may need to fall back to a different path.
		// If it was marked as "language specific" and it is a bank or a file package, try without the language on the same device
		if (m_pOpenData->flagsStorage.bIsLanguageSpecific &&
			m_pOpenData->flagsStorage.uCompanyID == AKCOMPANYID_AUDIOKINETIC &&
			(AK::IsBankCodecID(m_pOpenData->flagsStorage.uCodecID)))
		{
			m_pOpenData->flagsStorage.bIsLanguageSpecific = false;

			SetReadyForIO(true);
			m_bRequiresScheduling = true;
			if (m_pOpenData->flagsStorage.bIsAutomaticStream)
				m_pDevice->AutoSemIncr();
			else
				m_pDevice->StdSemIncr();
			
			bFinished = false;
		}
		else
		{
			//Reset the language flag, in case we modified it above, in a previous pass
			m_pOpenData->flagsStorage.bIsLanguageSpecific = (bool)m_pOpenData->bOriginalLanguageFlag;

			//Try the next device.
			m_lockStatus.Unlock();  //Can't be locked when moving tasks in devices.
			AKRESULT eNextDeviceResult = MoveToNextDevice();
			if (eNextDeviceResult != AK_NotImplemented)
				in_eResult = eNextDeviceResult;

			m_lockStatus.Lock();

			bFinished = (in_eResult != AK_Success);
		}
	}

	if (m_bIsToBeDestroyed)
	{
		//This can happen if the file was released before the open completed; just close the file and wait to be cleaned up
		if (m_pOpenData->pFileDesc)
			m_pDevice->GetLowLevelHook()->Close(m_pOpenData->pFileDesc);

		m_eOpenStatus = AK_StmOpenStatus_Complete;

		AkDelete(AkMemID_Streaming, m_pOpenData);
		m_pOpenData = nullptr;
		m_pDevice->SignalIOCompleted(this);

		VerifyOpenResult(in_eResult);
		m_lockStatus.Unlock();  //Can't be locked when moving tasks in devices.
		return;
	}

	if (bFinished)
	{
		m_eOpenStatus = AK_StmOpenStatus_Complete;

		// File open operation has finished, whether it is success or failure.
		// Time to notify the subclass variant (Std or Auto) so it can take over the stream
		
		// Copy the file open data to the stream data
		m_bLanguageSpecific = m_pOpenData->pFlags->bIsLanguageSpecific;
		m_pFileDesc = m_pOpenData->pFileDesc;
		if (m_pOpenData->GetStreamName())	//The IOHook can override the name. See WG-61616.
			SetStreamName(m_pOpenData->GetStreamName());

		if (in_eResult == AK_Success)
		{
			// Now that the file is opened, we can determine the LLIO block size.
			AkUInt32 uGranularity = m_pDevice->GetGranularity();
			const AkUInt32 uLLBlockSize = m_pDevice->GetLowLevelHook()->GetBlockSize(*m_pFileDesc);
			if (!uLLBlockSize
				|| uLLBlockSize > uGranularity
				|| (uGranularity % uLLBlockSize) > 0)
			{
				AK_MONITOR_ERROR(AK::Monitor::ErrorCode_IncompatibleIOSettings);
				AKASSERT(!"Invalid Low-Level I/O block size. Must be >= 1 && <= Granularity and a multiple of the granularity");
			}
			m_uLLBlockSize = uLLBlockSize;
		}
		
		AkDeferredOpenData* pOpenData = m_pOpenData;
		m_pOpenData = nullptr;

		OnFileDeferredOpen(*pOpenData, in_eResult);

		VerifyOpenResult(in_eResult);
		m_lockStatus.Unlock();

#ifndef AK_OPTIMIZED
		if (in_eResult != AK_Success || pOpenData->pFileDesc == nullptr)
		{
			AK::StreamMgr::MonitorFileOpenError(in_eResult, *pOpenData);
		}
		else if (pOpenData->pFileDesc->iFileSize <= 0 && pOpenData->eOpenMode == AK_OpenModeRead)
		{
			AKASSERT(!"Invalid file size: fileDesc.iFileSize <= 0");	// Low-level IO implementation error.
			AK::StreamMgr::MonitorFileOpenError(AK_InvalidFileSize, *pOpenData);
		}
#endif
		AkDelete(AkMemID_Streaming, pOpenData);
	}
	else
	{
		m_eOpenStatus = AK_StmOpenStatus_Idle; // Waiting for I/O thread to re-attempt another open.

		VerifyOpenResult(in_eResult);
		m_lockStatus.Unlock();
	}
}

//-----------------------------------------------------------------------------
// Name: class CAkStdStmBase
// Desc: Standard stream base implementation.
//-----------------------------------------------------------------------------

CAkStdStmBase::CAkStdStmBase()
: m_memBlock( NULL )
, m_uTotalScheduledSize( 0 )
, m_eStmStatus( AK_StmStatusIdle )
, m_uNextExpectedUserPosition(0)
, m_uNextExpectedFilePosition(0)
, m_bIsOpComplete( true )
{
	m_eStmType = AK_StmTypeStandard;
}

CAkStdStmBase::~CAkStdStmBase( )
{
	// If the stream kept asking to be scheduled, now it is time to stop.
    if ( m_bRequiresScheduling )
        m_pDevice->StdSemDecr();
}

//-----------------------------------------------------------------------------
// IAkStdStream interface.
//-----------------------------------------------------------------------------

// Stream info access.
// Sync: None.
void CAkStdStmBase::GetInfo(AkStreamInfo & out_info)
{
	CAkStmTask::GetInfo(out_info);
}

// Name the stream (appears in Wwise profiler).
AKRESULT CAkStdStmBase::SetStreamName(const AkOSChar * in_pszStreamName)
{
	return CAkStmTask::SetStreamName(in_pszStreamName);
}

// Get low-level block size for this stream.
// Returns block size for optimal/unbuffered IO.
AkUInt32 CAkStdStmBase::GetBlockSize()
{
    return m_uLLBlockSize;
}

// Get stream position.
// Sync: None. 
// Users should not call this when pending.
AkUInt64 CAkStdStmBase::GetPosition( 
    bool * out_pbEndOfStream   // Input streams only. Can be NULL.
    )   
{
	if (IsFileOpened())
	{
		AKASSERT(m_eStmStatus != AK_StmStatusPending ||
			!"Inaccurate stream position when operation is pending");
		if (out_pbEndOfStream != NULL)
		{
			*out_pbEndOfStream = m_bHasReachedEof;
		}
		return GetCurUserPosition();
	}
	return 0;
}

// Operations.
// ------------------------------------------

// Set stream position. Modifies position of next read/write.
// Sync: 
// Fails if an operation is pending.
AKRESULT CAkStdStmBase::SetPosition(
    AkInt64         in_iMoveOffset,     // Seek offset.
    AkMoveMethod    in_eMoveMethod      // Seek method, from beginning, end or current file position.
    )
{
    // Compute position.
    AkInt64 iPosition;
    if ( in_eMoveMethod == AK_MoveBegin )
    {
        iPosition = in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveCurrent )
    {
        iPosition = GetCurUserPosition() + in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveEnd )
    {
        iPosition = m_pFileDesc->iFileSize + in_iMoveOffset;
    }
    else
    {
        AKASSERT( !"Invalid move method" );
        return AK_InvalidParameter;
    }

    if ( iPosition < 0 )
    {
        AKASSERT( !"Trying to move the file pointer before the beginning of the file" );
        return AK_InvalidParameter;
    }

	AkAutoLock<CAkLock> status(m_lockStatus);

	if (!IsFileOpened())
	{
		m_uNextExpectedUserPosition = iPosition;
		m_uNextExpectedFilePosition = iPosition;
		return AK_Success;	//Will be applied when the file is opened.
	}

	// Safe status.
	if (m_eStmStatus == AK_StmStatusPending)
	{
		AKASSERT(!"Trying to change stream position while standard IO is pending");
		return AK_Fail;
	}

	m_uNextExpectedUserPosition = iPosition;
	m_uNextExpectedFilePosition = iPosition;

    // Round offset to block size.
    if (iPosition % m_uLLBlockSize != 0 )
    {
        // Snap to lower boundary.
		iPosition -= (iPosition % m_uLLBlockSize );
        AKASSERT(iPosition >= 0 );
    }

    // Update position if it changed.
    // Set new file position.
    SetCurUserPosition(iPosition);

    m_bHasReachedEof = (iPosition >= m_pFileDesc->iFileSize);
    return AK_Success;
}

AKRESULT CAkStdStmBase::Open(const AkFileOpenData& in_FileOpen, bool in_bSync)
{
	m_eStmStatus = AK_StmStatusPending;

	AKRESULT res = CAkStmTask::Open(in_FileOpen, in_bSync, false);
	if (res != AK_Success)
	{
		//If there was an error in async opening it will be reported in m_eResult. 
		//Otherwise, res already has the most up to date error.
		if (m_eStmStatus == AK_StmStatusError)
			res = m_eResult;	
	}

	return res;
}

void CAkStdStmBase::OnFileDeferredOpen(const AkAsyncFileOpenData& in_OpenData, AKRESULT in_eResult)
{
	if (in_eResult == AK_Success)
	{
		m_eStmStatus = AK_StmStatusIdle; // SetPosition wants this.
		in_eResult = SetPosition(m_uNextExpectedUserPosition, AkMoveMethod::AK_MoveBegin);
	}

	SetStatus(in_eResult == AK_Success ? AK_StmStatusIdle : AK_StmStatusError, in_eResult);

	m_pDevice->SignalIOCompleted(this);
}


// Read.
// Sync: Returns if task pending. Status change.
AKRESULT CAkStdStmBase::Read(
    void *          in_pBuffer,         // User buffer address. 
    AkUInt32        in_uReqSize,        // Requested read size.
    bool            in_bWait,           // Block until operation is complete.
    AkPriority      in_priority,        // Heuristic: operation priority.
    AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
    AkUInt32 &      out_uSize           // Size actually read.
    )
{
    return ExecuteOp( OpRead,// (Read)
		in_pBuffer,         // User buffer address. 
		in_uReqSize,        // Requested write size. 
		in_bWait,           // Block until operation is complete.
		in_priority,        // Heuristic: operation priority.
		in_fDeadline,       // Heuristic: operation deadline (s).
		out_uSize );        // Size actually written.
}

// Write.
// Sync: Returns if task pending. Changes status.
AKRESULT CAkStdStmBase::Write(
    void *          in_pBuffer,         // User buffer address. 
    AkUInt32        in_uReqSize,        // Requested write size. 
    bool            in_bWait,           // Block until operation is complete.
    AkPriority      in_priority,        // Heuristic: operation priority.
    AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
    AkUInt32 &      out_uSize           // Size actually written.
    )
{
    return ExecuteOp( OpWrite,	// (Write)
		in_pBuffer,         // User buffer address. 
		in_uReqSize,        // Requested write size. 
		in_bWait,           // Block until operation is complete.
		in_priority,        // Heuristic: operation priority.
		in_fDeadline,       // Heuristic: operation deadline (s).
		out_uSize );        // Size actually written.
}

// Execute Operation (either Read or Write).
AKRESULT CAkStdStmBase::ExecuteOp(
	OpType			in_op,				// Open, Read or Write.
	void *          in_pBuffer,         // User buffer address. 
    AkUInt32        in_uReqSize,        // Requested write size. 
    bool            in_bWait,           // Block until operation is complete.
    AkPriority      in_priority,        // Heuristic: operation priority.
    AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
    AkUInt32 &      out_uSize           // Size actually written.
	)
{
	AKASSERT(in_op != OpOpen);

	// Check requested size.
	if (in_pBuffer == NULL)
	{
		char err_msg[100];
		AKPLATFORM::SafeStrFormat(err_msg, sizeof(err_msg), "%s, invalid buffer", __func__);
		AK::Monitor::PostString(err_msg, AK::Monitor::ErrorLevel_Error);
		return AK_InvalidParameter;
	}

	// Check heuristics.
	if (in_priority < AK_MIN_PRIORITY ||
		in_priority > AK_MAX_PRIORITY ||
		in_fDeadline < 0)
	{
		char err_msg[100];
		AKPLATFORM::SafeStrFormat(err_msg, sizeof(err_msg), "%s, Invalid heuristics", __func__);
		AK::Monitor::PostString(err_msg, AK::Monitor::ErrorLevel_Error);
		return AK_InvalidParameter;
	}

	// Check status.
	if (m_eStmStatus == AK_StmStatusPending)
	{
		if (in_bWait)
			WaitForPendingOperation();
		else
			return AK_TooManyConcurrentOperations;
	}

	if (m_eStmStatus == AK_StmStatusError)
		return m_eResult;

    out_uSize					= 0;
	m_uTotalScheduledSize		= 0;
	m_bIsOpComplete				= false;
	m_eOp						= in_op;
    m_priority					= in_priority;
    m_fDeadline					= in_fDeadline;
	// Make our mem block represent the user provided buffer. The file position stored in m_memBlock is the current
	// user position, that is, the position where this operation should start.
	m_memBlock.pData			= in_pBuffer;
	m_memBlock.uAvailableSize	= in_uReqSize;

    // Verify with block size.
	if (in_op == OpRead)
	{
		if ( in_uReqSize % m_uLLBlockSize != 0 )
		{
			AKASSERT( !"Requested size incompatible with Low-Level block size" );
			return AK_InvalidParameter;
		}

		bool bEof;	// unused
		in_uReqSize = ClampRequestSizeToEof( GetCurUserPosition(), in_uReqSize, bEof );
	}

	// Leave right away if requested size turns out to be 0 (only for Read).
	if ( in_op == OpRead && (0 == in_uReqSize) && in_pBuffer )
	{
		AkAutoLock<CAkLock> status( m_lockStatus );
		SetStatus( AK_StmStatusCompleted, AK_Success );
		out_uSize = 0;
		return AK_Success;
	}

    // Reset time.
	AKPLATFORM::PerformanceCounter( &m_iIOStartTime );
    
    // If blocking, register this thread as blocked.
	AKRESULT eResult;
    if ( in_bWait )
    {
		{
			AkAutoLock<CAkLock> stmGate(m_lockStatus);
			SetBlockedStatus();

			// Set Status. Notify device sheduler.
			SetStatus(AK_StmStatusPending, AK_Success);
		}
		
	    // Wait for the blocking event.
		m_pDevice->WaitForIOCompletion( this );

		eResult = ( AK_StmStatusCompleted == m_eStmStatus ) ? AK_Success : m_eResult;
    }
    else
    {
    	// Set Status. Notify device sheduler.
    	AkAutoLock<CAkLock> status( m_lockStatus );

		SetStatus( AK_StmStatusPending, AK_Success);
		eResult = AK_Success;
    }
    	
    out_uSize = in_uReqSize;

    return eResult;
}

// Get data and size.
// Returns address of data. No check for pending I/O.
// Sync: None. Always accurate when I/O not pending.
void* CAkStdStmBase::GetData(
	AkUInt32& out_uActualSize   // Size actually read or written.
)
{
	// Note: m_memBlock.uAvailableSize maps the user provided memory, but was clamped to the end of file when 
	// the operation completed.
	out_uActualSize = m_memBlock.uAvailableSize;
	return m_memBlock.pData;
}

// Info access.
// Sync: None. Status query.
AkStmStatus CAkStdStmBase::GetStatus()
{
    return m_eStmStatus;
}

/// Block and wait for a pending read to finish, and return the current status of the stream.
/// This will return immediately if the status is not pending.
AkStmStatus CAkStdStmBase::WaitForPendingOperation()
{
	AkAutoLock<CAkLock> updateStatus(m_lockStatus);	
	if (GetStatus() == AK_StmStatusPending)
	{
		SetBlockedStatus();
		m_lockStatus.Unlock();
		m_pDevice->WaitForIOCompletion( this );
		m_lockStatus.Lock();
	}

	return GetStatus();
}

//-----------------------------------------------------------------------------
// CAkStmTask virtual methods implementation.
//-----------------------------------------------------------------------------

// Compute task's deadline for next operation.
// Sync: None: if m_uActualSize is changed in the meantime, the scheduler could take a suboptimal decision.
AkReal32 CAkStdStmBase::EffectiveDeadline()
{
	AKASSERT(m_pDevice != nullptr);
	AkUInt32 uGranularity = m_pDevice->GetGranularity();
	AkUInt32 uNumTransfersRemaining = ( m_memBlock.uAvailableSize - m_uTotalScheduledSize + uGranularity - 1 ) / uGranularity;
	if (uNumTransfersRemaining > 0)
	{
		AKASSERT(uNumTransfersRemaining > 0);
		AkReal32 fDeadline = (m_fDeadline / uNumTransfersRemaining) - AKPLATFORM::Elapsed(m_pDevice->GetTime(), m_iIOStartTime);
		return (fDeadline > 0 ? fDeadline : 0);
	}
	else
	{
		return 0;
	}
}

// Add a new streaming memory view (or content of a memory view) to this stream after a transfer.
// If it ends up not being used, it is disposed of. Otherwise it's status is set to Ready.
// All logical transfers must end up here, even if they were cancelled.
// Sync: Status must be locked prior to calling this function. 
void CAkStdStmBase::AddMemView( 
	CAkStmMemView * in_pMemView,		// Transfer-mode memory view to resolve.
	bool			in_bStoreData		// Store data in stream object only if true.
	)
{
	AKASSERT( in_pMemView );
	
	AkUInt32 uTransferSize = 0;

	if ( in_bStoreData
		&& !m_bIsToBeDestroyed 
		&& m_eStmStatus != AK_StmStatusError )
    {
		m_bHasReachedEof = false;

		// Deduce transfer size.
		m_bIsOpComplete = true;
		uTransferSize = in_pMemView->Size();
		if ( uTransferSize > m_pDevice->GetGranularity() )
		{
			uTransferSize = m_pDevice->GetGranularity();
			m_bIsOpComplete = false;
		}

		// Check EOF. in_pMemView->EndPosition() may overshoot file end position because it maps the 
		// user-provided memory.
		if ( m_eOp == OpRead
			&& uTransferSize >= ( FileSize() - in_pMemView->StartPosition() ) )
		{
			uTransferSize = (AkUInt32)( FileSize() - in_pMemView->StartPosition() );
			m_bHasReachedEof = true;
			
			// Operation is complete, and we reached the end of file in read mode. Adjust available size for user.
			m_bIsOpComplete = true;
			AKASSERT( in_pMemView->EndPosition() >= FileSize() );
			AkUInt32 uFileOvershoot = (AkUInt32)( in_pMemView->EndPosition() - FileSize() );
			m_memBlock.uAvailableSize -= uFileOvershoot;
		}
		
		AKASSERT( uTransferSize <= m_memBlock.uAvailableSize
				&& ( m_bIsOpComplete || !m_bHasReachedEof ) );
	}
	// else 
	// Stream was either scheduled to be destroyed while I/O was occurring, stopped, or 
	// its position was set dirty while I/O was occuring. 

	// Once transfers are completed, standard streams don't keep data refs. Dispose of it now.
	{
		AkAutoLock<CAkIOThread> lock( *m_pDevice );

// Profiling.
#ifndef AK_OPTIMIZED
		m_uBytesTransfered += uTransferSize;

		AKASSERT( in_pMemView->Status() != CAkStmMemView::TransferStatus_Ready 
				|| !"Standard streams cannot use cache data" );
		if ( m_pDevice->IsMonitoring() )
			m_pDevice->PushTransferStatistics( uTransferSize, true );
#endif

		m_pDevice->DestroyMemView( &m_memBlock, in_pMemView );
	}
}

// Update task's status after transfer.
// Sync: Status must be locked prior to calling this function.
void CAkStdStmBase::UpdateTaskStatus(
	AKRESULT	in_eIOResult			// AK_Success if IO was successful, AK_Cancelled if IO was cancelled, AK_Fail otherwise.
	)
{
	AKASSERT(m_eOp != OpOpen || CanBeDestroyed()); // Don't call this to update task status after file open, only after a file transfer

    // Compute status.
	if (in_eIOResult == AK_Success)
	{
		if (m_bIsOpComplete)
		{
			// Read/Write op
			// Update client position.
			// Note: m_memBlock.uAvailableSize maps the user provided memory, but was clamped to the end of file when 
			// the operation completed.
			SetCurUserPosition(GetCurUserPosition() + m_memBlock.uAvailableSize);

			// Change Status and update semaphore count.
			SetStatus(AK_StmStatusCompleted, AK_Success);
		}
		// else Still pending: do not change status.
	}
	else 
    {
		// Change Status and update semaphore count.
		SetStatus(in_eIOResult == AK_Cancelled ? AK_StmStatusCancelled : AK_StmStatusError, in_eIOResult );
    }
   
    // Release the client thread if blocking I/O.
    if ( m_eStmStatus != AK_StmStatusPending 
		&& m_eStmStatus != AK_StmStatusIdle )
    {
		m_pDevice->SignalIOCompleted( this );
    }
}

void CAkStdStmBase::Kill()
{
	AkAutoLock<CAkLock> updateStatus( m_lockStatus );
	UpdateTaskStatus( AK_Fail );
}

//-----------------------------------------------------------------------------
// Profiling.
//-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
// Get stream data.
void CAkStdStmBase::GetStreamData(
    AkStreamData & out_streamData
    )
{
	AkAutoLock<CAkLock> stmBufferGate(m_lockStatus);

    out_streamData.uStreamID = m_uStreamID;
    // Note. Priority appearing in profiler will be that which was used in last operation. 
    out_streamData.uPriority = m_priority;
    out_streamData.uTargetBufferingSize = 0;
    out_streamData.uVirtualBufferingSize = 0;
    out_streamData.uFilePosition = GetCurUserPosition();
    out_streamData.uNumBytesTransfered = m_uBytesTransfered;
	out_streamData.uNumBytesTransferedLowLevel = m_uBytesTransfered;
    m_uBytesTransfered = 0;    // Reset.
	out_streamData.uMemoryReferenced = 0;
	out_streamData.uBufferedSize = 0;
	out_streamData.fEstimatedThroughput = ( m_fDeadline > 0 ) ? m_memBlock.uAvailableSize / m_fDeadline : 0.f;
	out_streamData.bActive = m_bWasActive;
	if ( m_bCanClearActiveProfile )
	{
		m_bWasActive = false;
	}
}

// Signals that stream can be destroyed.
void CAkStdStmBase::ProfileAllowDestruction()
{
	AkAutoLock<CAkLock> statusChange(m_lockStatus);
    AKASSERT( m_bIsToBeDestroyed );
    m_bIsProfileDestructionAllowed = true;	
	SetStatus( AK_StmStatusCancelled, AK_Success );
 }
#endif

//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------
// Set task's status.
// Sync: Status must be locked.
void CAkStdStmBase::SetStatus( AkStmStatus in_eStatus, AKRESULT in_eResult )
{	
	m_eStmStatus = in_eStatus;
	m_eResult = in_eResult;

	// Update semaphore.
	if ( IsToBeDestroyed() && CanBeDestroyed() )
	{
		// Requires clean up.
		if ( !m_bRequiresScheduling )
        {
            // Signal IO thread for clean up.
			m_bRequiresScheduling = true;
#ifndef AK_OPTIMIZED
			m_bWasActive = true;
			m_bCanClearActiveProfile = false;
#endif
            m_pDevice->StdSemIncr();
        }
	}
    else
    {
		if ( AK_StmStatusPending == in_eStatus )
		{
			// Requires IO transfer.
			AKASSERT( !IsToBeDestroyed() || !"Cannot call SetStatus(Pending) after stream was scheduled for termination" );
			SetReadyForIO( true );
			if ( !m_bRequiresScheduling )
			{
				m_bRequiresScheduling = true;
#ifndef AK_OPTIMIZED
				m_bWasActive = true;
				m_bCanClearActiveProfile = false;
#endif
				m_pDevice->StdSemIncr();
			}
		}
		else
		{
			// Does not require IO transfer.
			SetReadyForIO( false );
			if ( m_bRequiresScheduling )
			{
				m_bRequiresScheduling = false;
				m_pDevice->StdSemDecr();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Name: class CAkAutoStmBase
// Desc: Automatic stream base implementation.
//-----------------------------------------------------------------------------

CAkAutoStmBase::CAkAutoStmBase()
: m_cacheID( AK_INVALID_CACHE_ID )
, m_uMinimalBufferingSize( 0 )
, m_uVirtualBufferingSize( 0 )
, m_uBufferAlignment( 1 )
#ifndef AK_OPTIMIZED
, m_uBytesTransferedLowLevel( 0 )
#endif
, m_uNextToGrant( 0 )
, m_bIsRunning( false )
, m_bIOError( false )
, m_bCachingReady( false )
{
	m_eStmType = AK_StmTypeAutomatic;
}

CAkAutoStmBase::~CAkAutoStmBase( )
{
	// If the stream kept asking to be scheduled, now it is time to stop.
    if ( m_bRequiresScheduling )
        m_pDevice->AutoSemDecr();
}

// Init.
void CAkAutoStmBase::Init( 
    const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
    AkAutoStmBufSettings *      in_pBufferSettings // Stream buffer settings.
    )
{
    AKASSERT( in_heuristics.priority >= AK_MIN_PRIORITY &&
              in_heuristics.priority <= AK_MAX_PRIORITY );

	SetThroughput(AkMax(in_heuristics.fThroughput, AK_MINIMAL_THROUGHPUT));

	m_uLoopStart = in_heuristics.uLoopStart;
	m_uLoopEnd = in_heuristics.uLoopEnd;

	// Minimum number of buffers concurrently owned by client is 1 when not specified.
	m_uMinNumBuffers = (in_heuristics.uMinNumBuffers > 0) ? in_heuristics.uMinNumBuffers : 1;

	SetPriority(in_heuristics.priority);

	m_uNextExpectedUserPosition = 0;
	m_uNextExpectedFilePosition = 0;

	//Buffering settings will be adjusted once granularity is known.
	if (in_pBufferSettings)
		m_bufSettings = *in_pBufferSettings;
	else
	{
		m_bufSettings.uBlockSize = 0;
		m_bufSettings.uMinBufferSize = 0;
		m_bufSettings.uBufferSize = 0;
	}
}

AKRESULT CAkAutoStmBase::Open(const AkFileOpenData& in_FileOpen, bool in_bSync)
{
	if (in_FileOpen.pFlags)
		m_cacheID = in_FileOpen.pFlags->uCacheID;
	return CAkStmTask::Open(in_FileOpen, in_bSync, true);
}

static inline AkUInt32 Gcd( AkUInt32 a, AkUInt32 b )
{
	AKASSERT( a > 0 && b > 0 );
	if ( a < b ) 
	{
		AkUInt32 tmp = a;
		a = b;
		b = tmp;
	}
	
	AkUInt32 r = a % b;
	a = b;
	b = r;
	if ( b==0 ) return a;

	AkUInt32 k = 0;
	while ( !((a|b)&1) ) // both even
	{
		k++;
		a >>= 1;
		b >>= 1;
	}
	while ( !(a&1) ) a >>= 1;
	while ( !(b&1) ) b >>= 1;
	do
	{
		if ( a==b ) return a << k;
		if ( a < b )
		{
			AkUInt32 tmp = a;
			a = b;
			b = tmp;
		}
		AkUInt32 t = (a-b) >> 1; // t>0
		while ( !(t&1) ) t >>= 1;
		a = t;
	}
	while ( 1 );
}

// Helper: Set stream's buffering constraints.
// Sync: Status.
AKRESULT CAkAutoStmBase::AdjustBufferingSettings()
{
	AkAutoStmBufSettings bufClientSettings = m_bufSettings;

	AkUInt32 uGranularity = m_pDevice->GetGranularity();

	// Set up buffer size according to streaming memory constraints.
	
	// Fix buffer alignment if specified. It is the smallest common multiple between user and low-level block sizes.
	if ( m_bufSettings.uBlockSize > 0 )
		m_uBufferAlignment = m_uLLBlockSize * (m_bufSettings.uBlockSize / Gcd( m_uLLBlockSize, m_bufSettings.uBlockSize ));

	// Default values: Align buffer with block size, 
	// min buffer size is equal to block size.
	m_bufSettings.uBufferSize = uGranularity - (uGranularity % m_uBufferAlignment);
	m_bufSettings.uMinBufferSize = m_uBufferAlignment;	
		
	// Buffer size constraints.
    if (bufClientSettings.uBufferSize != 0)
    {
		// User constrained buffer size. Ensure that it is valid with device granularity and low-level block size.
        if (bufClientSettings.uBufferSize > uGranularity
			|| (bufClientSettings.uBufferSize % m_uBufferAlignment ) > 0)
        {
			AK_MONITOR_ERROR( AK::Monitor::ErrorCode_IncompatibleIOSettings );
            return AK_Fail;
        }
        m_bufSettings.uBufferSize = bufClientSettings.uBufferSize;
		m_bufSettings.uMinBufferSize = bufClientSettings.uBufferSize;	// Hard constraint: min buffer size is equal to specified buffer size.
    }
    else
    {
		// Minimum buffer size specified?
		if (bufClientSettings.uMinBufferSize > 0)
		{
			// Min buffer size is equal to specified min buffer size if specified, low-level block size otherwise.
			if (m_bufSettings.uMinBufferSize < bufClientSettings.uMinBufferSize )
			{
				m_bufSettings.uMinBufferSize = bufClientSettings.uMinBufferSize;
				// Snap to required alignment, and then check against buffer size.
				m_bufSettings.uMinBufferSize = ( (m_bufSettings.uMinBufferSize + m_uBufferAlignment - 1) / m_uBufferAlignment ) * m_uBufferAlignment;
				if (m_bufSettings.uMinBufferSize > m_bufSettings.uBufferSize)
				{
					AK_MONITOR_ERROR( AK::Monitor::ErrorCode_IncompatibleIOSettings );
					return AK_Fail;
				}
			}
		}
    }

	return AK_Success;
}

//-----------------------------------------------------------------------------
// IAkAutoStream interface.
//-----------------------------------------------------------------------------

// Destruction.
// Close stream. The object is destroyed and the interface becomes invalid.
// Buffers are flushed here.
// Sync: 
// 1. Locks scheduler to set its m_bIsToBeDestroued flag. Better lock the
// scheduler once in a while than having the scheduler lock all the streams
// every time it schedules a task. Also, it spends most of its time executing I/O,
// so risks of interlock are minimized.
// 2. Lock status.
void CAkAutoStmBase::Destroy()
{
	bool bDestroyNow = false;
	{
		AkAutoLock<CAkLock> stmBufferGate(m_lockStatus);

		SetToBeDestroyed();

		if (!m_pDevice)
		{
			// Detached device, we can destroy ourselves
			bDestroyNow = true; // We cannot destroy ourselves now while m_lockStatus is locked
		}
		else
		{
			CHECK_BUFFERING_CONSISTENCY();

			// Client can have buffers still granted to him. Clear them before flushing.
			// More efficient just to clear m_uNextToGrant while adjusting the virtual buffering value, then flush all.
			AkBufferList::Iterator it = m_listBuffers.Begin();
			while (m_uNextToGrant > 0)
			{
				AKASSERT(it != m_listBuffers.End());
				m_uVirtualBufferingSize += GetEffectiveViewSize((*it));
				--m_uNextToGrant;
				++it;
			}

			Flush();	// Scheduling status is updated in Flush().

			m_listBuffers.Term();
		}
	} // Unlock m_lockStatus

	if (bDestroyNow)
		InstantDestroy();
}

void CAkAutoStmBase::GetInfo(AkStreamInfo& out_info)
{
	CAkStmTask::GetInfo(out_info);
}

// Stream heuristics access.
// Sync: None.
void CAkAutoStmBase::GetHeuristics(
    AkAutoStmHeuristics & out_heuristics    // Returned stream heuristics.
    )
{
    out_heuristics.fThroughput  = GetThroughput();
    out_heuristics.uLoopStart   = m_uLoopStart;
    out_heuristics.uLoopEnd     = m_uLoopEnd;
    out_heuristics.uMinNumBuffers   = m_uMinNumBuffers;
    out_heuristics.priority     = m_priority;
}

// Stream heuristics run-time change.
// Sync: None.
AKRESULT CAkAutoStmBase::SetHeuristics(
    const AkAutoStmHeuristics & in_heuristics   // New stream heuristics.
    )
{
	if (SetPriority(in_heuristics.priority) != AK_Success)
    {
        return AK_InvalidParameter;
    }

	AkAutoLock<CAkLock> stmBufferGate(m_lockStatus);

	//
	// Update heuristics that have an effect on scheduling.
    //
	AkReal32 fNewThroughput = AkMax( in_heuristics.fThroughput, AK_MINIMAL_THROUGHPUT );

	// Looping.
	//
	AkUInt64 uLoopEnd;
	if ( !IsFileOpened() || (AkInt64)in_heuristics.uLoopEnd <= m_pFileDesc->iFileSize)
		uLoopEnd = in_heuristics.uLoopEnd;
	else
		uLoopEnd = m_pFileDesc->iFileSize;

    if ( m_uLoopEnd != uLoopEnd ||
         m_uLoopStart != in_heuristics.uLoopStart )
    {
        // Update other heuristics.
		SetThroughput(fNewThroughput);
		// Note: Minimum number of buffers concurrently owned by client is 1 when not specified.
		m_uMinNumBuffers = ( in_heuristics.uMinNumBuffers > 0 ) ? in_heuristics.uMinNumBuffers : 1;

		// Snap loop start to block size and store new heuristics.
		AkUInt64 uLoopStart = in_heuristics.uLoopStart - ( in_heuristics.uLoopStart % m_uLLBlockSize );
	    m_uLoopStart = uLoopStart;
	    SetLoopEnd( uLoopEnd );	// Loop end changed: recompute virtual buffering from scratch.

		if (IsFileOpened())
		{

			// Inspect all the current memory views, ready and pending. Flush anything that is incompatible
			// with the new looping heuristics.

			// 1) Get first buffer not granted.
			AkUInt64 uNextExpectedPosition = m_uNextExpectedFilePosition;
			AkBufferList::IteratorEx it = m_listBuffers.BeginEx();
			AkUInt32 uLastBufferIdxToKeep = 0;
			AKASSERT(m_listBuffers.Length() >= m_uNextToGrant);
			while (uLastBufferIdxToKeep < m_uNextToGrant) // skip buffers that are already granted
			{
				uNextExpectedPosition = (*it)->EndPosition();
				++it;
				++uLastBufferIdxToKeep;
			}
			if (uLoopEnd > 0 && uNextExpectedPosition >= uLoopEnd)
				uNextExpectedPosition = uLoopStart;

			// 2) Iterate through list of buffers already streamed in, and dequeue any of them that is not 
			// consistent with the next expected position. Store them temporarily in a separate queue in order
			// to release them all at once.
			AkStmMemViewListLight listToRemove;
			while (it != m_listBuffers.End())
			{
				CAkStmMemView* pMemView = (*it);

				if (pMemView->StartPosition() != uNextExpectedPosition)
				{
					// Dequeue and add to listToRemove.				
					it = m_listBuffers.Erase(it);
					listToRemove.AddFirst(pMemView);
				}
				else
				{
					// Valid. Keep it and update uNextExpectedPosition.
					uNextExpectedPosition = pMemView->EndPosition();
					if (uLoopEnd > 0 && uNextExpectedPosition >= uLoopEnd)
						uNextExpectedPosition = uLoopStart;
					++it;
				}
			}

			// 3) Release all dequed buffers (must be inside device lock: memory change).
			if (!listToRemove.IsEmpty())
			{
				AkAutoLock<CAkIOThread> lock(*m_pDevice);

				AkStmMemViewListLight::IteratorEx itRemove = listToRemove.BeginEx();
				while (itRemove != listToRemove.End())
				{
					CAkStmMemView* pMemView = (*itRemove);
					itRemove = listToRemove.Erase(itRemove);
					DestroyBuffer(pMemView);
				}
			}

			listToRemove.Term();

			// 4) Do the same with pending transfers.
			CancelInconsistentPendingTransfers(uNextExpectedPosition);

			UpdateSchedulingStatus();
		}
    }
	else	// looping changed
	{
		// Update other heuristics that have an effect on scheduling only if their value changed.

		// Note: Minimum number of buffers concurrently owned by client is 1 when not specified.
		AkUInt8 uNewMinNumBuffers = ( in_heuristics.uMinNumBuffers > 0 ) ? in_heuristics.uMinNumBuffers : 1;

		if ( GetThroughput() != fNewThroughput
			|| m_uMinNumBuffers != uNewMinNumBuffers )
		{
			m_uMinNumBuffers = uNewMinNumBuffers;
			SetThroughput(fNewThroughput);

			if (IsFileOpened())
			{
				UpdateSchedulingStatus();
			}
		}		
	}
    return AK_Success;
}

// Run-time change of the stream's minimum buffer size that can be handed out to client.
// Sync: Status.
AKRESULT CAkAutoStmBase::SetMinimalBufferSize(
	AkUInt32 in_uMinBufferSize	// Minimum buffer size that can be handed out to client.
	)
{
	AkUInt32 uOldMinBufferSize = m_bufSettings.uMinBufferSize;

	// Lock status.
	AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );
	m_bufSettings.uBufferSize = 0; // Unconstrained.
	m_bufSettings.uMinBufferSize = in_uMinBufferSize;
	m_bufSettings.uBlockSize = 0; // Unconstrained.
	
	AKRESULT eResult = AdjustBufferingSettings();
	if ( eResult == AK_Success )
	{
		if (m_bufSettings.uMinBufferSize > uOldMinBufferSize && IsFileOpened())
		{
			// Now that buffering constraints have been changed, pass through buffer list and flush everything
			// after an invalid buffer size.
			FlushSmallBuffersAndPendingTransfers( in_uMinBufferSize );
			UpdateSchedulingStatus();
		}
	}
	else
		UpdateTaskStatus( AK_Fail );

	return eResult;
}

// Name the stream (appears in Wwise profiler).
AKRESULT CAkAutoStmBase::SetStreamName(const AkOSChar* in_pszStreamName)
{
	return CAkStmTask::SetStreamName(in_pszStreamName);
}

// Returns low-level IO block size for this stream's file descriptor.
AkUInt32 CAkAutoStmBase::GetBlockSize()
{
    return m_uLLBlockSize;
}

// Operations.
// ---------------------------------------

// Starts automatic scheduling.
// Sync: Status update if not already running. 
// Notifies memory change.
AKRESULT CAkAutoStmBase::Start()
{
	if (!m_bIOError && !m_bIsRunning)
	{
		{
			AkAutoLock<CAkLock> status(m_lockStatus);
			if (!IsFileOpened())
			{
				m_bIsRunning = true;	//Keep the state for when it is opened.
				return AK_FormatNotReady;
			}

			// UpdateSchedulingStatus() will notify scheduler if required.
			SetRunning( true );
			UpdateSchedulingStatus();
			m_bCachingReady = true;
			
			// Reset time. Time count since last transfer starts now.
			m_iIOStartTime = m_pDevice->GetTime();
		}

        // The scheduler should reevaluate memory usage. Notify it.
		{
			AkAutoLock<CAkIOThread> lock( *m_pDevice );
	        m_pDevice->NotifyMemChange();
		}
	}

    return m_bIOError ? AK_Fail : AK_Success;
}

// Stops automatic scheduling.
// Sync: Status update.
AKRESULT CAkAutoStmBase::Stop()
{
	// Lock status.
    AkAutoLock<CAkLock> status( m_lockStatus );

	if (!IsFileOpened())
	{
		m_bIsRunning = false;	//Keep the state for when it is opened.
		return AK_Success;
	}

    SetRunning( false );
	Flush();
    
    return AK_Success;
}

// Get stream position; position as seen by the user.
// In other word, gets the position of the first byte returned by the next GetBuffer call.
AkUInt64 CAkAutoStmBase::GetPosition( 
    bool * out_pbEndOfStream    // Can be NULL.
    )
{
	AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );

	AKASSERT(m_listBuffers.IsEmpty() || m_listBuffers.First()->StartPosition() == m_uNextExpectedFilePosition);
	
    if ( out_pbEndOfStream != NULL )
        *out_pbEndOfStream = (m_uNextExpectedUserPosition >= (AkUInt64)m_pFileDesc->iFileSize );

    return m_uNextExpectedUserPosition;
}

// Set stream position. Modifies position of next read/write.
// Sync: Updates status. 
AKRESULT CAkAutoStmBase::SetPosition(
    AkInt64         in_iMoveOffset,     // Seek offset.
    AkMoveMethod    in_eMoveMethod     // Seek method, from beginning, end or current file position.
    )
{
    // Compute position.
    AkInt64 iPosition;
    if ( in_eMoveMethod == AK_MoveBegin )
    {
        iPosition = in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveCurrent )
    {
		iPosition = GetPosition( NULL ) + in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveEnd )
    {
        iPosition = m_pFileDesc->iFileSize + in_iMoveOffset;
    }
    else
    {
        AKASSERT( !"Invalid move method" );
        return AK_InvalidParameter;
    }

    if ( iPosition < 0 )
    {
        AKASSERT( !"Trying to move the file pointer before the beginning of the file" );
        return AK_InvalidParameter;
    }

	// Lock status.
	AkAutoLock<CAkLock> statusGate(m_lockStatus);

	m_uNextExpectedUserPosition = iPosition;
	m_uNextExpectedFilePosition = iPosition;

	if (!IsFileOpened())
		return m_bIOError ? AK_Fail : AK_Success;

    // Change offset if Low-Level block size is greater than 1.
    if (m_uNextExpectedFilePosition % m_uLLBlockSize != 0 )
    {
        // Round down to block size.
		m_uNextExpectedFilePosition -= (m_uNextExpectedFilePosition % m_uLLBlockSize );
    }
		
	// Check whether next buffer position corresponds to desired position (if SetPosition() was consistent
	// with looping heuristic, it will be). If it isn't, then we need to flush everything we have.

	if (m_uNextToGrant < m_listBuffers.Length())
	{
		if (GetNextBufferToGrant()->StartPosition() != m_uNextExpectedFilePosition)
		{
			// Flush everything we have, that was not already granted to user.
			// Note: Flush() also flushes pending transfers.
			Flush();
			AKASSERT(m_listBuffers.Length() == m_uNextToGrant);
		}
		else
			UpdateSchedulingStatus();
	}
	else
	{
		// Nothing buffered. Yet, there might be pending transfers that are inconsistent with in_uNewPosition.
		// Cancel all pending transfers if applicable.
		CancelInconsistentPendingTransfers(m_uNextExpectedFilePosition);
		UpdateSchedulingStatus();
	}

    return AK_Success;
}

// Cache query:
// Returns true if data for the next expected transfer was found either in the list or in cache.
// Returns false if data is found neither in the list or cache.
// If data is found in the list or cache, out_pBuffer is set and ready to use. 
// Sync: Stream needs to be locked.
bool CAkAutoStmBase::GetBufferOrReserveCacheBlock( void *& out_pBuffer, AkUInt32 & out_uSize )
{
	// Check in buffer list first.
	out_pBuffer = GetReadBuffer( out_uSize );

	if ( out_pBuffer )
		return true;

	// Not in buffer; Try getting from cache.
	if ( m_pDevice->ExecuteCachedTransfer( this ) )
		out_pBuffer = GetReadBuffer( out_uSize );

	return ( out_pBuffer != NULL );
}

// Data/status access. 
// -----------------------------------------

// GetBuffer.
// Return values : 
// AK_DataReady     : if buffer is granted.
// AK_NoDataReady   : if buffer is not granted yet.
// AK_NoMoreData    : if buffer is granted but reached end of file (next will return with size 0).
// AK_Fail          : there was an IO error. 

// Sync: Updates status.
AKRESULT CAkAutoStmBase::GetBuffer(
    void *&         out_pBuffer,        // Address of granted data space.
    AkUInt32 &      out_uSize,          // Size of granted data space.
    bool            in_bWait            // Block until data is ready.
    )
{
    out_pBuffer    = NULL;
    out_uSize       = 0;

#ifdef _DEBUG
	//Debug tool to simulate starvation in GameSimulator.
	if (g_bSimulateStreamStarvation && g_DebugRNG.RandomInt() % 10 == 0)	//Simulate 1 read out of 10 having a starvation	
		return AK_NoDataReady;	
#endif
	{
		AkAutoLock<CAkLock> stmBufferGate(m_lockStatus);
		if (m_bIOError)
		{
			return AK_Fail;
		}

		if (!m_bIsRunning && in_bWait)
		{
			// Trying to block on GetBuffer() on a stopped stream. This is a user error. Inform the caller.
			return AK_InvalidState;
		}
	
		// Get data from list of buffers already streamed in.
		if (!IsFileOpened())
		{
			if (in_bWait)
			{
				SetBlockedStatus();
				m_lockStatus.Unlock();
				m_pDevice->WaitForIOCompletion(this);
				m_lockStatus.Lock();

				if (!IsFileOpened())
					return AK_Fail;
			}
			else
				return AK_NoDataReady;
		}

	    out_pBuffer = GetReadBuffer( out_uSize );
	
		// Data ready?
		if ( !out_pBuffer
			&& !m_bIOError )
		{
			// Handle blocking GetBuffer. No data is ready, but there is more data to come
			// (otherwise out_pBuffer would not be NULL).
			
			// Retry getting buffer. 
			if ( GetBufferOrReserveCacheBlock( out_pBuffer, out_uSize ) 
				|| in_bWait )	// IMPORTANT: in_bWait needs to be 2nd because GetBufferOrReserveCacheBlock() HAS TO BE called.
			{
				while ( !out_pBuffer 
						&& !m_bIOError
						&& !NeedsNoMoreTransfer( 0 ) )	// Pass 0 "Actual buffering size": We know we don't have any.
				{
					// Wait for I/O to complete if there is no error.
					// Set as "blocked, waiting for I/O".
					SetBlockedStatus();

					// Release lock and let the scheduler perform IO.
					m_lockStatus.Unlock();
					m_pDevice->WaitForIOCompletion( this );
					m_lockStatus.Lock();
					
					// Try get buffer again. If it returns nothing, then wait for I/O to complete.
					out_pBuffer = GetReadBuffer( out_uSize );
				}
			}
		}
	}
	    
    AKRESULT eRetCode;
    if ( m_bIOError )
    {
        eRetCode = AK_Fail;
    }
    else if ( out_pBuffer == NULL )
    {
        // Buffer is empty, either because no data is ready, or because scheduling has completed 
        // and there is no more data.
		if ( m_bHasReachedEof 
			&& m_uNextExpectedUserPosition >= FileSize() )
            eRetCode = AK_NoMoreData;
        else
		{
			AKASSERT( !in_bWait || !"Blocking GetBuffer() cannot return AK_NoDataReady" );
            eRetCode = AK_NoDataReady;
		}
    }
    else
    {
		// Return AK_NoMoreData if buffering reached EOF and this is going to be the last buffer granted.
		if ( m_bHasReachedEof 
			&& m_uNextExpectedUserPosition >= FileSize() )
            eRetCode = AK_NoMoreData;
        else
            eRetCode = AK_DataReady;
    }
    return eRetCode;
}

// Release buffer granted to user. Returns AK_Fail if there are no buffer granted to client.
// Sync: Status lock.
AKRESULT CAkAutoStmBase::ReleaseBuffer()
{
    // Lock status.
    AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );

    // Release first buffer granted to client. 
	if ( m_uNextToGrant > 0 )
    {
		CAkStmMemView * pFirst = m_listBuffers.First();
		AKASSERT( pFirst );

		// Note: I/O pool access must be enclosed in scheduler lock.
		{
			AkAutoLock<CAkIOThread> lock( *m_pDevice );
			
			AKVERIFY( m_listBuffers.RemoveFirst() == AK_Success );
			m_pDevice->DestroyMemView( pFirst );
		}
        
        // Update "next to grant" index.
        m_uNextToGrant--;

		UpdateSchedulingStatus();

		return AK_Success;
    }

	// Failure: Buffer was not found or not granted.
	return AK_Fail;
}

//Return true if more data needs to be requested
AkUInt32 CAkAutoStmBase::GetVirtualBufferingSize()
{
	return m_uVirtualBufferingSize;
}

void CAkAutoStmBase::StartCaching()
{
	if ( m_bCachingReady )
		Start();
}

AkUInt32 CAkAutoStmBase::StopCaching(AkUInt32 in_uMemNeededForTask)
{
	// Lock status.
	AkAutoLock<CAkLock> status( m_lockStatus );

	AkUInt32 uMemFreed = ReleaseCachingBuffers(in_uMemNeededForTask);

	SetRunning( false );
	UpdateSchedulingStatus();

#ifndef AK_OPTIMIZED
	//Prevent stopped caching streams from appearing as active in the profiler.
	m_bCanClearActiveProfile = true;
#endif

	return uMemFreed;
}

void CAkAutoStmBase::SetCachingBufferSize(AkUInt32 in_uNumBytes)
{
	AKASSERT(IsCachingStream());
	m_uCachingBufferSize = AkMax( AK_CACHING_STREAM_MIN_BUFFER_SIZE, (((in_uNumBytes - 1) / m_uBufferAlignment) + 1 ) * m_uBufferAlignment );
}

AkUInt32 CAkAutoStmBase::ReleaseCachingBuffers(AkUInt32 in_uTargetMemToRecover)
{
	AkUInt32 uMemFreed = 0;
	
	if ( uMemFreed < in_uTargetMemToRecover && m_listBuffers.Length() > 0 )
	{
		AkAutoLock<CAkIOThread> lock( *m_pDevice ); 		// Lock scheduler for memory change.

		CAkStmMemView * pLast = m_listBuffers.Last();
		while (pLast != NULL && uMemFreed < in_uTargetMemToRecover) 
		{
			uMemFreed += pLast->Size();
			AKVERIFY( m_listBuffers.Remove(pLast) );
			DestroyBuffer( pLast );

			pLast = m_listBuffers.Last();
		}
	}

	return uMemFreed;
}

// Get the amount of buffering that the stream has. 
// Returns
// - AK_DataReady: Some data has been buffered (out_uNumBytesAvailable is greater than 0).
// - AK_NoDataReady: No data is available, and the end of file has not been reached.
// - AK_NoMoreData: Some or no data is available, but the end of file has been reached. The stream will not buffer any more data.
// - AK_Fail: The stream is invalid due to an I/O error.
AKRESULT CAkAutoStmBase::QueryBufferingStatus( AkUInt32 & out_uNumBytesAvailable )
{
	if ( AK_EXPECT_FALSE( m_bIOError ) )
		return AK_Fail;

	// Lock status to query buffering.
	AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );

	if (!IsFileOpened())
		return m_bIOError ? AK_Fail : AK_NoDataReady;

	AKRESULT eRetCode;
	bool bBufferingReady;
	do 
	{
		eRetCode = CalcUnconsumedBufferSize( out_uNumBytesAvailable );

		// Try to see if it is readily available in cache as long is it needs more data. 
		bBufferingReady = NeedsNoMoreTransfer( out_uNumBytesAvailable );

	} while ( 
		!bBufferingReady
		&& m_pDevice->ExecuteCachedTransfer( this ) );

	
	// Deal with end of stream: return AK_NoMoreData if we are not going to stream in any more data,
	// or if the device currently cannot stream in anymore data. Clients must be aware that the device
	// is idle to avoid hangs.
	if ( bBufferingReady
		|| m_pDevice->CannotScheduleAutoStreams() )
	{
		eRetCode = AK_NoMoreData;
	}

	return eRetCode;
}

AKRESULT CAkAutoStmBase::CalcUnconsumedBufferSize(AkUInt32 & out_uNumBytesAvailable)
{
	AKRESULT eRetCode = AK_NoDataReady;

	out_uNumBytesAvailable = 0;

	// Skip granted buffers.
	AkBufferList::Iterator it = m_listBuffers.Begin();
	AkUInt32 uIdx = 0;
	AKASSERT( m_uNextToGrant <= m_listBuffers.Length() );
	AkUInt32 uSkip = m_uNextToGrant;
	while ( uIdx < uSkip )
	{
		++uIdx;
		++it;
	}

	// Count in buffers that are already there.
	while ( it != m_listBuffers.End() )
	{
		out_uNumBytesAvailable += (*it)->Size();
		eRetCode = AK_DataReady;
		++it;
	}

	return eRetCode;
}

// Returns the target buffering size based on the throughput heuristic.
AkUInt32 CAkAutoStmBase::GetNominalBuffering()
{
	AkUInt32 uNominalBuffering;
	
	if (IsCachingStream())
	{
		uNominalBuffering= m_uCachingBufferSize;
	}
	else
	{
		uNominalBuffering= (AkUInt32)(m_pDevice->GetTargetAutoStmBufferLength() * m_fThroughput);
		uNominalBuffering = AkMax(m_uMinimalBufferingSize, uNominalBuffering);
	}

	return uNominalBuffering;
}



//-----------------------------------------------------------------------------
// CAkStmTask implementation.
//-----------------------------------------------------------------------------

AKRESULT CAkAutoStmBase::PostOpenHelper()
{
	m_uBufferAlignment = m_uLLBlockSize;

	//Adjust the loop points, now we know the file size and blocksize
	m_uLoopStart = m_uLoopStart - (m_uLoopStart % m_uLLBlockSize);
	if ((AkInt64)m_uLoopEnd > m_pFileDesc->iFileSize)
		m_uLoopEnd = m_pFileDesc->iFileSize;

	if (m_pFileDesc->iFileSize == 0)
		SetReachedEof(true);

	return AdjustBufferingSettings();
}

void CAkAutoStmBase::OnFileDeferredOpen(const AkAsyncFileOpenData& in_OpenData, AKRESULT in_eResult)
{
	if (in_eResult != AK_Success)
	{
		//Error should have been reported by the IOHook.
		UpdateTaskStatus(AK_Fail);
		return;
	}

	if (in_OpenData.pFlags)
		m_cacheID = in_OpenData.pFlags->uCacheID;
	
	m_eOp = OpRead;	//Auto streams only read.
	
	in_eResult = PostOpenHelper();
	if (in_eResult == AK_Success)
		in_eResult = SetPosition(m_uNextExpectedUserPosition, AkMoveMethod::AK_MoveBegin);

	//Unblock this stream
	UpdateTaskStatus(in_eResult);
	
	if (m_bIsRunning)
	{
		m_bIsRunning = false;	//Now that the stream is open, re-set the running state.
		Start();
	}
}

// Add a new streaming memory view (or content of a memory view) to this stream after a transfer.
// If it ends up not being used, it is disposed of. Otherwise it's status is set to Ready.
// All logical transfers must end up here, even if they were cancelled.
// Sync: Status must be locked prior to calling this function. 
void CAkAutoStmBase::AddMemView( 
	CAkStmMemView * in_pMemView,		// Transfer-mode memory view to resolve.
	bool			in_bStoreData		// Store data in stream object only if true.
	)
{
	AKASSERT( in_pMemView );

	if ( in_bStoreData 
		&& !m_bIsToBeDestroyed 
		&& !m_bIOError )
    {
		// Data size cannot be passed the end of file. 
		AKASSERT ( in_pMemView->EndPosition() <= FileSize() );

		// Profiling. 
#ifndef AK_OPTIMIZED
		{
			AkUInt32 uTransferSize = in_pMemView->Size();
			AKASSERT(uTransferSize > 0);
			m_uBytesTransfered += uTransferSize;

			bool bIsFromLowLevel = in_pMemView->Status() != CAkStmMemView::TransferStatus_Ready;
			if (bIsFromLowLevel)
				m_uBytesTransferedLowLevel += uTransferSize;	// Was not already ready: required low-level transfer.

			// Push transfer statistics only if monitoring, as this requires unwanted locking.
			if (m_pDevice->IsMonitoring())
				m_pDevice->PushTransferStatistics(uTransferSize, bIsFromLowLevel);
		}
#endif

		// Add buffer to list.
		in_pMemView->TagAsReady();
		m_listBuffers.AddLast( in_pMemView );
    }
    else
    {
		// Stream was either scheduled to be destroyed while I/O was occurring, stopped, or 
        // its position was set dirty while I/O was occuring. Flush that data.

		// Note: I/O pool access must be enclosed in scheduler lock.
		AkAutoLock<CAkIOThread> lock( *m_pDevice );

		DestroyBuffer( in_pMemView );
    }
}

// Update task's status after transfer.
void CAkAutoStmBase::UpdateTaskStatus(
	AKRESULT	in_eIOResult			// AK_Success if IO was successful, AK_Cancelled if IO was cancelled, AK_Fail otherwise.
	)
{
    // Status.
    if ( in_eIOResult != AK_Success && in_eIOResult != AK_Cancelled )
    {
        // Set to Error.
        m_bIOError = true;

        // Stop auto stream.
        Stop();
    }
	else
	{
		// Update scheduling status.
		UpdateSchedulingStatus();
	}

	m_pDevice->SignalIOCompleted( this );
}

void CAkAutoStmBase::Kill()
{
	AkAutoLock<CAkLock> updateStatus( m_lockStatus );
	UpdateTaskStatus( AK_Fail );
}

// Compute task's deadline for next operation.
// Sync: None, but copy throughput heuristic on the stack.
AkReal32 CAkAutoStmBase::EffectiveDeadline()
{
	// Note: Sync. These values might be changed by another thread. 
	// In the worst case, the scheduler will take a sub-optimal decision.
	AkReal32 fThroughput= GetThroughput();
	AKASSERT( fThroughput >= AK_MINIMAL_THROUGHPUT );
	return m_uVirtualBufferingSize / fThroughput;
}

//-----------------------------------------------------------------------------
// Profiling.
//-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
// Profiling: get data.
void CAkAutoStmBase::GetStreamData(
    AkStreamData & out_streamData
    )
{
	AkAutoLock<CAkLock> stmBufferGate(m_lockStatus);

    out_streamData.uStreamID = m_uStreamID;
    out_streamData.uPriority = m_priority;
	out_streamData.uFilePosition = m_uNextExpectedUserPosition;
	out_streamData.uVirtualBufferingSize = m_uVirtualBufferingSize;
	// IMPORTANT: Target buffering size logic should be kept in sync with NeedsBuffering.
	out_streamData.uTargetBufferingSize = GetNominalBuffering();
	
	if (IsCachingStream())
	{
		out_streamData.fEstimatedThroughput= 0.f;
	}
	else
	{
		out_streamData.fEstimatedThroughput = m_fThroughput;
	}

	out_streamData.uNumBytesTransfered = m_uBytesTransfered;
	out_streamData.uNumBytesTransferedLowLevel = m_uBytesTransferedLowLevel;
	out_streamData.bActive = m_bWasActive;

	out_streamData.uMemoryReferenced = 0;
	out_streamData.uBufferedSize = 0;
	{
		AkUInt32 uNextToGrant = m_uNextToGrant;

		// Add up memory
		for (AkBufferList::Iterator it = m_listBuffers.Begin(); it != m_listBuffers.End(); ++it)
		{
			out_streamData.uMemoryReferenced += (*it)->AllocSize();

			if (uNextToGrant == 0)
				out_streamData.uBufferedSize += GetEffectiveViewSize(*it);
			else
				uNextToGrant--;
		}

		if ( m_bCanClearActiveProfile )
			m_bWasActive = false;
	}

	// Clamp buffering size to target.
	if (out_streamData.uVirtualBufferingSize > out_streamData.uTargetBufferingSize)
		out_streamData.uVirtualBufferingSize = out_streamData.uTargetBufferingSize;	// Clamp amount of available data to target buffer length.

	if (out_streamData.uBufferedSize > out_streamData.uTargetBufferingSize)
		out_streamData.uBufferedSize = out_streamData.uTargetBufferingSize;	// Clamp amount of available data to target buffer length.

	m_uBytesTransfered = 0;    // Reset.
	m_uBytesTransferedLowLevel = 0;
}

// Signals that stream can be destroyed.
void CAkAutoStmBase::ProfileAllowDestruction()	
{
	AkAutoLock<CAkLock> statusChange(m_lockStatus);
	AKASSERT( m_bIsToBeDestroyed );
	m_bIsProfileDestructionAllowed = true;
	if (CanBeDestroyed())
		UpdateSchedulingStatus();
}
#endif

//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------
// Update task status.
// Sync: Status lock.
void CAkAutoStmBase::ForceFilePosition(
    const AkUInt64 in_uNewPosition     // New stream position.
    )
{
    // Lock status.
    AkAutoLock<CAkLock> statusGate( m_lockStatus );

	// Update position.
	m_uNextExpectedUserPosition = in_uNewPosition;

    // Check whether next buffer position corresponds to desired position (if SetPosition() was consistent
	// with looping heuristic, it will be). If it isn't, then we need to flush everything we have.

	if ( m_uNextToGrant < m_listBuffers.Length() )
	{
		if ( GetNextBufferToGrant()->StartPosition() != in_uNewPosition )
    	{
			// Flush everything we have, that was not already granted to user.
			// Note: Flush() also flushes pending transfers.
			Flush();
			AKASSERT( m_listBuffers.Length() == m_uNextToGrant );
    	}
		else
			UpdateSchedulingStatus();
    }
	else 
	{
		// Nothing buffered. Yet, there might be pending transfers that are inconsistent with in_uNewPosition.
		// Cancel all pending transfers if applicable.
		CancelInconsistentPendingTransfers( in_uNewPosition );
		UpdateSchedulingStatus();
	}
}

// Update task scheduling status; whether or not it is waiting for I/O and counts in scheduler semaphore.
// Sync: Status MUST be locked from outside.
void CAkAutoStmBase::UpdateSchedulingStatus()
{
	AKASSERT(m_eOpenStatus != AK_StmOpenStatus_InFlight || m_bIOError || !"UpdateSchedulingStatus should never be called before the file is opened.");
	CHECK_BUFFERING_CONSISTENCY();

	// Set EOF flag.
	if ( !m_uLoopEnd && GetVirtualFilePosition() >= FileSize() )
        SetReachedEof( true );
    else
		SetReachedEof( false );

    // Update scheduler control.

	if ( ( ReadyForIO() && NeedsBuffering( m_uVirtualBufferingSize ) )	// requires a transfer.
		|| ( IsToBeDestroyed() && CanBeDestroyed() ) )	// requires clean up.
    {
        if ( !m_bRequiresScheduling )
        {
			m_bRequiresScheduling = true;
#ifndef AK_OPTIMIZED
			m_bWasActive = true;
			m_bCanClearActiveProfile = false;
#endif
            m_pDevice->AutoSemIncr();
        }
    }
    else
    {
        if ( m_bRequiresScheduling )
        {
			m_bRequiresScheduling = false;
            m_pDevice->AutoSemDecr();
        }
    }
}

// Compares in_uVirtualBufferingSize to target buffer size.
// Important: Keep computation of target buffer size in sync with GetStreamData().
bool CAkAutoStmBase::NeedsBuffering(
	AkUInt32 in_uVirtualBufferingSize
	)
{
	// Needs buffering if below target buffer length.
	return ( in_uVirtualBufferingSize < GetNominalBuffering() );
}

// Returns a buffer filled with data. NULL if no data is ready.
// Sync: Accessing list. Must be locked from outside.
void * CAkAutoStmBase::GetReadBuffer(     
    AkUInt32 & out_uSize                // Buffer size.
    )
{
	AKASSERT( m_uNextToGrant < 255 || !"Cannot get more than 255 buffers at once" );

    if ( m_uNextToGrant < m_listBuffers.Length() )
    {
		// Get first buffer not granted.
		CAkStmMemView * pMemView = GetNextBufferToGrant();

		if ( pMemView->StartPosition() != m_uNextExpectedFilePosition )
        {
            // User attempts to read a buffer passed the end loop point heuristic, but did not set the
            // stream position accordingly!
			// This should never occur if the client is consistent with the heuristics it sets.
            // Flush data reset looping heuristics for user to avoid repeating this mistake, return AK_NoDataReady.
			AKASSERT(!"Auto stream position inconsistant");
            SetLoopEnd( 0 );
			Flush();
            out_uSize = 0;
            return NULL;
        }
        
        // Update "next to grant" index.
		m_uNextToGrant++;

		// Update amount of buffered data (data granted to user does not count as buffered data).
		out_uSize = pMemView->Size();
		m_uVirtualBufferingSize -= GetEffectiveViewSize( pMemView );

		UpdateSchedulingStatus();
        
		AkUInt64 offset = m_uNextExpectedUserPosition - m_uNextExpectedFilePosition;	//Remove offset due to seeking on unaligned block boundaries
        void *pBuf = ((AkUInt8*)pMemView->Buffer() + offset);
		out_uSize -= (AkUInt32)offset;

		// Update m_iNextExpectedUserPosition.
		m_uNextExpectedFilePosition = pMemView->EndPosition();
		m_uNextExpectedUserPosition = m_uNextExpectedFilePosition;
		
		return pBuf;
    }
    
    // No data ready.
    out_uSize = 0;
    return NULL;
}

// Flushes all stream buffers that are not currently granted.
// Sync: None. Always called from within status-protected code.
void CAkAutoStmBase::Flush()
{
	if (!IsFileOpened())
		return;

	CHECK_BUFFERING_CONSISTENCY();

	CancelAllPendingTransfers();

    if ( m_listBuffers.Length() > m_uNextToGrant )
    {
		AkUInt32 uIdx = 0;
		AkBufferList::IteratorEx it = m_listBuffers.BeginEx();
		while ( uIdx < m_uNextToGrant )
	    {
			++uIdx;
			++it;
		}
        
		// Lock scheduler for memory change.
		AkAutoLock<CAkIOThread> lock( *m_pDevice );
        while ( it != m_listBuffers.End() )
        {
			CAkStmMemView * pMemView = (*it);
			it = m_listBuffers.Erase( it );
			DestroyBuffer( pMemView );
        }
    }

	UpdateSchedulingStatus();
}

AK::StreamMgr::CAkAutoStmMemory::CAkAutoStmMemory(void * pBuffer, AkUInt64 sz)
	: m_uMinBufferSize(1)
	, m_pBuffer(static_cast<AkUInt8*>(pBuffer))
	, m_uSize(sz)
	, m_uPosition(0)
	, m_uNumBuffers(0)
	, m_pszStreamName(NULL)
{
}

AK::StreamMgr::CAkAutoStmMemory::~CAkAutoStmMemory()
{
	if (m_pszStreamName != NULL)
	{
		AkFree(AkMemID_Streaming, m_pszStreamName);
		m_pszStreamName = NULL;
	}
}

void AK::StreamMgr::CAkAutoStmMemory::Destroy()
{
	AkDelete( AkMemID_Streaming, this );
}

void AK::StreamMgr::CAkAutoStmMemory::GetInfo(AkStreamInfo & out_info)
{
	out_info.bIsOpen = true;
	out_info.deviceID = AK_INVALID_DEVICE_ID;
	out_info.pszName = m_pszStreamName;
	out_info.uSize = m_uSize;
	out_info.bIsLanguageSpecific = false;
}

void * AK::StreamMgr::CAkAutoStmMemory::GetFileDescriptor()
{
	return nullptr;
}

void AK::StreamMgr::CAkAutoStmMemory::GetHeuristics(AkAutoStmHeuristics & out_heuristics)
{
	out_heuristics = m_Heuristics;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::SetHeuristics(const AkAutoStmHeuristics & in_heuristics)
{
	m_Heuristics = in_heuristics;
	return AK_Success;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::SetMinimalBufferSize(AkUInt32 in_uMinBufferSize)
{
	m_uMinBufferSize = in_uMinBufferSize;
	return AK_Success;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::SetStreamName(const AkOSChar * in_pszStreamName)
{
	if ( m_pszStreamName != NULL )
		AkFree( AkMemID_Streaming, m_pszStreamName );

	if ( in_pszStreamName != NULL )
	{
		// Allocate string buffer for user defined stream name.
		size_t uStrLen = AKPLATFORM::OsStrLen( in_pszStreamName ) + 1;
		m_pszStreamName = (AkOSChar*)AkAlloc( AkMemID_Streaming, uStrLen * sizeof(AkOSChar) );
		if ( m_pszStreamName == NULL )
			return AK_InsufficientMemory;

		// Copy.
		AKPLATFORM::SafeStrCpy( m_pszStreamName, in_pszStreamName, uStrLen );
	}
	return AK_Success;
}

AkUInt32 AK::StreamMgr::CAkAutoStmMemory::GetBlockSize()
{
	return sizeof(AkUInt8);
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::QueryBufferingStatus(AkUInt32 & out_uNumBytesAvailable)
{
	out_uNumBytesAvailable = (AkUInt32)(m_uSize - m_uPosition);
	return AK_NoMoreData; // We are always fully buffered
}

AkUInt32 AK::StreamMgr::CAkAutoStmMemory::GetNominalBuffering()
{
	return (AkUInt32)(m_uSize - m_uPosition);
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::Start()
{
	return AK_Success;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::Stop()
{
	return AK_Success;
}

AkUInt64 AK::StreamMgr::CAkAutoStmMemory::GetPosition(bool * out_pbEndOfStream)
{
	return m_uPosition;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::SetPosition(AkInt64 in_iMoveOffset, AkMoveMethod in_eMoveMethod)
{
	AkInt64 iPosition;
	if ( in_eMoveMethod == AK_MoveBegin )
	{
		iPosition = in_iMoveOffset;
	}
	else if ( in_eMoveMethod == AK_MoveCurrent )
	{
		iPosition = m_uPosition + in_iMoveOffset;
	}
	else if ( in_eMoveMethod == AK_MoveEnd )
	{
		iPosition = m_uSize + in_iMoveOffset;
	}
	else
	{
		AKASSERT( !"Invalid move method" );
		return AK_InvalidParameter;
	}

	if ( iPosition < 0 )
	{
		AKASSERT( !"Trying to move the file pointer before the beginning of the file" );
		return AK_InvalidParameter;
	}

	m_uPosition = iPosition;
	return AK_Success;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::GetBuffer(void *& out_pBuffer, AkUInt32 & out_uSize, bool)
{
	// Always return the remainder of the buffer
	out_pBuffer = m_pBuffer + m_uPosition;
	out_uSize = (AkUInt32)(m_uSize - m_uPosition);
	m_uNumBuffers++;
	return AK_NoMoreData;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::ReleaseBuffer()
{
	if (m_uNumBuffers == 0)
	{
		return AK_Fail;
	}
	m_uNumBuffers--;
	return AK_Success;
}

AKRESULT AK::StreamMgr::CAkAutoStmMemory::Relocate(AkUInt8 * in_pNewBuffer)
{
	m_pBuffer = in_pNewBuffer;
	return AK_Success;
}
