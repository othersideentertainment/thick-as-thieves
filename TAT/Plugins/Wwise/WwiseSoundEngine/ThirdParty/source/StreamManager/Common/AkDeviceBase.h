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
// AkDeviceBase.h
//
// Device implementation that is common across all high-level IO devices.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DEVICE_BASE_H_
#define _AK_DEVICE_BASE_H_

#include "AkIOThread.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkStreamMgr.h"
#include "AkIOMemMgr.h"
#include "AkStmMemView.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkListBareLight.h>
#include <AK/Tools/Common/AkMonitorError.h>

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

// ------------------------------------------------------------------------------
// Defines.
// ------------------------------------------------------------------------------

#ifdef AK_OS_WCHAR
#define OS_PRINTF	swprintf
#else
#define OS_PRINTF	sprintf
#endif


/// Stream type.
enum AkStmType
{
	AK_StmTypeStandard	= 0,	///< Standard stream for manual IO (AK::IAkStdStream).
	AK_StmTypeAutomatic	= 1,	///< Automatic stream (AK::IAkAutoStream): IO requests are scheduled automatically into internal Stream Manager memory.
	AK_StmTypeCaching	= 2		///< Caching stream. An auto stream that will cache up to a predefined amount of bytes, managed by the user.
};

enum AkStmOpenStatus
{
	AK_StmOpenStatus_Idle     = 0,  ///< Waiting for I/O thread to send an open request to LLIO
	AK_StmOpenStatus_InFlight = 1,  ///< File open request was sent to LLIO. No callback was received yet.
	AK_StmOpenStatus_Complete = 2   ///< File open request is complete. Stream enters its normal lifecycle (which is different for Std and Auto streams)
};

namespace AK
{
namespace StreamMgr
{
	class CAkStmTask;
	class CAkStdStmBase;
	class CAkAutoStmBase;
	class CAkLowLevelDeferredOpen;

