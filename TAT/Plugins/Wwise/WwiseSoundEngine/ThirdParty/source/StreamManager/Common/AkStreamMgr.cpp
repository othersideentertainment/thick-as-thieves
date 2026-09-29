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
// AkStreamMgr.cpp
//
// Stream manager Windows-specific implementation:
// Device factory.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkStreamMgr.h"
#include <AK/Tools/Common/AkMonitorError.h>
#include "AkStreamingDefaults.h"
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>

// Factory products.
#include "AkDeviceDeferredLinedUp.h"
#ifdef AK_SUPPORT_WCHAR
#include <wchar.h>
#endif //AK_SUPPORT_WCHAR
#include <stdio.h>
#include <AK/SoundEngine/Common/AkAtomic.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Declaration of the one and only global pointer to the stream manager.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AKSOUNDENGINE_API AK::IAkStreamMgr * AK::IAkStreamMgr::m_pStreamMgr = NULL;


//-----------------------------------------------------------------------
// Global variables.
//-----------------------------------------------------------------------
AK::StreamMgr::IAkFileLocationResolver * AK::StreamMgr::CAkStreamMgr::m_pFileLocationResolver = NULL;
AK::StreamMgr::CAkStreamMgr::AkDeviceArray AK::StreamMgr::CAkStreamMgr::m_arDevices;
#ifndef AK_OPTIMIZED
	AkAtomic32 AK::StreamMgr::CAkStreamMgr::m_iNextStreamID = 0;
#endif
AK::StreamMgr::CAkStreamMgr::ArrayLangChgObservers	AK::StreamMgr::CAkStreamMgr::m_arLangChgObserver;

namespace AK
{
	namespace StreamMgr
	{
		//
		// Language management.
		// ---------------------------------------------

		// The one and only language static buffer.
		static AkOSChar m_szCurrentLanguage[AK_MAX_LANGUAGE_NAME_SIZE];
	
		// Helpers.
		// ---------------------------------------------
		void MonitorFileOpenError(AKRESULT in_eRes, AkAsyncFileOpenData& in_file)
		{
#ifndef AK_OPTIMIZED
			AKASSERT(in_eRes != AK_Success);	//Why are you calling this?

			AkOSChar searchPaths[AK_MAX_PATH] = { 0 };
			AK::StreamMgr::OutputSearchedPaths(in_eRes, in_file, searchPaths, AK_MAX_PATH);

			const AkOSChar* fileName = in_file.pszFileName;
			AkOSChar fileID[25];
			if (!in_file.pszFileName)
			{
				AK_OSPRINTF(fileID, 25, AKTEXT("with file ID: %u"), in_file.fileID);
				fileName = fileID;
			}

			char* utf8FileName, * utf8Path;
			CONVERT_OSCHAR_TO_CHAR(fileName, utf8FileName);
			CONVERT_OSCHAR_TO_CHAR(searchPaths, utf8Path);

			AK::Monitor::ErrorCode eCode = AK::Monitor::ErrorCode_CannotOpenFile;
			switch (in_eRes)
			{
			case AK_FileNotFound:
				eCode = AK::Monitor::ErrorCode_FileNotFound; break;
			case AK_FilePermissionError:
				eCode = AK::Monitor::ErrorCode_FilePermissionError; break;
			case AK_InvalidFileSize:
				eCode = AK::Monitor::ErrorCode_InvalidFileSize; break;
			default:
				eCode = AK::Monitor::ErrorCode_CannotOpenFile; break;
			}

			AK::Monitor::PostCodeVarArg(eCode, AK::Monitor::ErrorLevel_Error, AK::Monitor::MsgContext(), utf8FileName, utf8Path);
#endif
		}

