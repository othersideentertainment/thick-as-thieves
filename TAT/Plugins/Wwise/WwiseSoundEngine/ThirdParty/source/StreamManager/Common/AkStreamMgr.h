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
// AkStreamMgr.h
//
// Stream manager Windows-specific implementation:
// Device factory.
// Platform-specific scheduling strategy.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_STREAM_MGR_H_
#define _AK_STREAM_MGR_H_

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/Tools/Common/AkArray.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

#include <AK/Tools/Common/AkKeyArray.h>

#define AK_STM_OBJ_POOL_BLOCK_SIZE      (32)

namespace AK
{
namespace StreamMgr
{
	class CAkAutoStmBase;
	class CAkDeviceBase;
	class CAkStmTask;
	class CAkStmWrapper;
	
	void MonitorFileOpenError(AKRESULT in_eRes, AkAsyncFileOpenData& in_file);

	void OutputSearchedPaths(
		AKRESULT in_result,					///< Result of the open call
		AkAsyncFileOpenData& in_fileOpen,	///< File name and flags passed to the Open call.
		AkOSChar* out_searchedPath,			///< Pre-allocated string buffer to be filled with all searched paths searched for the file.
		AkInt32 in_pathSize					///< The maximum size of the string
	);

	bool UsingIOThread();
	void ResetUsingIOThread();
	void SetUsingIOThread(bool bUseIOThread);

	//-----------------------------------------------------------------------------
    // Name: class CAkStreamMgr
    // Desc: Implementation of the stream manager.
    //-----------------------------------------------------------------------------
	