	//-----------------------------------------------------------------------------
    // Name: CAkDeviceBase
    // Desc: Base implementation of the high-level I/O device interface.
    //       Implements the I/O thread, provides Stream Tasks (CAkStmTask) 
    //       scheduling services for data transfer. 
    // Note: Device description and access to platform API is handled by low-level.
    //       The I/O thread calls pure virtual method PerformIO(), that has to be
    //       implemented by derived classes according to how they communicate with
    //       the Low-Level IO.
    //       Implementation of the device logic is distributed across the device
    //       and its streaming objects.
    //-----------------------------------------------------------------------------
    class CAkDeviceBase : public CAkIOThread
#ifndef AK_OPTIMIZED
						, public AK::IAkDeviceProfile
#endif
    {
    public:

        CAkDeviceBase(
			IAkLowLevelIOHook *	in_pLowLevelHook
			);
        virtual ~CAkDeviceBase( );
        
		// Methods used by Stream Manager.
		// -------------------------------

       virtual AKRESULT	Init( 
            const AkDeviceSettings &    in_settings,
            AkDeviceID                  in_deviceID,
            bool                        in_bUseIOThread
            );
        virtual void	Destroy();

        AkDeviceID		GetDeviceID();

		// Update the priority of a caching stream.  This function takes care of making sure the appropriate caching streams are started 
		//	again if necessary, if we are at or near the caching memory limit.
		void UpdateCachingPriority(CAkStmTask* in_pStmTask, AkPriority in_uNewPriority);

		// Force the device to clean up dead tasks. 
		void			ForceCleanup(
			bool in_bKillLowestPriorityTask,				// True if the device should kill the task with lowest priority.
			AkPriority in_priority							// Priority of the new task if applicable. Pass AK_MAX_PRIORITY to ignore.
            );

		// Cache management.
		void			FlushCache();
        
        // Methods used by stream objects.
        // --------------------------------------------------------
        
        // Access for stream objects.
		inline IAkLowLevelIOHook * GetLowLevelHook()
		{
			return m_pLowLevelHook;
		}
        inline AkUInt32 GetGranularity()
        {
            return m_uGranularity;
        }
        inline AkReal32 GetTargetAutoStmBufferLength()
        {
            return m_fTargetAutoStmBufferLength;
        }
        inline AkInt64 GetTime()
        {
            return m_time;
        }
		inline bool UseCache() 
		{ 
			return m_mgrMemIO.UseCache();
		}

	
		// IO memory management:
		// Memory views factory.

	protected:

		typedef AkListBareLight<CAkStmTask> TaskList;

		// Create a new, empty streaming memory view.
		// Sync: Caller must hold stream lock.
		virtual CAkStmMemView * MemViewFactory() = 0;

	public:

		// Disposes of a streaming memory view. The memory block it references is released.
		// Returns the refcount of the memory block, 0 if there was no memory block.
		// Sync: Caller must hold device lock.
		inline AkUInt32 DestroyMemView( 
			CAkStmMemView *	in_pMemView			// Mem view to destroy. The memory block it owns is also released. 
			)
		{
			AkUInt32 uRefCount = 0;
			AkMemBlock * pBlock = in_pMemView->Detach();
			if ( pBlock )
				uRefCount = m_mgrMemIO.ReleaseBlock( pBlock );
			AKASSERT( !in_pMemView->Block() );
			AkDelete( AkMemID_Streaming, in_pMemView );
			return uRefCount;
		}

		// Disposes of a mem view, used by standard streams. Unless it is in_pMemBlockBase, the memory block 
		// it references is released and dequeued from the chain starting with in_pMemBlockBase.
		inline void DestroyMemView(
			AkMemBlock *	in_pMemBlockBase,	// Base block, start of the chain.
			CAkStmMemView *	in_pMemView			// Mem view to destroy. The memory block it owns is also released, unless it is in_pMemBlockBase itself.
			)
		{
			AkMemBlock * pBlockToRelease = in_pMemView->Detach();
			if ( pBlockToRelease )
				m_mgrMemIO.DestroyTempBlock( in_pMemBlockBase, pBlockToRelease );
			AKASSERT( !in_pMemView->Block() );
			AkDelete( AkMemID_Streaming, in_pMemView );
		}

		// Try execute next transfer from cache data. 
		// Returns false if it requires a low-level transfer, or if this is not supported by the device.
		virtual bool ExecuteCachedTransfer( CAkStmTask * );
        
		AkUInt32 RemainingCachePinnedBytes() const
		{
			return m_uMaxCachePinnedBytes - m_uCurrentCachePinnedData;	
		}

        // Device Profile Ex interface.
        // --------------------------------------------------------
#ifndef AK_OPTIMIZED

	    // Monitoring status.
        virtual AKRESULT     StartMonitoring();
	    virtual void         StopMonitoring();
        inline bool          IsMonitoring() { return m_bIsMonitoring; }

        // Device profiling.
		virtual void	OnProfileStart();
		virtual void	OnProfileEnd();
        virtual void     GetDesc( 
            AkDeviceDesc & out_deviceDesc 
            );
		virtual void    GetData(
            AkDeviceData &  out_deviceData
            );
        virtual bool     IsNew();
        virtual void     ClearNew();
        
        // Stream profiling.
        virtual AkUInt32 GetNumStreams();
        // Note. The following functions refer to streams by index, which must honor the call to GetNumStreams().
        virtual AK::IAkStreamProfile * GetStreamProfile( 
            AkUInt32    in_uStreamIndex             // [0,numStreams[
            );

		inline void PushTransferStatistics( AkUInt32 in_uSizeTransferred, bool in_bFromLowLevel )
		{
			AkAutoLock<CAkIOThread> lock(*this);

			m_uBytesThisInterval += in_uSizeTransferred;
			m_uBytesThisSession += in_uSizeTransferred;

			if (in_bFromLowLevel)
			{
				m_uNumLowLevelRequests++;
				m_uBytesLowLevelThisInterval += in_uSizeTransferred;
				m_uBytesThisSession += in_uSizeTransferred;
			}

			AKASSERT(m_uBytesThisInterval >= m_uBytesLowLevelThisInterval);
		}

#endif
		void AddTask(CAkStmTask* in_pStmTask);
		void RemoveTask(CAkStmTask* in_pStmTask);

    protected:

		void CountStreamsInTaskList(TaskList& in_listTasks);

        // Destroys all streams.
		// Returns true if it was able to destroy all streams. Otherwise, the IO thread needs to
		// wait for pending transfers to complete.
        virtual bool ClearStreams();

		bool ClearTaskList(TaskList& in_taskList);
		
		//Only destroy tasks marked dead already. Contrast with ClearTaskList which kills remaining tasks too. Need to be locked externally.
		void CleanDeadTasks(TaskList& in_taskList);

		//Return true if a task was killed
		bool ForceTaskListCleanup(
			bool in_bKillLowestPriorityTask,				// True if the device should kill the task with lowest priority.
			AkPriority in_priority,							// Priority of the new task if applicable. Pass AK_MAX_PRIORITY to ignore.
			TaskList& in_listTasks
			);

		// Called once when I/O thread starts.
		virtual void OnThreadStart();

		// Scheduler algorithm.
		// Finds the next task for which an I/O request should be issued.
		// Return: If a task is found, a valid pointer to a task is returned, as well
		// as the operation's deadline (for low-level IO heuristics).
		// Otherwise, returns NULL.
        CAkStmTask *    SchedulerFindNextTask(
			AkReal32 &		out_fOpDeadline	// Returned deadline for this transfer.
            );

		CAkStmTask *    SchedulerFindNextCachingTask();

        // Finds next task among standard streams only (typically when there is no more memory for automatic streams).
        CAkStmTask *    ScheduleStdStmOnly(
			AkReal32 &	out_fOpDeadline		// Returned deadline for this transfer.
            );

	protected:
		typedef AkArrayAllocatorNoAlign<AkMemID_Streaming> ArrayPoolLocal;
		CAkStmTask * SchedulerFindLowestPriorityCachingTask(AkPriority in_uMaxPriority);

		// Time in milliseconds. Stamped at every scheduler pass.
        AkInt64         m_time;

		// Task list.
        // Tasks live in m_arTasks from the time they are created until they are completely destroyed (by the I/O thread).
        // It is more efficient to query the tasks every time scheduling occurs than to add/remove them from the list, 
        // every time, from other threads.
        
        TaskList        m_listTasks;            // List of tasks.
        TaskList        m_listCachingTasks;     // List of caching tasks.
        CAkLock         m_lockTasksList;        // Protects tasks array.

		// Stream IO memory.
		CAkIOMemMgr			m_mgrMemIO;

		// Low-Level I/O hook.
		IAkLowLevelIOHook *	m_pLowLevelHook;

		// Settings.
        AkUInt32        m_uGranularity;
        AkReal32        m_fTargetAutoStmBufferLength;
        /** Needed at thread level (CAkIOThread)
		AkUInt32        m_uMaxConcurrentIO;
		**/

        AkDeviceID      m_deviceID;

		AkUInt32 m_uMaxCachePinnedBytes;
		AkUInt32 m_uCurrentCachePinnedData;

        // Profiling specifics.
#ifndef AK_OPTIMIZED

		//Per profile interval data:
		AkInt32			m_uNumActiveStreams;	// Number of automatic streams that are running, plus number of standard streams that have an operation pending.
		AkUInt32		m_uBytesLowLevelThisInterval;	// Bandwidth of real transfers.
		AkUInt32		m_uBytesThisInterval;	// Bandwidth, including the amount of data newly referenced from cache.
		AkUInt32		m_uNumLowLevelRequests;	// Number of requests to the low-level IO since lass profiling pass.
		AkUInt32		m_uNumLowLevelRequestsCancelled;	// Number of requests to the low-level IO that were cancelled since lass profiling pass.
        
		//Per profiling session data:
		AkUInt64		m_uBytesThisSession;	// Data, including the data grabbed from cache, over the life of the profiling session.
		AkUInt64		m_uCacheBytesThisSession;	// Data grabbed from cache, over the life of the profiling session.

		//Flags
		bool            m_bIsMonitoring;
		bool            m_bIsNew;

	protected:
		typedef AkArray< AK::IAkStreamProfile*, AK::IAkStreamProfile*, ArrayPoolLocal > ArrayStreamProfiles;
        ArrayStreamProfiles m_arStreamProfiles; // Tasks pointers are copied there when GetNumStreams() is called, to avoid 
                                                // locking-unlocking the real tasks list to query each stream's profiling data.
		bool            m_bTasksListLockAcquired;    // track if tasks array is locked
#endif
    };


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
	class CAkStmTask : public CAkClientThreadAware
#ifndef AK_OPTIMIZED
        , public IAkStreamProfile
#endif
    {
    public:
		virtual ~CAkStmTask();

		AKRESULT Open(const AkFileOpenData& in_FileOpen, bool in_bSync, bool in_bAuto);
		
		// Scheduling interface.
		// -------------------------

        // Returns true when the object is ready to be destroyed. Common. 
        inline bool IsToBeDestroyed()
        {
            // Note. When profiling, we need to have the profiler's agreement to destroy the stream.
        #if !defined(AK_OPTIMIZED) && !defined(WWISE_AUTHORING)
            return m_bIsToBeDestroyed && 
                ( !m_pDevice || !m_pDevice->IsMonitoring( ) || m_bIsProfileDestructionAllowed );
        #else
            return m_bIsToBeDestroyed;
        #endif
        }

		// Task needs to acknowledge that it can be destroyed. Device specific (virtual). Call only when IsToBeDestroyed().
		// Note: Implementations must always lock the task's status in order to avoid race conditions between
		// client's Destroy() and the I/O thread calling InstantDestroy().
		virtual bool CanBeDestroyed();

        // Destroys the object. Must be called only if IsToBeDestroyed() and CanBeDestroyed() returned True.
        inline void InstantDestroy()
		{
			AKASSERT( IsToBeDestroyed() && CanBeDestroyed() );
			// Destroys itself.
			AkDelete( AkMemID_Streaming, this );
		}

		// Sets the object in error state.
		virtual void Kill() = 0;

        // Settings access.
        inline AkStmType StmType()      // Task stream type.
        {
			return m_eStmType;
        }

		enum OpType { OpRead, OpWrite, OpOpen, OpCount };
		inline OpType CurrentOp()
		{
			return m_eOp;
		}
		
        inline AkPriority Priority()    // Priority.
        {
            AKASSERT( m_priority >= AK_MIN_PRIORITY &&
                    m_priority <= AK_MAX_PRIORITY );
            return m_priority;
        }


		AKRESULT SetPriority(const AkPriority in_priority)
		{
			if (in_priority < AK_MIN_PRIORITY || in_priority > AK_MAX_PRIORITY)
			{
				AKASSERT(!"Invalid stream heuristics");
				return AK_InvalidParameter;
			}

			m_priority = in_priority;
			return AK_Success;
		}

        // Get information for data transfer.
		// Returns the (device-specific) streaming memory view containing logical transfer information. 
		// NULL if preparing transfer has aborted.
		// out_pLowLevelXfer is set if and only if the transfer requires a transfer in the low-level IO.
		// Sync: Locks stream's status.
		virtual CAkStmMemView * PrepareTransfer( 
			AkFileDesc *&			out_pFileDesc,		// Stream's associated file descriptor.
			CAkLowLevelTransfer *&	out_pLowLevelXfer,	// Low-level transfer. Set to NULL if it doesn't need to be pushed to the Low-Level IO.
			bool &					out_bExistingLowLevelXfer, // Indicates a low level transfer already exists that references the memory block returned.
			bool					in_bCacheOnly		// Prepare transfer only if data is found in cache. out_pLowLevelXfer will be NULL.
			) = 0;

		AKRESULT PrepareOpen(AkAsyncFileOpenData*& in_pLLOpen);

		void HandleFileOpenResult(AkAsyncFileOpenData * in_pOpenData, AKRESULT in_eResult);
		virtual void OnFileDeferredOpen(const AkAsyncFileOpenData& in_OpenData, AKRESULT in_eResult) = 0;

        // Update stream object after I/O.
		// Sync: Locks stream's status.
		// Return true if a pending buffer was added to the buffer list
		virtual bool Update(
			CAkStmMemView *	in_pTransfer,	// Logical transfer object.
			AKRESULT		in_eIOResult,	// AK_Success if IO was successful, AK_Cancelled if IO was cancelled, AK_Fail otherwise.
			bool			in_bRequiredLowLevelXfer	// True if this transfer required a call to low-level.
			) = 0;

		// Returns True if the task should be considered by the scheduler for an I/O operation.
		// - Standard streams are ready for I/O when they are Pending.
        // - Automatic streams are ready for I/O when they are Running and !EOF.
		// - Both types are ready for I/O when a file open request must be sent
		inline bool ReadyForIO()
		{
			AkAutoLock<CAkLock> status( m_lockStatus );
			return m_bIsReadyForIO;
		}

		// Returns True if the task requires scheduling (tasks requiring scheduling have priority,
		// regardless of their deadline).
        inline bool RequiresScheduling()
		{
			return m_bRequiresScheduling;
		}

        // Scheduling heuristics.
        virtual AkReal32 EffectiveDeadline() = 0;   // Compute task's effective deadline for next operation, in ms.
        AkReal32 TimeSinceLastTransfer(             // Time elapsed since last I/O transfer.
            const AkInt64 & in_liNow                // Time stamp.
            )
        {
            return AKPLATFORM::Elapsed( in_liNow, m_iIOStartTime );
        }

		// Access to file descriptor.
		AkFileDesc * GetFileDesc() { return m_pFileDesc; }

		// Access to device.
		CAkDeviceBase * GetDevice() { return m_pDevice; }

		// Returns file offset in bytes.
		inline AkUInt64 GetFileOffset()
		{
			if (m_pFileDesc)
				return m_pFileDesc->uSector * m_uLLBlockSize;
			return 0;
		}

		// Returns file size, in bytes.
		inline AkUInt64 FileSize()
		{
			if (m_pFileDesc)
				return m_pFileDesc->iFileSize;
			return 0;
		}

		inline bool IsFileOpened() { return m_pFileDesc != nullptr; }

        // Profiling.
#ifndef AK_OPTIMIZED
        
        // IAkStreamProfile interface.
        // ---------------------------
        virtual bool IsNew()                        // Returns true if stream is tagged "New".
        {
            return m_bIsNew;
        }
        virtual void ClearNew()                     // Clears stream's "New" tag.
        {
            m_bIsNew = false;
        }
        virtual void GetStreamRecord( 
            AkStreamRecord & out_streamRecord
            );
        // ---------------------------

        inline bool IsProfileNew()                  // Returns true if stream is tagged "New".
        {
            return m_bIsNew;
        }
		inline bool IsProfileReady()
		{
			return m_pFileDesc != NULL;					// Ready to be profiled when Low-Level Open succeeded.
		}
        inline AK::IAkStreamProfile * GetStreamProfile()    // Returns associated stream profile interface.
        {
            return this;
        }
      
        inline bool ProfileIsToBeDestroyed()        // True when the stream has been scheduled for destruction.
        {
            return m_bIsToBeDestroyed;
        }
        virtual void ProfileAllowDestruction() = 0;	// Signals that stream can be destroyed.
        
		inline bool WasActive()
		{
			return m_bWasActive;
		}
#endif
		inline bool IsCachingStream()
		{
			return m_eStmType == AK_StmTypeCaching;
		}

		inline void SetIsCachingStream()
		{
			AKASSERT(m_eStmType == AK_StmTypeAutomatic);
			m_eStmType = AK_StmTypeCaching;
		}

		// List bare light sibling: device's TaskList.
		CAkStmTask * pNextLightItem = nullptr;

		inline void SetToBeDestroyed()
		{
			m_bIsToBeDestroyed = true;
			SetReadyForIO(false);
		}

		//
		//	Caching stream interface
		//

		virtual void SetCachingBufferSize(AkUInt32 /*in_uNumBytes*/ ) {}

		virtual AkUInt32 GetNominalBuffering()			{return 0;}
		virtual AkUInt32 GetVirtualBufferingSize()		{return 0;}

		virtual void StartCaching(){}

		virtual AkUInt32 StopCaching(AkUInt32 /*in_uMemNeeded*/){ return 0; }

		void GetInfo(AkStreamInfo& out_info);
		AKRESULT SetStreamName(const AkOSChar* in_pszStreamName);
		void SetDevice(CAkDeviceBase* in_pDevice)
		{
			AKASSERT((m_pDevice == nullptr) != (in_pDevice == nullptr));
			m_pDevice = in_pDevice;
		}
		
	protected:

		AKRESULT MoveToNextDevice();
		static void OnDeferredOpen(AkAsyncFileOpenData* in_pOpenInfo, AKRESULT in_eResult);

		virtual AkUInt32 ReleaseCachingBuffers(AkUInt32 /*in_uMemNeeded*/){ return 0; }
		// Helpers.

		// Set "Ready for I/O" status.
        inline void SetReadyForIO( bool in_bReadyForIO )
		{
			// If this doesn't hold, we are most likely going to deadlock.
			AKASSERT(in_bReadyForIO || m_eOpenStatus != AK_StmOpenStatus_Idle || m_bIsToBeDestroyed || !m_pDevice);
			m_bIsReadyForIO = in_bReadyForIO;
		}

		// Returns size clamped to end of file.
		inline AkUInt32 ClampRequestSizeToEof( AkUInt64 in_uPosition, AkUInt32 in_uDesiredSize, bool & out_bEof )
		{
			AkUInt32 uClampedSize = in_uDesiredSize;

			AkUInt64 uPositionOfEof = FileSize();
			AKASSERT(in_uPosition <= uPositionOfEof);
			AkUInt64 uNewPos = AkMin(in_uPosition + in_uDesiredSize, uPositionOfEof);
			uClampedSize = (AkUInt32)(uNewPos - in_uPosition);
			out_bEof = (uNewPos == uPositionOfEof);

			//Check for caching streams
			if ( IsCachingStream() )
			{
				AkUInt32 uNominalBuffering = GetNominalBuffering();
				if(  in_uPosition + uClampedSize > uNominalBuffering )
					uClampedSize = uNominalBuffering - (AkUInt32) in_uPosition;
			}

			return uClampedSize;
		}

        // Common attributes.
        // ------------------
    protected:
        
		CAkStmTask();
		CAkStmTask(CAkStmTask&);

		// Deferred open data storage class.
		class AkDeferredOpenData : public AkAsyncFileOpenData
		{
		public:
			AkDeferredOpenData();
			~AkDeferredOpenData();

			AKRESULT Init(const AkFileOpenData& in_FileOpenParams);

		public:
			AkFileSystemFlags	flagsStorage;
			AkDeviceID			uDeviceID;
			AkUInt32			bByString : 1;
			AkUInt32			bOriginalLanguageFlag : 1;
		};

		void VerifyOpenResult(AKRESULT in_eResult)
		{
			// At this point, we are in one of these cases
			AKASSERT(
				// File is opened and we are done
				(m_eOpenStatus == AK_StmOpenStatus_Complete && m_pFileDesc != nullptr && m_pOpenData == nullptr && !m_bIsToBeDestroyed && in_eResult == AK_Success) ||
				// A file open error occured and we have no more devices to fall back on
				(m_eOpenStatus == AK_StmOpenStatus_Complete && m_pFileDesc == nullptr && m_pOpenData == nullptr && !m_bIsToBeDestroyed && in_eResult != AK_Success) ||
				// A file open error occured and will attempt again with the next device; in this case, the result code could be anything
				(m_eOpenStatus == AK_StmOpenStatus_Idle && m_pOpenData != nullptr && m_pFileDesc == nullptr && !m_bIsToBeDestroyed) ||
				// User has aborted the stream and we cleaned up properly.
				(m_eOpenStatus == AK_StmOpenStatus_Complete && m_bIsToBeDestroyed && m_pOpenData == nullptr && m_pFileDesc == nullptr));
		}

	private:
		AkDeferredOpenData* m_pOpenData = nullptr;	//Open info, as provided by the client; NOT exposed to subclasses

	protected:
        // File info.
        AkFileDesc 			*m_pFileDesc = nullptr;    // File descriptor:
                                                // uFileSize: File size.
                                                // uSector: Position of beginning of file (relative to handle).
                                                // hFile: System handle.
                                                // Custom parameter and size (owned by Low-level).
                                                // Device ID.
        CAkLock				m_lockStatus;       // Lock for status integrity.
        AkInt64		        m_iIOStartTime;     // Time when I/O started. 
        CAkDeviceBase *     m_pDevice;          // Access to owner device.
        AkOSChar *			m_pszStreamName;    // User defined stream name.
        AkUInt32            m_uLLBlockSize;     // Low-level IO block size (queried once at init).

#ifndef AK_SUPPORT_THREAD
		AkEvent *			m_pIOEvent = nullptr;	// Used to signal when a sync IO is completed.
#endif

        // Profiling.
#ifndef AK_OPTIMIZED
		AkFileID			m_idFile = AK_INVALID_FILE_ID;			// FileID (may or may not be the same as the string m_pszStreamName). For profiling.
        AkUInt32            m_uStreamID;        // Profiling stream ID.
        AkUInt32            m_uBytesTransfered; // Number of bytes transferred (replace).
#endif

        AkPriority          m_priority;         // IO priority. Keeps last operation's priority.

		AkStmType			m_eStmType;
		OpType				m_eOp;				// Operation of this task
		AkStmOpenStatus     m_eOpenStatus;      // Status of initial file open operation
		bool				m_bHasReachedEof;	// True when file pointer reached eof.
		bool				m_bIsToBeDestroyed; // True when this stream is scheduled to be destroyed.
		bool				m_bRequiresScheduling; // Stream's own indicator saying if it counts in the scheduler semaphore.
		bool				m_bLanguageSpecific; //File was opened in a language-specific folder.
	private:
		bool				m_bIsReadyForIO;	 // True when task is in a state where it is ready for an I/O transfer (distinct from Deadline-based status).
	
	protected:
        // Profiling.
#ifndef AK_OPTIMIZED
		bool				m_bIsNew; // "New" flag.
        bool				m_bIsProfileDestructionAllowed; // True when profiler gave its approbation for destruction.
		bool				m_bWasActive;	// Set to true as soon as this task requires scheduling.
		bool				m_bCanClearActiveProfile;	// Set to true when this task is updated. When set, m_bWasActive is reset to "m_bRequiresScheduling" at next profiler pass.
#endif
    };