		void OutputSearchedPaths(
			AKRESULT in_result,
			AkAsyncFileOpenData& in_fileOpen,
			AkOSChar* out_searchedPath,
			AkInt32 in_pathSize
		)
		{
			if (AK::IAkStreamMgr::Get())
			{
				CAkStreamMgr* pStreamMgr = static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get());

				IAkFileLocationResolver* pResolver = AK::StreamMgr::GetFileLocationResolver();
				if (pResolver)
				{
					AkDeviceID deviceID = AK_INVALID_DEVICE_ID;
					auto res = pResolver->GetNextPreferredDevice(in_fileOpen, deviceID);
					if (res == AK_NotImplemented)
					{
						if (pStreamMgr->m_arDevices.Length() > 0)
						{
							auto* pHook = pStreamMgr->m_arDevices[0]->GetLowLevelHook();
							if (pHook)
								pHook->OutputSearchedPaths(in_result, in_fileOpen, out_searchedPath, in_pathSize);
						}
					}
					else
					{
						AkOSChar devicePaths[AK_MAX_PATH];

						while (res == AK_Success && deviceID != AK_INVALID_DEVICE_ID && deviceID < pStreamMgr->m_arDevices.Length())
						{
							auto* pHook = pStreamMgr->m_arDevices[deviceID]->GetLowLevelHook();
							if (pHook)
							{
								devicePaths[0] = 0;
								pHook->OutputSearchedPaths(in_result, in_fileOpen, devicePaths, in_pathSize);
								if (devicePaths[0] != 0)
								{
									if (out_searchedPath[0] != 0)
										AKPLATFORM::SafeStrCat(out_searchedPath, AKTEXT("; "), in_pathSize);
									AKPLATFORM::SafeStrCat(out_searchedPath, devicePaths, in_pathSize);
								}
							}
							res = pResolver->GetNextPreferredDevice(in_fileOpen, deviceID);
						}
					}
				}
			}
		}

#if defined(AK_SUPPORT_THREADS)
		bool g_bUseIOThread = true;
		bool UsingIOThread() { return g_bUseIOThread; }
		void ResetUsingIOThread() { g_bUseIOThread = true; }
		void SetUsingIOThread(bool bUseIOThread) { g_bUseIOThread = bUseIOThread; }
#else
		bool UsingIOThread() { return false; }
		void ResetUsingIOThread() {}
		void SetUsingIOThread(bool bUseIOThread) {}
#endif
	}
}


// ---------------------------------------------

AkAsyncFileOpenData::~AkAsyncFileOpenData()
{
	if (pszStreamName)
	{
		AkFree(AkMemID_Streaming, (void*)pszStreamName);
		pszStreamName = nullptr;
	}
}

AKRESULT AkAsyncFileOpenData::SetStreamName(const AkOSChar* in_pszStreamName)
{
	if (pszStreamName)
	{
		AkFree(AkMemID_Streaming, (void*)pszStreamName);
		pszStreamName = nullptr;
	}

	if (in_pszStreamName)
	{
		// Allocate string buffer for user defined stream name.
		size_t uStrLen = AKPLATFORM::OsStrLen(in_pszStreamName) + 1;

		pszStreamName = (AkOSChar*)AkAlloc(AkMemID_Streaming, uStrLen * sizeof(AkOSChar));
		if (!pszStreamName)
			return AK_InsufficientMemory;

		// Copy.
		AKPLATFORM::SafeStrCpy(pszStreamName, in_pszStreamName, uStrLen);
	}

	return AK_Success;
}
// ---------------------------------------------

//-----------------------------------------------------------------------------
// Factory.
//-----------------------------------------------------------------------------
AK::IAkStreamMgr * AK::StreamMgr::Create( 
	const AkStreamMgrSettings &	in_settings		// Stream manager initialization settings.
	)
{
	// Check memory manager.
	if ( !AK::MemoryMgr::IsInitialized() )
	{
		AKASSERT( !"Memory manager does not exist" );
		return NULL;
	}

	// Factory.
	AKASSERT( AK::IAkStreamMgr::Get() == NULL || !"CreateStreamMgr( ) should be called only once" );
	if ( AK::IAkStreamMgr::Get() == NULL )
	{
		// Clear current language.
		m_szCurrentLanguage[0] = 0;
		
		// Instantiate stream manager.
		CAkStreamMgr * pStreamMgr = AkNew( AkMemID_Streaming, CAkStreamMgr() );
		
		// Initialize.
		if ( pStreamMgr != NULL )
		{
			if ( pStreamMgr->Init( in_settings ) != AK_Success )
			{
				// Failed. Clean up.
				AKASSERT( !"Failed initializing stream manager" );
				pStreamMgr->Destroy();
				pStreamMgr = NULL;
			}
		}
	}
		
	AKASSERT( AK::IAkStreamMgr::Get() != NULL );
	return AK::IAkStreamMgr::Get();
}