    class CAkStreamMgr : public IAkStreamMgr
#ifndef AK_OPTIMIZED
                        ,public IAkStreamMgrProfile
#endif
    {
        // Public factory.
        friend IAkStreamMgr * Create( 
            const AkStreamMgrSettings &	in_settings		// Stream manager initialization settings.
            );

		// Default settings.
		void GetDefaultSettings(
			AkStreamMgrSettings &		out_settings
			);
		void GetDefaultDeviceSettings(
			AkDeviceSettings &			out_settings
			);

		// Public file location handler getter/setter.
		friend IAkFileLocationResolver * GetFileLocationResolver();
		
		friend void SetFileLocationResolver(
			IAkFileLocationResolver *	in_pFileLocationResolver	// File location resolver. Needed for Open().
			);

        // Device management.
        // Warning: This function is not thread safe.
        friend AKRESULT CreateDevice(
            const AkDeviceSettings &    in_settings,		// Device settings.
			IAkLowLevelIOHook *			in_pLowLevelHook,	// Device specific low-level I/O hook.
			AkDeviceID&					out_idDevice		// Assigned device ID.
            );
        // Warning: This function is not thread safe. No stream should exist for that device when it is destroyed.
        friend AKRESULT   DestroyDevice(
            AkDeviceID                  in_deviceID         // Device ID.
            );
		
		friend AKRESULT PerformIO();

		// Language management.
		friend AKRESULT SetCurrentLanguage(
			const AkOSChar *	in_pszLanguageName			// Language name.
			);

		friend AKRESULT AddLanguageChangeObserver(
			AkLanguageChangeHandler in_handler,				// Callback function.
			void * in_pCookie								// Cookie.
			);

		friend void RemoveLanguageChangeObserver(
			void * in_pCookie								// Cookie.
			);

		// Cache management.
		friend void FlushAllCaches();

		friend void OutputSearchedPaths(
			AKRESULT in_result,					///< Result of the open call
			AkAsyncFileOpenData& in_fileOpen,	///< File name and flags passed to the Open call.
			AkOSChar* out_searchedPath,			///< Pre-allocated string buffer to be filled with all searched paths searched for the file.
			AkInt32 in_pathSize					///< The maximum size of the string
		);

    public:

        virtual ~CAkStreamMgr();

		// Stream manager destruction.
        virtual void     Destroy();

		inline static IAkFileLocationResolver * GetFileLocationResolver()
		{
			return m_pFileLocationResolver;
		}

		// Global pool cleanup: dead streams.
		// Since the StreamMgr's global pool is shared across all devices, they all need to perform
		// dead handle clean up. The device that calls this method will also be asked to kill one of
		// its tasks.
		static void ForceCleanup( 
			CAkDeviceBase * in_pCallingDevice,		// Calling device: if specified, the task with the lowest priority for this device will be killed.
			AkPriority		in_priority				// Priority of the new task if applicable. Pass AK_MAX_PRIORITY to ignore.
			);

        // Stream creation interface.
        // ------------------------------------------------------
        
        // Standard stream create methods.
        // -----------------------------

        // String overload.
        virtual AKRESULT CreateStd(
			const AkFileOpenData& in_FileOpen,	    ///< File name or file ID (only one should be valid), open flags, open mode
            IAkStdStream *&     out_pStream,		// Returned interface to a standard stream.
			bool				in_bSyncOpen		// If true, force the Stream Manager to open file synchronously. Otherwise, it is left to its discretion.
            );

        
        // Automatic stream create methods.
        // ------------------------------

        // Note: Open does not start automatic streams. 
        // They need to be started explicitly with IAkAutoStream::Start().

        // String overload.
        virtual AKRESULT CreateAuto(
			const AkFileOpenData& in_FileOpen,				///< File name or file ID (only one should be valid), open flags, open mode
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            IAkAutoStream *&            out_pStream,		// Returned interface to an automatic stream.
			bool						in_bSyncOpen,		// If true, force the Stream Manager to open file synchronously. Otherwise, it is left to its discretion.
			bool						in_bCaching = false	// Does this stream stay in cache
            );
		// Memory overload
		virtual AKRESULT CreateAuto(
			void *						in_pBuffer,
			AkUInt64					in_uSize,
			const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
			IAkAutoStream *&            out_pStream			// Returned interface to an automatic stream.
			);


		// Cache pinning mechanism.
		// ------------------------------
		// Start streaming the first "in_pFSFlags->uNumBytesPrefetch" bytes of the file with id "in_fileID" into cache.  The stream will be scheduled only after
		// all regular streams (not file caching streams) are serviced.  The file will stay cached until either the UnpinFileInCache is called,
		// or the limit as set by uMaxCachePinnedBytes is reached and another higher priority file (in_uPriority) needs the space.  
		virtual AKRESULT PinFileInCache(
			AkFileID                    in_fileID,          // Application-defined ID
			AkFileSystemFlags *         in_pFSFlags,        // Special file system flags (can NOT pass NULL)
			AkPriority					in_uPriority		// Stream caching priority.  Note: Caching requests only get serviced after all regular streaming requests.
			);

		// Un-pin a file that has been previouly pinned into cache.  This function must be called once for every call to PinFileInCache() with the same file id.
		// The file may still remain in stream cache after this is called, until the memory is reused by the streaming memory manager in accordance with to its 
		// cache management algorithm.
		virtual AKRESULT UnpinFileInCache(
			AkFileID                    in_fileID,          	// Application-defined ID
			AkPriority					in_uPriority 		///< Priority of stream that you are un pinning
			);

		// Update the priority of the caching stream.  Higher priority streams will be serviced before lower priority caching streams, and will be more likely to stay in 
		// memory if the cache pin limit as set by "uMaxCachePinnedBytes" is reached.
		virtual AKRESULT UpdateCachingPriority(
			AkFileID                    in_fileID,			// Application-defined ID
			AkPriority					in_uPriority, 		// Priority
			AkPriority					in_uOldPriority 		// Priority
			);

		// Return information about a file that has been pinned into cache.
		// Retrieves the percentage of the requested buffer size that has been streamed in and stored into stream cache, and whether 
		// the cache pinned memory limit is preventing this buffer from filling.
		virtual AKRESULT GetBufferStatusForPinnedFile( 
			AkFileID in_fileID,								// Application-defined ID
			AkReal32& out_fPercentBuffered,					// Percentage of buffer full (out of 100)
			bool& out_bCacheFull							// Set to true if the rest of the buffer can not fit into the cache-pinned memory limit.
			);

		// Make a memory stream point to a new area in memory, otherwise keeping the exact same state.
		virtual AKRESULT RelocateMemoryStream(
			IAkAutoStream * in_pStream,                     //< The stream to relocate. Must be a stream created with the memory overload of CreateAutoStm.
			AkUInt8 * in_pNewStart                          //< The new area in memory to point to
		);

        // -----------------------------------------------

        // Profiling interface.
        // -----------------------------------------------

         // Profiling access. Returns NULL in AK_OPTIMIZED.
        virtual IAkStreamMgrProfile * GetStreamMgrProfile();

#ifndef AK_OPTIMIZED
        // Public profiling interface.
        // ---------------------------
        virtual AKRESULT StartMonitoring();
	    virtual void     StopMonitoring();

        // Devices enumeration.
        virtual AkUInt32 GetNumDevices();         // Returns number of devices.
        virtual IAkDeviceProfile * GetDeviceProfile( 
            AkUInt32 in_uDeviceIndex              // [0,numDevices[
            );

		static AkUInt32 GetNewStreamID();

		static AkAtomic32  m_iNextStreamID;
    #endif

		//Returns true if succeeded
		static bool AddTask(CAkStmTask* in_pTask, AkDeviceID in_deviceID);

    private:
		
		AKRESULT OpenStream(
			AkFileOpenData& in_FileOpenParams,
			IAkStdStream*& out_pStream,				// Returned interface to a standard stream.
			bool				in_bSyncOpen		// If true, force the Stream Manager to open file synchronously. Otherwise, it is left to its discretion.
		);

        // Device management.
        // -----------------------------------------------

        // Warning: This function is not thread safe.
		AKRESULT CreateDevice(
			const AkDeviceSettings& in_settings,		// Device settings.
			IAkLowLevelIOHook* in_pLowLevelHook,		// Device specific low-level I/O hook.
			AkDeviceID& out_idDevice					// Assigned device ID.
            );
        // Warning: This function is not thread safe. No stream should exist for that device when it is destroyed.
        AKRESULT   DestroyDevice(
            AkDeviceID                  in_deviceID         // Device ID.
            );

		AKRESULT PerformIO();

        // Get device by ID.
        inline CAkDeviceBase * GetDevice( 
            AkDeviceID  in_deviceID 
            )
        {
	        if ( (AkUInt32)in_deviceID >= m_arDevices.Length( ) )
	        {
	            AKASSERT( !"Invalid device ID" );
	            return NULL;
	        }
	        return m_arDevices[in_deviceID];
	    }

        // Singleton.
        CAkStreamMgr();
		CAkStreamMgr( CAkStreamMgr& );
        CAkStreamMgr & operator=( CAkStreamMgr& );

        // Initialise/Terminate.
        AKRESULT Init( 
            const AkStreamMgrSettings &	in_settings
            );
        void     Term();

        // Globals: pools and low-level IO interface.
        static IAkFileLocationResolver *m_pFileLocationResolver;// Low-level IO location handler.

        // Array of devices.
	public:
        // NOTE: Although ArrayStreamProfiles is used only inside CAkDeviceBase, the definition of its allocator policy
		// must be public in order to avoid private nested class access inside AkArray. 
		typedef AkArrayAllocatorNoAlign<AkMemID_Streaming> ArrayPoolLocal;
	private:
        typedef AkArray< CAkDeviceBase*, CAkDeviceBase*, ArrayPoolLocal > AkDeviceArray;
        static AkDeviceArray m_arDevices;

		//
		// Global language management.
		// -----------------------------------------------
		AKRESULT SetCurrentLanguage(
			const AkOSChar *	in_pszLanguageName			// Language name.
			);

		AKRESULT AddLanguageChangeObserver(
			AkLanguageChangeHandler in_handler,				// Callback function.
			void * in_pCookie								// Cookie.
			);

		void RemoveLanguageChangeObserver(
			void * in_pCookie								// Cookie.
			);

		struct LangChgObserver
		{
			AkLanguageChangeHandler handler;
			void *					pCookie;
		};
		typedef AkArray<LangChgObserver, LangChgObserver&, ArrayPoolLocal> ArrayLangChgObservers;
		static ArrayLangChgObservers	m_arLangChgObserver;

		// Cache management.
		void FlushAllCaches();

		struct CachedFileStreamData
		{
			CachedFileStreamData(): pStream(NULL), m_refCount(0) {}
			~CachedFileStreamData()
			{
				m_Priorities.Term();
			}

			CAkAutoStmBase* pStream;

			AkPriority GetPriority() 
			{
				AkPriority uMax = 0;
				for( PriorityArray::Iterator it = m_Priorities.Begin(); it != m_Priorities.End(); ++it )
					if ( *it > uMax ) uMax = *it; 
				return uMax;
			}

			bool AddRef( AkPriority in_prio )
			{
				m_refCount++;
				return ( m_Priorities.AddLast(in_prio) != NULL );
			}

			bool Release( AkPriority in_prio )
			{
				for( CachedFileStreamData::PriorityArray::Iterator it = m_Priorities.Begin(); it != m_Priorities.End(); ++it )
				{
					if ( *it == in_prio )
					{
						m_Priorities.EraseSwap(it);
						break;
					}
				}

				m_refCount--;
				return ( m_refCount == 0 );
			}

			bool UpdatePriority( AkPriority in_uNewPriority, AkPriority in_uOldPriority )
			{
				for( CachedFileStreamData::PriorityArray::Iterator it = m_Priorities.Begin(); it != m_Priorities.End(); ++it )
				{
					if ( *it == in_uOldPriority )
					{
						*it = in_uNewPriority; 
						return true;
					}
				}
				return false;
			}
		private:
			AkUInt32 m_refCount;
			typedef AkArray< AkPriority, AkPriority, AkHybridAllocator< sizeof( AkPriority ) > > PriorityArray;
			PriorityArray m_Priorities;
		};

		CAkAutoStmBase* GetPinnedStream(AkFileID in_fileID, CachedFileStreamData*& out_pData);
		typedef AkKeyDataPtrStruct< AkFileID, CachedFileStreamData > CachedFileStreamDataStruct;
		typedef CAkKeyArray< AkFileID, CachedFileStreamDataStruct, ArrayPoolLocal > CachedFileStreamDataMap;
		
		//NOTE: This map can be accessed by multiple threads.  Currently this is protected by an external lock in AkStreamCacheMgmnt.
		//	If access to this map is needed from outside the context of AkStreamCacheMgmnt, an additional lock will need to be added.
		CachedFileStreamDataMap m_cachedFileStreams;

#if defined(AK_SUPPORT_THREADS)
		static bool s_bUseIOThread;	// When false, no I/O thread is created for any device. Instead, the user is responsible for calling AK::StreamMgr::PerformIO() whenever I/O must be performed. Defaults to true on all platforms that support threads.
#endif
    };
}
}
#endif //_AK_STREAM_MGR_H_