	//-----------------------------------------------------------------------------
    // Name: class CAkStmBase
    // Desc: Base implementation for standard streams.
    //-----------------------------------------------------------------------------
    class CAkStdStmBase : public CAkStmTask,
						  public AK::IAkStdStream
    {
    public:

        // Construction/destruction.
        CAkStdStmBase();
        virtual ~CAkStdStmBase();

		//-----------------------------------------------------------------------------
        // AK::IAkStdStream interface.
        //-----------------------------------------------------------------------------

        // Stream info access.
        virtual void      GetInfo(
            AkStreamInfo &      out_info        // Returned stream info.
            ) override;
		// Returns a unique cookie for a given stream (file descriptor).
		virtual void * GetFileDescriptor() override { return m_pFileDesc; }
        // Name the stream (appears in Wwise profiler).
        virtual AKRESULT  SetStreamName(
            const AkOSChar * in_pszStreamName	// Stream name.
            ) override;
        // Returns I/O block size.
        virtual AkUInt32  GetBlockSize() override;       // Returns block size for optimal/unbuffered IO.
        
        // Operations.
        // ---------------------------------------

		AKRESULT Open(const AkFileOpenData& in_FileOpen, bool in_bSync);
  
		// Read/Write.				
        // Ask for a multiple of the device's atomic block size, 
        // obtained through IAkStdStream::GetBlockSize().
        virtual AKRESULT Read(
            void *          in_pBuffer,         // User buffer address. 
            AkUInt32        in_uReqSize,        // Requested read size.
            bool            in_bWait,           // Block until operation is complete.
            AkPriority      in_priority,        // Heuristic: operation priority.
            AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
            AkUInt32 &      out_uSize           // Size actually read.
            ) override;
        virtual AKRESULT Write(
            void *          in_pBuffer,         // User buffer address. 
            AkUInt32        in_uReqSize,        // Requested write size. 
            bool            in_bWait,           // Block until operation is complete.
            AkPriority      in_priority,        // Heuristic: operation priority.
            AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
            AkUInt32 &      out_uSize           // Size actually written.
            ) override;
        
        // Get current stream position.
        virtual AkUInt64 GetPosition( 
            bool *          out_pbEndOfStream   // Input streams only. Can pass NULL.
            ) override;
        // Set stream position. Modifies position of next read/write.
        virtual AKRESULT SetPosition(
            AkInt64         in_iMoveOffset,     // Seek offset.
            AkMoveMethod    in_eMoveMethod     // Seek method, from beginning, end or current file position.
            ) override;
	
		virtual void* GetData(
			AkUInt32& out_uSize           ///< Size actually read or written
		) override;
        
        virtual AkStmStatus GetStatus() override;        // Get operation status.
		AKRESULT GetResult() {	return m_eResult; }

        /// Block and wait for a pending read to finish, and return the current status of the stream.
        virtual AkStmStatus WaitForPendingOperation() override;

        //-----------------------------------------------------------------------------
        // CAkStmTask interface.
        //-----------------------------------------------------------------------------

		// Scheduling heuristics.
		// Compute task's effective deadline for next operation, in ms.
		virtual AkReal32 EffectiveDeadline() override;

		// Sets the object in error state.
		virtual void Kill() override;

        //-----------------------------------------------------------------------------
        // Profiling.
        //-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
        
        // IAkStreamProfile interface.
        virtual void GetStreamData(
            AkStreamData &   out_streamData
            ) override;

		// Signals that stream can be destroyed.
		virtual void ProfileAllowDestruction() override;
#endif

        //-----------------------------------------------------------------------------
        // Helpers.
        //-----------------------------------------------------------------------------
	protected:

		virtual void OnFileDeferredOpen(const AkAsyncFileOpenData& in_OpenData, AKRESULT in_eResult) override;

		// Execute Operation (either Read or Write).
		AKRESULT ExecuteOp(
			OpType			in_eOp,				// Open, Read or Write.
			void *          in_pBuffer,         // User buffer address. 
			AkUInt32        in_uReqSize,        // Requested write size. 
			bool            in_bWait,           // Block until operation is complete.
			AkPriority      in_priority,        // Heuristic: operation priority.
			AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
			AkUInt32 &      out_uSize           // Size actually written.
			);

        // Set task status. Increment and release Std semaphore.
		// Note: Status must be locked prior to calling this function.
        void SetStatus( AkStmStatus in_eStatus, AKRESULT in_eResult );

		// Stream type specific policies.
		// ------------------------------

		// Add a new streaming memory view (or content of a memory view) to this stream after a transfer.
		// If it ends up not being used, it is disposed of. Otherwise it's status is set to Ready.
		// All logical transfers must end up here, even if they were cancelled.
		// Sync: Status must be locked prior to calling this function. 
		void AddMemView( 
			CAkStmMemView * in_pMemView,		// Transfer-mode memory view to resolve.
			bool			in_bStoreData		// Store data in stream object only if true.
			);

		// Update task's status after transfer.
		void UpdateTaskStatus(
			AKRESULT	in_eIOResult			// AK_Success if IO was successful, AK_Cancelled if IO was cancelled, AK_Fail otherwise.
			);

		// Correct virtual buffering after cancel. Does nothing.
		inline void CorrectVirtualBufferingAfterCancel(
			CAkStmMemView * // io_pMemView 
			) {}

		// Returns stream position, as seen by the user. It is updated when a transfer completes or when
		// the user calls SetPosition(). 
		inline AkUInt64 GetCurUserPosition() { return m_memBlock.uPosition; }
		inline void SetCurUserPosition( AkUInt64 in_uPosition ) { m_memBlock.uPosition = in_uPosition; }

    protected:

		AkMemBlock			m_memBlock;			// Memory block. Maps user-provided memory when an operation starts.
												// AkMemBlock::uPosition keeps the position in file (relative to beginning of file).
		AkUInt32			m_uTotalScheduledSize;	// Total size already scheduled for transfer.
        AkReal32            m_fDeadline;        // Deadline. Keeps last operation's deadline.

        // Operation info.
        AkStmStatus         m_eStmStatus;	 // Stream operation status.
		AKRESULT			m_eResult;		 // Error code when m_eStmStatus is set to Error.

		AkUInt64			m_uNextExpectedUserPosition;	// Expected position of next GetBuffer(), relative to file start, as expected by the client, accurate to the byte. 
		AkUInt64			m_uNextExpectedFilePosition;	// Expected position of next GetBuffer(), relative to file start, rounded by blocksize.

		bool				m_bIsOpComplete; // User request was completed.
    };