void AK::StreamMgr::GetDefaultSettings(
	AkStreamMgrSettings &		out_settings
	)
{
}

void AK::StreamMgr::GetDefaultDeviceSettings(
	AkDeviceSettings &			out_settings
	)
{
	out_settings.pIOMemory				= NULL;
	out_settings.uIOMemorySize			= AK_DEFAULT_DEVICE_IO_POOL_SIZE;
	out_settings.uIOMemoryAlignment		= AK_REQUIRED_IO_POOL_ALIGNMENT;
	out_settings.ePoolAttributes		= AK_DEFAULT_BLOCK_ALLOCATION_TYPE;

	out_settings.uGranularity			= AK_DEFAULT_DEVICE_GRANULARITY;
	
	AKPLATFORM::AkGetDefaultThreadProperties( out_settings.threadProperties );
	
	// I/O thread uses a thread priority above normal.
	out_settings.threadProperties.nPriority	= AK_DEFAULT_DEVICE_THREAD_PRIORITY;

	out_settings.fTargetAutoStmBufferLength = AK_DEFAULT_DEVICE_BUFFERING_LENGTH;
	out_settings.uMaxConcurrentIO			= AK_DEFAULT_MAX_CONCURRENT_IO;

	out_settings.bUseStreamCache			= AK_DEFAULT_DEVICE_CACHE_ENABLED;

	out_settings.uMaxCachePinnedBytes	= (AkUInt32)-1; //Unlimited;
}

AK::StreamMgr::IAkFileLocationResolver * AK::StreamMgr::GetFileLocationResolver()
{
	AKASSERT( AK::IAkStreamMgr::Get() 
			|| !"Trying to get file location resolver before StreamManager was created" );
	return CAkStreamMgr::m_pFileLocationResolver;
}

void AK::StreamMgr::SetFileLocationResolver(
	AK::StreamMgr::IAkFileLocationResolver *	in_pFileLocationResolver
	)
{
	AKASSERT( AK::IAkStreamMgr::Get()
			|| !"Trying to set file location handler before StreamManager was created" );
	CAkStreamMgr::m_pFileLocationResolver = in_pFileLocationResolver;
}

// Device creation.
AKRESULT AK::StreamMgr::CreateDevice(
	const AkDeviceSettings &	in_settings,		// Device settings.
	IAkLowLevelIOHook *			in_pLowLevelHook,	// Device specific low-level I/O hook.
	AkDeviceID &				out_idDevice		// Assigned Id.
	)
{
	return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->CreateDevice( in_settings, in_pLowLevelHook, out_idDevice);
}
AKRESULT AK::StreamMgr::DestroyDevice(
	AkDeviceID                  in_deviceID         // Device ID.
	)
{
	return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->DestroyDevice( in_deviceID );
}

AKRESULT AK::StreamMgr::PerformIO()
{
	return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->PerformIO();
}

// Language management.
AKRESULT AK::StreamMgr::SetCurrentLanguage(
	const AkOSChar *	in_pszLanguageName			///< Language name.
	)
{
	return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->SetCurrentLanguage( in_pszLanguageName );
}

// Get current language. See AkStreamMgrModule.h.
const AkOSChar * AK::StreamMgr::GetCurrentLanguage()
{
	return m_szCurrentLanguage;
}

AKRESULT AK::StreamMgr::AddLanguageChangeObserver(
	AkLanguageChangeHandler in_handler,	///< Callback function.
	void * in_pCookie					///< Cookie, passed back to AkLanguageChangeHandler.
	)
{
	if (AK::IAkStreamMgr::Get())
		return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->AddLanguageChangeObserver( in_handler, in_pCookie );
	return AK_Success;
}

void AK::StreamMgr::RemoveLanguageChangeObserver(
	void * in_pCookie					///< Cookie, passed back to AkLanguageChangeHandler.
	)
{
	if (AK::IAkStreamMgr::Get())
		static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->RemoveLanguageChangeObserver( in_pCookie );
}

// Cache management.
void AK::StreamMgr::FlushAllCaches()
{
	static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get())->FlushAllCaches();
}


using namespace AK::StreamMgr;



//--------------------------------------------------------------------
// class CAkStreamMgr
//--------------------------------------------------------------------

void CAkStreamMgr::Destroy()
{
	Term();
	AkDelete( AkMemID_Streaming, this );
}