    //-----------------------------------------------------------------------------
    // Name: class CAkAutoStmBase
    // Desc: Base automatic stream implementation.
    //-----------------------------------------------------------------------------
    class CAkAutoStmBase : public CAkStmTask,
						   public AK::IAkAutoStream
    {
    public:
    
    	// Construction/destruction.
        CAkAutoStmBase();
        virtual ~CAkAutoStmBase();

        void Init( 
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings  // Stream buffer settings. Pass NULL to use defaults (recommended).
            );

		AKRESULT Open(const AkFileOpenData& in_FileOpen, bool in_bSync);

        //-----------------------------------------------------------------------------
        // AK::IAkAutoStream interface.
        //-----------------------------------------------------------------------------

        // Closes stream. The object is destroyed and the interface becomes invalid.
        virtual void      Destroy() override;

        // Stream info access.
        virtual void      GetInfo(
            AkStreamInfo &      out_info        // Returned stream info.
            ) override;
		// Returns a unique cookie for a given stream (file descriptor).
		virtual void * GetFileDescriptor() override { return m_pFileDesc; }
        // Stream heuristics access.
        virtual void      GetHeuristics(
            AkAutoStmHeuristics & out_heuristics// Returned stream heuristics.
            ) override;
        // Stream heuristics run-time change.
        virtual AKRESULT  SetHeuristics(
            const AkAutoStmHeuristics & in_heuristics   // New stream heuristics.
            ) override;
		// Run-time change of the stream's minimum buffer size that can be handed out to client.
		virtual AKRESULT  SetMinimalBufferSize(
			AkUInt32 in_uMinBufferSize	///< Minimum buffer size that can be handed out to client.
			) override;
		
        // Name the stream (appears in Wwise profiler).
        virtual AKRESULT  SetStreamName(
            const AkOSChar * in_pszStreamName    // Stream name.
            ) override;
        // Returns I/O block size.
        virtual AkUInt32  GetBlockSize() override;


        // Operations.
        // ---------------------------------------
        
        // Starts automatic scheduling.
        virtual AKRESULT Start() override;
        // Stops automatic scheduling.
        virtual AKRESULT Stop() override;

        // Get stream position.
        virtual AkUInt64 GetPosition( 
            bool *          out_pbEndOfStream   // Set to true if reached end of stream. Can pass NULL.
            ) override;
        // Set stream position. Modifies position in stream for next read user access.
        virtual AKRESULT SetPosition(
            AkInt64         in_iMoveOffset,     // Seek offset.
            AkMoveMethod    in_eMoveMethod      // Seek method, from beginning, end or current file position.
            ) override;

        // Data/status access. 
        // -----------------------------------------

        // GetBuffer.
        // Return values : 
        // AK_DataReady     : if buffer was granted.
        // AK_NoDataReady   : if buffer was not granted yet.
        // AK_NoMoreData    : if buffer was granted but reached end of file (next call will return with size 0).
        // AK_Fail          : there was an IO error.
        virtual AKRESULT GetBuffer(
            void *&         out_pBuffer,        // Address of granted data space.
            AkUInt32 &      out_uSize,          // Size of granted data space.
            bool            in_bWait            // Block until data is ready.
            ) override;

        // Release buffer granted through GetBuffer().
        virtual AKRESULT ReleaseBuffer() override;

		// Get the amount of buffering that the stream has. 
		// Returns
		// - AK_DataReady	: Some data has been buffered (out_uNumBytesAvailable is greater than 0).
		// - AK_NoDataReady	: No data is available, and the end of file has not been reached.
		// - AK_NoMoreData	: No data is available, but the end of file has been reached. There will not be any more data.
		// - AK_Fail		: The stream is invalid due to an I/O error.
		virtual AKRESULT QueryBufferingStatus( 
			AkUInt32 & out_uNumBytesAvailable 
			) override;

		//Return the amount of data in all the attached buffers for this stream
		virtual AkUInt32 GetVirtualBufferingSize() override;

		// Set the amount of data that you want to cache in a caching stream.
		virtual void SetCachingBufferSize(AkUInt32 in_uNumBytes) override;

		virtual void StartCaching() override;

		//Stop Caching and release the buffers that are being held in a caching stream, up to in_uMemNeeded bytes. 
		//Return the amount of data that is freed.
		virtual AkUInt32 StopCaching(AkUInt32 in_uMemNeeded) override;

		// Returns the target buffering size based on the throughput heuristic.
		virtual AkUInt32 GetNominalBuffering() override;

        //-----------------------------------------------------------------------------
        // CAkStmTask interface.
        //-----------------------------------------------------------------------------

		// Sets the object in error state.
		virtual void Kill() override;

		// This is called when file size is set after deferred open. Stream object implementations may
		// perform any update required after file size was set. 
		// Automatic stream object ensure that the loop end heuristic is consistent.
		virtual void OnFileDeferredOpen(const AkAsyncFileOpenData& in_OpenData, AKRESULT in_eResult) override;

        // Scheduling heuristics.
        virtual AkReal32 EffectiveDeadline() override;   // Compute task's effective deadline for next operation, in ms.

		//-----------------------------------------------------------------------------
        // Device-specific automatic streams implementation.
        //-----------------------------------------------------------------------------

		// Automatic streams must implement a method that returns the file position after the last
		// valid (non cancelled) pending transfer. If there is no transfer pending, then it is the position
		// at the end of buffering.
		virtual AkUInt64 GetVirtualFilePosition() = 0;

		// Cancel all pending transfers.
		// Stream must be locked.
		virtual void CancelAllPendingTransfers() = 0;

		// Cancel all pending transfers that are inconsistent with the next expected position (argument) 
		// and looping heuristics. 
		// Stream must be locked.
		virtual void CancelInconsistentPendingTransfers(
			AkUInt64 in_uNextExpectedPosition	// Expected file position of next transfer.
			) = 0;
		
		// Run through buffers (completed and pending) and check their data size. If it is smaller than 
		// in_uMinBufferSize, flush it and flush everything that comes after.
		virtual void FlushSmallBuffersAndPendingTransfers( 
			AkUInt32 in_uMinBufferSize // Minimum buffer size.
			) = 0;
		
		// Change loop end heuristic. Use this function instead of setting m_uLoopEnd directly because
		// m_uLoopEnd has an impact on the computation of the effective data size (see GetEffectiveViewSize()).
		// Implemented in derived classes because virtual buffering has to be recomputed.
		virtual void SetLoopEnd( 
			AkUInt64 in_uLoopEnd	// New loop end value.
			) = 0;        

#ifndef AK_OPTIMIZED
        // IAkStreamProfile interface.
        //----------------------------
        virtual void GetStreamData(
            AkStreamData & out_streamData
            ) override;

		// Signals that stream can be destroyed.
		virtual void ProfileAllowDestruction() override;
#endif

	protected:

		//-----------------------------------------------------------------------------
        // Helpers.
        //-----------------------------------------------------------------------------

		// Add up the amount of data that is ready to be consumed but has not been granted yet.
		AKRESULT CalcUnconsumedBufferSize(AkUInt32 & out_uNumBytesAvailable);

		//Release the buffers that are being held in a caching stream, up to in_uMemNeeded bytes. 
		//Return the amount of data that is freed.
		virtual AkUInt32 ReleaseCachingBuffers(AkUInt32 in_uMemNeeded) override;

		bool NeedsBuffering(
			AkUInt32 in_uVirtualBufferingSize
			);

		inline void SetRunning( bool in_bRunning )
		{
			m_bIsRunning = in_bRunning;
			SetReadyForIO( in_bRunning && !m_bHasReachedEof && !m_bIsToBeDestroyed );
		}

		inline void SetReachedEof( bool in_bHasReachedEof )
		{
			m_bHasReachedEof = in_bHasReachedEof;
			SetReadyForIO( m_bIsRunning && !in_bHasReachedEof && !m_bIsToBeDestroyed );
		}

		// Returns true if the writer thread is done feeding this stream with data.
		// NOTE: Since stream objects do not keep an explicit count of their actual buffering 
		// (that is, the amount of data which transfers have completed, which is the sum of 
		// uDataSize of all their AkStmBuffers), callers must compute it and pass it to this function. 
		// It is compared to the virtual buffering size, which represents the amount of data for 
		// transfers that have been _scheduled_ (including those that have completed).
		inline bool NeedsNoMoreTransfer( AkUInt32 in_uActualBufferingSize )
		{
			return !RequiresScheduling() && ( m_uVirtualBufferingSize <= in_uActualBufferingSize );
		}

		inline void GetPositionForNextTransfer( 
			AkUInt64 & out_uFilePosition,
			AkUInt32 & out_uRequestedSize,
			bool & out_bEof
			)
		{
			// Get position of last transfer request that was sent to Low-Level IO.
			out_uFilePosition = GetVirtualFilePosition();	

			// Handle loop buffering.
			if ( m_uLoopEnd 
				&& out_uFilePosition >= m_uLoopEnd )
			{
				// Read at the beginning of the loop region. Snap to Low-Level block size.
				out_uFilePosition = m_uLoopStart;
			}

			out_uRequestedSize = ClampRequestSizeToEof( out_uFilePosition, m_bufSettings.uBufferSize, out_bEof );
		}

		// Position management.
        void ForceFilePosition(
            const AkUInt64 in_uNewPosition		// New stream position.
            );

        // Scheduling status management.
        void UpdateSchedulingStatus();

        // Returns a buffer filled with data. NULL if no data is ready.
        void *   GetReadBuffer(     
            AkUInt32 &  out_uSize               // Buffer size.
            );
        // Releases the latest buffer granted to user. Returns AK_Fail if no buffer was granted.
        AKRESULT ReleaseReadBuffer();
        // Flushes all stream buffers that are not currently granted.
        void Flush();

		// Returns effective mem view size, taking looping heuristics into consideration.
		inline AkUInt32 GetEffectiveViewSize( 
			CAkStmMemView * in_pStmMemView 
			)
		{
			return ( in_pStmMemView->StartPosition() < m_uLoopEnd && in_pStmMemView->EndPosition() > m_uLoopEnd ) ?
				(AkUInt32)(m_uLoopEnd - in_pStmMemView->StartPosition()) : in_pStmMemView->Size();
		}

		// Get rid of a buffer after it was enqueued in the buffer (ready) list.
		// The virtual buffering size is corrected accordingly.
		// IMPORTANT: Always dequeue buffer from listbare before calling this function.
		// Sync: Caller must hold the device lock.
		// NOTE: One should call m_pDevice->NotifyMemChange() after a call or a series of call to 
		// this function, to notify the device that some memory may have been freed.
		inline void DestroyBuffer(
			CAkStmMemView * in_pMemView
			)
		{
			AKASSERT( m_uVirtualBufferingSize >= GetEffectiveViewSize( in_pMemView ) );
			m_uVirtualBufferingSize -= GetEffectiveViewSize( in_pMemView );
			m_pDevice->DestroyMemView( in_pMemView );
		}

		// Cache query:
		// Returns true if data for the next expected transfer was found either in the list or in cache.
		// Returns false if data is found neither in the list or cache.
		// If data is found in the list or cache, out_pBuffer is set and ready to use. 
		// Sync: Stream needs to be locked.
		bool GetBufferOrReserveCacheBlock( void *& out_pBuffer, AkUInt32 & out_uSize );

#ifdef _DEBUG
		virtual void CheckVirtualBufferingConsistency() = 0;
#define CHECK_BUFFERING_CONSISTENCY()	CheckVirtualBufferingConsistency()
#else
#define CHECK_BUFFERING_CONSISTENCY()
#endif
        
		// Stream type specific policies.
		// ------------------------------

		// Add a new streaming memory view (or content of a memory view) to this stream after a transfer.
		// If it ends up not being used, it is disposed of. Otherwise it's status is set to Ready.
		// All logical transfers must end up here, even if they were cancelled.
		// Sync: Status must be locked prior to calling this function. 
		void AddMemView( 
			CAkStmMemView * in_pMemView,		// Transfer-mode memory view to resolve.
			bool			in_bStoreData		// Store data in stream object only if true.
			);

		// Update task's status after transfer.
		void UpdateTaskStatus(
			AKRESULT	in_eIOResult			// AK_Success if IO was successful, AK_Cancelled if IO was cancelled, AK_Fail otherwise.
			);

		// Correct virtual buffering after cancel. Decrements it by mem view's size, and then clears it.
		inline void CorrectVirtualBufferingAfterCancel( 
			CAkStmMemView * io_pMemView 
			)
		{
			AKASSERT( m_uVirtualBufferingSize >= GetEffectiveViewSize( io_pMemView ) );
			m_uVirtualBufferingSize -= GetEffectiveViewSize( io_pMemView );
			io_pMemView->ClearSize();
		}

		inline AkReal32 GetThroughput() { AKASSERT(!IsCachingStream()); return m_fThroughput; }
		inline void SetThroughput(AkReal32 fThroughput) { AKASSERT(!IsCachingStream()); m_fThroughput= fThroughput; }

	private:
		// Helper: Set and check user buffering constraints against device granularity and low-level block size.
		AKRESULT AdjustBufferingSettings();
		AKRESULT PostOpenHelper();
		
        
    protected:

		AkUInt64			m_uNextExpectedUserPosition;	// Expected position of next GetBuffer(), relative to file start, as expected by the client, accurate to the byte. 
		AkUInt64			m_uNextExpectedFilePosition;	// Expected position of next GetBuffer(), relative to file start, rounded by blocksize.
        
		// File identifier for memory cache.
		AkCacheID			m_cacheID = AK_INVALID_CACHE_ID;

        // Stream heuristics.
		union
		{
			// This should only be assessed through the getters and setters
			AkReal32        m_fThroughput;      // Average throughput in bytes/ms. Only valid if IsCachingStream()  is false
			// The places that touch this should always check/assert that this is a caching stream first.
			AkUInt32        m_uCachingBufferSize;      // Size of this caching buffer. Only valid if IsCachingStream() is true.
		};
		AkUInt32			m_uMinimalBufferingSize; // Optional, Will make this a minimum target buffering size.
        AkUInt64            m_uLoopStart;       // Set to start of loop (byte offset from beginning of stream) for streams that loop, 0 otherwise.
        AkUInt64            m_uLoopEnd;         // Set to end of loop (byte offset from beginning of stream) for streams that loop, 0 otherwise.

        // Streaming buffers.

		AkAutoStmBufSettings m_bufSettings;
        AkUInt32            m_uVirtualBufferingSize;	// Virtual buffering size: sum of all buffered data and pending transfers, minus what is granted to client.
														// Used for fast scheduling (minimizes scheduler computation and locking).
		AkUInt32            m_uBufferAlignment;	// Effective streaming buffer alignment. It is the LCM of both the low-level block size and user block size.

		// Profiling.
#ifndef AK_OPTIMIZED
		AkUInt32            m_uBytesTransferedLowLevel; // Number of bytes transferred from low-level IO only (replace).
#endif

		typedef AkListBare<CAkStmMemView,AkListBareNextMemView,AkCountPolicyWithCount>	AkBufferList;
		AkBufferList		m_listBuffers;
		AkUInt8				m_uNextToGrant;     // Index of next buffer to grant (this implementation supports a maximum of 255 concurrently granted buffers).

		// Helper: get next buffer to grant to client.
		inline CAkStmMemView * GetNextBufferToGrant()
		{
			AKASSERT( m_listBuffers.Length() > m_uNextToGrant );
			AkUInt32 uIdx = 0;
			AkBufferList::Iterator it = m_listBuffers.Begin();
			while ( uIdx < m_uNextToGrant )
			{
				++uIdx;
				++it;
			}
			return *it;
		}

		AkUInt8				m_uMinNumBuffers;   // Specify a minimal number of buffers if you plan to own more than one buffer at a time, 0 or 1 otherwise.

        // Stream status.
        bool            	m_bIsRunning;		// Running or paused.
		bool           		m_bIOError;			// Stream encountered I/O error.
		bool				m_bCachingReady;	// Caching stream is fully constructed and started.
    };