CAkStreamMgr::CAkStreamMgr()
{
	// Assign global pointer.
	m_pStreamMgr = this;
}

CAkStreamMgr::~CAkStreamMgr()
{
	for (CachedFileStreamDataMap::Iterator it = m_cachedFileStreams.Begin(); it !=m_cachedFileStreams.End(); ++it)
	{
		CachedFileStreamDataStruct& stct = (*it).item;
		AKASSERT (stct.pData && stct.pData->pStream);
		stct.pData->pStream->Destroy();
		stct.pData->pStream = NULL;
		stct.FreeData();
	}
	m_cachedFileStreams.Term();

	// Reset global pointer.
	m_pStreamMgr = NULL;
}


// Initialise/Terminate.
//-------------------------------------
AKRESULT CAkStreamMgr::Init(
	const AkStreamMgrSettings & in_settings
	)
{
	AK_MONITOR_STREAM_MGR_INIT(in_settings);
	return AK_Success;
}

void CAkStreamMgr::Term()
{
	// Destroy devices remaining.
	AkDeviceArray::Iterator it = m_arDevices.Begin();
	while ( it != m_arDevices.End() )
	{
		if ( (*it) )
			(*it)->Destroy();
		++it;
	}
	CAkStreamMgr::m_pFileLocationResolver = NULL;
	m_arDevices.Term();

	AK_MONITOR_STREAM_MGR_TERM();

	CAkStreamMgr::m_arLangChgObserver.Term();
}

//-----------------------------------------------------------------------------
// Device management.
// Warning: These functions are not thread safe.
//-----------------------------------------------------------------------------
// Device creation.
AKRESULT CAkStreamMgr::CreateDevice(
	const AkDeviceSettings &	in_settings,		// Device settings.
	IAkLowLevelIOHook *			in_pLowLevelHook,	// Device specific low-level I/O hook.
	AkDeviceID&					out_idDevice		// Assigned Id.
	)
{
	out_idDevice = AK_INVALID_DEVICE_ID;

	// Find first available slot.
	for ( AkUInt32 uSlot = 0; uSlot < m_arDevices.Length(); ++uSlot )
	{
		if ( !m_arDevices[uSlot] )
		{
			out_idDevice = uSlot;
			break;
		}
	}

	if ( AK_INVALID_DEVICE_ID == out_idDevice)
	{
		// Create slot.
		if ( !m_arDevices.AddLast( NULL ) )
		{
			AKASSERT( !"Could not add new device to list" );
			return AK_InsufficientMemory;
		}
		out_idDevice = (AkDeviceID)m_arDevices.Length() - 1;
		m_arDevices.Last() = NULL;
	}

	CAkDeviceBase * pNewDevice = NULL;
	AKRESULT eResult = AK_Fail;

	// Device factory.
	pNewDevice = AkNew( AkMemID_Streaming, CAkDeviceDeferredLinedUp( in_pLowLevelHook ) );
	if ( pNewDevice != NULL )
	{
		eResult = pNewDevice->Init( in_settings, out_idDevice, UsingIOThread() );
	}
	AKASSERT( eResult == AK_Success || !"Cannot initialize IO device" );

	if ( AK_Success == eResult )
		m_arDevices[out_idDevice] = pNewDevice;
	else
	{
		// Handle failure. At this point we have a valid device ID (index in array).
		if ( pNewDevice )
			pNewDevice->Destroy();

		out_idDevice = AK_INVALID_DEVICE_ID;
	}
	return eResult;
}

// Warning: This function is not thread safe. No stream should exist for that device when it is destroyed.
AKRESULT   CAkStreamMgr::DestroyDevice(
	AkDeviceID          in_deviceID         // Device ID.
	)
{
	if ( (AkUInt32)in_deviceID >= m_arDevices.Length() 
		|| !m_arDevices[in_deviceID] )
	{
		return AK_InvalidParameter;
	}

	CachedFileStreamDataMap::Iterator it = m_cachedFileStreams.Begin();
	while(it != m_cachedFileStreams.End())
	{
		CachedFileStreamDataStruct& stct = (*it).item;
		AKASSERT(stct.pData && stct.pData->pStream);
		AkStreamInfo info;
		stct.pData->pStream->GetInfo(info);
		if (info.deviceID == in_deviceID)
		{
			stct.pData->pStream->Destroy();
			stct.pData->pStream = NULL;
			stct.FreeData();
			it = m_cachedFileStreams.Erase(it);
		}
		else
			++it;
	}

	m_arDevices[in_deviceID]->Destroy();
	m_arDevices[in_deviceID] = NULL;

	return AK_Success;
}

AKRESULT CAkStreamMgr::PerformIO()
{
	if (AK_EXPECT_FALSE(UsingIOThread()))
	{
		return AK_InvalidState; // This should only be called when I/O threads are disabled
	}
	else
	{
		for (AkUInt32 uDevice = 0; uDevice < m_arDevices.Length(); uDevice++)
		{
			if (m_arDevices[uDevice])
			{
				m_arDevices[uDevice]->PerformIO();
			}
		}
		return AK_Success;
	}
}

// Global pool cleanup: dead streams.
// Since the StreamMgr's global pool is shared across all devices, they all need to perform
// dead handle clean up. The device that calls this method will also be asked to kill one of
// its tasks.
void CAkStreamMgr::ForceCleanup(
	CAkDeviceBase * in_pCallingDevice,		// Calling device: if specified, the task with the lowest priority for this device will be killed.
	AkPriority in_priority					// Priority of the new task if applicable. Pass AK_MAX_PRIORITY to ignore.
	)
{
	for ( AkUInt32 uDevice = 0; uDevice < m_arDevices.Length(); uDevice++ )
	{
		if ( m_arDevices[uDevice] )
			m_arDevices[uDevice]->ForceCleanup( in_pCallingDevice == m_arDevices[uDevice], in_priority );
	}
}

bool CAkStreamMgr::AddTask(CAkStmTask* in_pTask, AkDeviceID in_deviceID)
{
	if ((AkUInt32)in_deviceID >= ((CAkStreamMgr*)m_pStreamMgr)->m_arDevices.Length())
	{
		AKASSERT(!"Invalid device ID");
		return false;
	}

	//Should be detatched already
	AKASSERT(in_pTask->GetDevice() == NULL);
	AKASSERT(in_pTask->pNextLightItem == NULL);
	
	CAkDeviceBase* pDevice = ((CAkStreamMgr*)m_pStreamMgr)->m_arDevices[in_deviceID];
	pDevice->AddTask(in_pTask);
	return true;
}


// Stream creation interface.
// ------------------------------------------------------


// Standard stream open methods.
// -----------------------------

AKRESULT CAkStreamMgr::CreateStd(
	const AkFileOpenData& in_FileOpen,		///< File name or file ID (only one should be valid), open flags, open mode
	IAkStdStream*& out_pStream,		// Returned interface to a standard stream.
	bool				in_bSyncOpen		// If true, force the Stream Manager to open file synchronously. Otherwise, it is left to its discretion.
)
{
	out_pStream = nullptr;

	// Check parameters.
	if (!in_FileOpen.IsValid())
	{
		AKASSERT(!"Invalid file name");
		return AK_InvalidParameter;
	}

	CAkStdStmDeferredLinedUp* pNewStm = AkNew(AkMemID_Streaming, CAkStdStmDeferredLinedUp());
	if (!pNewStm)
		return AK_InsufficientMemory;

	AKRESULT res = pNewStm->Open(in_FileOpen, in_bSyncOpen);
	if (res != AK_Success)
	{
		pNewStm->Destroy();
		return res;
	}

	out_pStream = pNewStm;
	return AK_Success;
}


// Automatic stream open methods.
// ------------------------------

// String overload.
AKRESULT CAkStreamMgr::CreateAuto(
	const AkFileOpenData& in_FileOpen,				// File name or file ID (only one should be valid), open flags, open mode
	const AkAutoStmHeuristics& in_heuristics,      // Streaming heuristics.
	AkAutoStmBufSettings* in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
	IAkAutoStream*& out_pStream,		// Returned interface to an automatic stream.
	bool						in_bSyncOpen,		// If true, force the Stream Manager to open file synchronously. Otherwise, it is left to its discretion.
	bool						in_bCaching			// Does this stream stay in cache
)
{
	out_pStream = nullptr;
	
	CAkAutoStmDeferredLinedUp* pNewStm = AkNew(AkMemID_Streaming, CAkAutoStmDeferredLinedUp());
	if (!pNewStm)
		return AK_InsufficientMemory;

	pNewStm->Init(in_heuristics, in_pBufferSettings);

	if (in_bCaching)
		pNewStm->SetIsCachingStream();

	AKRESULT res = pNewStm->Open(in_FileOpen, in_bSyncOpen);
	if (res != AK_Success)
	{
		pNewStm->Destroy();
		return res;
	}

	out_pStream = pNewStm;
	return AK_Success;
}