	class CAkAutoStmMemory : public AK::IAkAutoStream
	{
	public:
		CAkAutoStmMemory(void * pBuffer, AkUInt64 sz);

		virtual ~CAkAutoStmMemory();

		virtual void Destroy() override;

		virtual void GetInfo(AkStreamInfo & out_info) override;

		virtual void * GetFileDescriptor() override;

		virtual void GetHeuristics(AkAutoStmHeuristics & out_heuristics) override;

		virtual AKRESULT SetHeuristics(const AkAutoStmHeuristics & in_heuristics) override;

		virtual AKRESULT  SetMinimalBufferSize(AkUInt32 in_uMinBufferSize) override;

		virtual AKRESULT  SetStreamName(const AkOSChar * in_pszStreamName) override;

		virtual AkUInt32  GetBlockSize() override;

		virtual AKRESULT QueryBufferingStatus( 
			AkUInt32 & out_uNumBytesAvailable
			) override;

		virtual AkUInt32 GetNominalBuffering() override;

		virtual AKRESULT Start() override;

		virtual AKRESULT Stop() override;

		virtual AkUInt64 GetPosition( 
			bool *          out_pbEndOfStream
			) override;  

		virtual AKRESULT SetPosition(
			AkInt64         in_iMoveOffset,
			AkMoveMethod    in_eMoveMethod
			) override;

		virtual AKRESULT GetBuffer(
			void *&         out_pBuffer,
			AkUInt32 &      out_uSize,
			bool            in_bWait
			) override;

		virtual AKRESULT ReleaseBuffer() override;

		AKRESULT Relocate(AkUInt8* in_pNewBuffer);

	private:
		AkAutoStmHeuristics m_Heuristics;
		AkUInt32 m_uMinBufferSize;
		AkUInt8* m_pBuffer;
		AkUInt64 m_uSize;
		AkUInt64 m_uPosition;
		AkUInt16 m_uNumBuffers;
		AkOSChar * m_pszStreamName;
	};
}
}
#endif //_AK_DEVICE_BASE_H_