AKRESULT AK::StreamMgr::CAkStreamMgr::CreateAuto(
	void * in_pBuffer,
	AkUInt64 in_uSize,
	const AkAutoStmHeuristics & in_heuristics,
	IAkAutoStream *& out_pStream)
{
	// Check parameters.
	if ( in_heuristics.fThroughput < 0 ||
		in_heuristics.priority < AK_MIN_PRIORITY ||
		in_heuristics.priority > AK_MAX_PRIORITY )
	{
		AKASSERT( !"Invalid automatic stream heuristic" );
		return AK_InvalidParameter;
	}

	AKRESULT eResult;
	IAkAutoStream * pStream = AkNew(AkMemID_Streaming, CAkAutoStmMemory(in_pBuffer, in_uSize));
	
	if ( pStream )
	{
		eResult = AK_Success;
		pStream->SetHeuristics(in_heuristics);
	}
	else
	{
		eResult = AK_InsufficientMemory;
	}
	
	out_pStream = pStream;
	return eResult;
}

namespace AK
{
	namespace OfflineRendering
	{
		extern bool g_bOfflineRenderingEnabled;
	}
}

inline static bool ShouldUseAsyncIO()
{
#if defined(AK_SUPPORT_THREADS)
	return AK::OfflineRendering::g_bOfflineRenderingEnabled;
#else
	return false;
#endif
}

// Cache pinning mechanism.
// ------------------------------

AK::StreamMgr::CAkAutoStmBase* CAkStreamMgr::GetPinnedStream(AkFileID in_fileID, CachedFileStreamData *& out_pData)
{	
	CachedFileStreamDataStruct* pStruct = m_cachedFileStreams.Exists(in_fileID);
	if (pStruct)
	{
		out_pData = pStruct->pData;
		return pStruct->pData->pStream;
	}
	out_pData = nullptr;
	return nullptr;
}

AKRESULT CAkStreamMgr::PinFileInCache(
	AkFileID                    in_fileID,          // Application defined ID.
	AkFileSystemFlags* in_pFSFlags,        // Special file system flags. Can pass NULL.
	AkPriority					in_uPriority
)
{
	AKRESULT res = AK_Fail;

	CachedFileStreamData* pData;
	CAkAutoStmBase* pStream = GetPinnedStream(in_fileID, pData);

	if (pStream)
	{
		if (pData->AddRef(in_uPriority))
		{
			pStream->GetDevice()->UpdateCachingPriority(pStream, in_uPriority);
			res = AK_Success;
		}
	}
	else
	{
		CachedFileStreamDataStruct* pStruct = m_cachedFileStreams.Set(in_fileID);
		if (pStruct && pStruct->AllocData())
		{
			CachedFileStreamData* pData = pStruct->pData;

			AkAutoStmHeuristics heuristics = {};
			heuristics.priority = in_uPriority;
			IAkAutoStream* pIAkStream = nullptr;
			res = CreateAuto(AkFileOpenData(in_fileID, AK_OpenModeRead, in_pFSFlags), heuristics, NULL, pIAkStream, ShouldUseAsyncIO(), true);
			if (pIAkStream)
			{
				pData->pStream = static_cast<CAkAutoStmBase*>(pIAkStream);
				
				const unsigned long MAX_NUMBER_STR_SIZE = 11;
				AkOSChar szName[MAX_NUMBER_STR_SIZE];
				AK_OSPRINTF(szName, MAX_NUMBER_STR_SIZE, AKTEXT("%u"), in_fileID);
				pData->pStream->SetStreamName(szName);
				res = pData->pStream->Start();
				if (res != AK_Success && res != AK_FormatNotReady)
				{
					pData->pStream->Destroy();
					pData->pStream = nullptr;
					res = AK_Fail;
				}
				else
				{
					pData->AddRef(in_uPriority);
					res = AK_Success;	//Change AK_FormatNotReady, it is ok to have that at this point. It will become ready later.
				}
			}

			if (res != AK_Success)
			{
				//Failed to create the stream.
				pStruct->FreeData();
				m_cachedFileStreams.Unset(in_fileID);
				pData = NULL;
				pStruct = NULL;
				res = AK_Fail;
			}
		}
		else
		{
			//Failed to allocate data
			m_cachedFileStreams.Unset(in_fileID);
			pStruct = NULL;
			res = AK_Fail;
		}
	}
	return res;
}

AKRESULT CAkStreamMgr::UnpinFileInCache(
	AkFileID                    in_fileID,          // Application defined ID.
	AkPriority					in_uPriority
	)
{
	AKRESULT res = AK_Fail;

	CachedFileStreamDataStruct* pStruct = m_cachedFileStreams.Exists( in_fileID );
	if (pStruct)
	{
		CachedFileStreamData* pData = pStruct->pData;
		if ( pData->Release(in_uPriority) )
		{
			if ( pData->pStream != NULL )
			{
				pData->pStream->Destroy();
				pData->pStream = NULL;
			}
			pStruct->FreeData();
			m_cachedFileStreams.Unset(in_fileID);
			pStruct = NULL;
			res = AK_Success;
		}
		else
		{
			pData->pStream->GetDevice()->UpdateCachingPriority(pData->pStream, in_uPriority);
		}
	}

	return res;
}

AKRESULT CAkStreamMgr::UpdateCachingPriority(
										AkFileID                    in_fileID,          // Application defined ID.
										AkPriority					in_uPriority,
										AkPriority					in_uOldPriority
										)
{
	CachedFileStreamData* pData;
	CAkAutoStmBase* pStream = GetPinnedStream(in_fileID, pData);
	if (pData && pData->UpdatePriority(in_uPriority, in_uOldPriority) )
	{		
		pStream->GetDevice()->UpdateCachingPriority(pStream, pData->GetPriority());
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT CAkStreamMgr::GetBufferStatusForPinnedFile( AkFileID in_fileID , AkReal32& out_fPercentBuffered, bool& out_bCacheFull )
{
	out_fPercentBuffered = 0.f;
	out_bCacheFull = false;

	AKRESULT res = AK_Fail;

	CachedFileStreamData* pData;
	CAkAutoStmBase* pStream = GetPinnedStream(in_fileID, pData);
	if (pData == nullptr)
		return AK_Fail;

	AkUInt32 uBufferedBytes = 0;
	res = pData->pStream->QueryBufferingStatus(uBufferedBytes);
	if (res != AK_Fail)
	{
		AkUInt32 uCachingBufferSize = pStream->GetNominalBuffering();
		if (uCachingBufferSize > 0)
		{
			AkStreamInfo stmInfo;
			pStream->GetInfo(stmInfo);
			if (stmInfo.uSize != 0)
				uCachingBufferSize = (AkUInt32)AkMin(uCachingBufferSize, stmInfo.uSize);

			out_fPercentBuffered = ((AkReal32)uBufferedBytes / (AkReal32)uCachingBufferSize) * 100.f;

			AkFileDesc *pFileDesc = (AkFileDesc*)pStream->GetFileDescriptor();
			if (pFileDesc)
			{
				AkUInt32 uRemainingBytes = m_arDevices[pFileDesc->deviceID]->RemainingCachePinnedBytes();

				out_bCacheFull = (uBufferedBytes < uCachingBufferSize) &&
					((uCachingBufferSize - uBufferedBytes) > uRemainingBytes);
			}
			else
				res = AK_Fail;
		}
	}

	return res;
}

AKRESULT AK::StreamMgr::CAkStreamMgr::RelocateMemoryStream(IAkAutoStream * in_pStream, AkUInt8 * in_pNewStart)
{
	return static_cast<CAkAutoStmMemory*>(in_pStream)->Relocate(in_pNewStart);
}

// Profiling interface.
// --------------------------------------------------------------------
IAkStreamMgrProfile * CAkStreamMgr::GetStreamMgrProfile()
{
#ifndef AK_OPTIMIZED
	return this;
#else
	return NULL;
#endif
}

#ifndef AK_OPTIMIZED
// Device enumeration.
AkUInt32 CAkStreamMgr::GetNumDevices()
{
	// Find real number of stream devices.
	AkUInt32 uNumDevices = 0;
	for ( AkUInt32 uSlot = 0; uSlot < m_arDevices.Length(); ++uSlot )
	{
		if ( m_arDevices[uSlot] )
			++uNumDevices;
	}

	return uNumDevices;
}

IAkDeviceProfile * CAkStreamMgr::GetDeviceProfile( 
	AkUInt32 in_uDeviceIndex    // [0,numStreams[
	)
{
	if ( in_uDeviceIndex >= m_arDevices.Length() )
	{
		AKASSERT( !"Invalid device index" );
		return NULL;
	}
	
	// Convert device index to device ID.
	for ( AkUInt32 uDeviceID = 0; uDeviceID < m_arDevices.Length(); ++uDeviceID )
	{
		if ( !m_arDevices[uDeviceID] )
			++in_uDeviceIndex;	// NULL. Skip.
		else if ( in_uDeviceIndex == uDeviceID )
			return m_arDevices[uDeviceID];
	}

	AKASSERT( !"Invalid device index" );
	return NULL;
}

AkUInt32 CAkStreamMgr::GetNewStreamID()
{
	return AkAtomicInc32(&m_iNextStreamID);
}

AKRESULT CAkStreamMgr::StartMonitoring()
{
	for ( AkUInt32 u=0; u<m_arDevices.Length( ); u++ )
	{
		if ( m_arDevices[u] )
		{
			AKVERIFY( m_arDevices[u]->StartMonitoring( ) == AK_Success );
		}
	}
	return AK_Success;
}

void CAkStreamMgr::StopMonitoring()
{
	for ( AkUInt32 u=0; u<m_arDevices.Length( ); u++ )
	{
		if ( m_arDevices[u] )
			m_arDevices[u]->StopMonitoring( );
	}
}

#endif

// Language management.
AKRESULT CAkStreamMgr::SetCurrentLanguage(
	const AkOSChar *	in_pszLanguageName
	)
{
	if ( !in_pszLanguageName )
	{
		AKASSERT( !"Invalid language string" );
		return AK_Fail;
	}
	size_t uStrLen = AKPLATFORM::OsStrLen( in_pszLanguageName );
	if ( uStrLen >= AK_MAX_LANGUAGE_NAME_SIZE 
		|| ( uStrLen > 0 && ( in_pszLanguageName[uStrLen-1] == '/' || in_pszLanguageName[uStrLen-1] == '\\' ) ) )
	{
		AKASSERT( !"Invalid language name" );
		return AK_Fail;
	}
	AKPLATFORM::SafeStrCpy( m_szCurrentLanguage, in_pszLanguageName, AK_MAX_LANGUAGE_NAME_SIZE );

	// Notify observers, from last to first (to let them unregister from callback).
	AkUInt32 uCallback = m_arLangChgObserver.Length();
	while ( uCallback > 0 )
	{
		LangChgObserver & rCallbackData = m_arLangChgObserver[uCallback-1];
		rCallbackData.handler( m_szCurrentLanguage, rCallbackData.pCookie );
		--uCallback;
	}

	return AK_Success;
}

AKRESULT CAkStreamMgr::AddLanguageChangeObserver(
	AkLanguageChangeHandler in_handler,	
	void * in_pCookie					
	)
{
	LangChgObserver * pNewObserver = m_arLangChgObserver.AddLast();
	if ( pNewObserver )
	{
		pNewObserver->handler = in_handler;
		pNewObserver->pCookie = in_pCookie;
		return AK_Success;
	}
	return AK_Fail;
}

void CAkStreamMgr::RemoveLanguageChangeObserver(
	void * in_pCookie					
	)
{
	ArrayLangChgObservers::Iterator it = m_arLangChgObserver.Begin();
	while ( it != m_arLangChgObserver.End() )
	{
		if ( (*it).pCookie == in_pCookie )
			it = m_arLangChgObserver.Erase( it );
		else
			++it;
	}
}

void CAkStreamMgr::FlushAllCaches()
{
	for ( AkUInt32 uDevice = 0; uDevice < m_arDevices.Length(); uDevice++ )
	{
		if ( m_arDevices[uDevice] )
			m_arDevices[uDevice]->FlushCache();
	}
}
