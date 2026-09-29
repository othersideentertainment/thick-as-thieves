/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the 
"Apache License"); you may not use this file except in compliance with the 
Apache License. You may obtain a copy of the Apache License at 
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#ifndef _AK_PLATFORM_FUNCS_H_
#define _AK_PLATFORM_FUNCS_H_

#include <AK/Tools/Common/AkAssert.h>
#include <sceerror.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <kernel/eventflag.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ajm.h>

//-----------------------------------------------------------------------------
// Platform-specific thread properties definition.
//-----------------------------------------------------------------------------
struct AkThreadProperties
{
    int						nPriority;		///< Thread priority
    SceKernelCpumask		dwAffinityMask;	///< Affinity mask
	size_t					uStackSize;		///< Thread stack size
	int						uSchedPolicy;	///< Thread scheduling policy
};

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
// These variables are declared and updated by the sound engine.
namespace AK
{
	// used by time helpers to return time values in milliseconds.
    extern AkReal32 g_fFreqRatio;
}

//-----------------------------------------------------------------------------
// Defines for PS5.
//-----------------------------------------------------------------------------
#define AK_DECLARE_THREAD_ROUTINE( FuncName )   void* FuncName(void* lpParameter)
#define AK_THREAD_RETURN( _param_ )				return (_param_);
#define AK_THREAD_ROUTINE_PARAMETER             lpParameter
#define AK_GET_THREAD_ROUTINE_PARAMETER_PTR(type) reinterpret_cast<type*>( AK_THREAD_ROUTINE_PARAMETER )

#define AK_RETURN_THREAD_OK                     0x00000000
#define AK_RETURN_THREAD_ERROR                  0x00000001
#define AK_DEFAULT_STACK_SIZE                   (128*1024)
#define AK_THREAD_DEFAULT_SCHED_POLICY			SCE_KERNEL_SCHED_FIFO
#define AK_THREAD_PRIORITY_NORMAL				SCE_KERNEL_PRIO_FIFO_DEFAULT
#define AK_THREAD_PRIORITY_ABOVE_NORMAL			SCE_KERNEL_PRIO_FIFO_HIGHEST
#define AK_THREAD_PRIORITY_BELOW_NORMAL			SCE_KERNEL_PRIO_FIFO_LOWEST

#define AK_THREAD_AFFINITY_ALL					8191 // from 0b1'1111'1111'1111 -- 13 cores available
#define	AK_THREAD_AFFINITY_DEFAULT				4095 // from 0b0'1111'1111'1111 -- Default to only the 12 fully-available cores. 13th core is half-available.

// NULL objects
#define AK_NULL_THREAD                          NULL

#define AK_INFINITE                             (AK_UINT_MAX)

#define AkMax(x1, x2)	(((x1) > (x2))? (x1): (x2))
#define AkMin(x1, x2)	(((x1) < (x2))? (x1): (x2))
#define AkClamp(x, min, max)  ((x) < (min)) ? (min) : (((x) > (max) ? (max) : (x)))

namespace AKPLATFORM
{
#ifndef AK_OPTIMIZED
	/// Output a debug message on the console (Ansi string)
	AkForceInline void OutputDebugMsg( const char* in_pszMsg )
	{
		fputs( in_pszMsg, stderr );
	}
	/// Output a debug message on the console (Unicode string)
	AkForceInline void OutputDebugMsg( const wchar_t* in_pszMsg )
	{
		fputws( in_pszMsg, stderr );
	}

	/// Output a debug message on the console (Unicode string) (variadic function)
	template <int MaxSize = 0> // Unused
	AkForceInline void OutputDebugMsgV( const wchar_t* in_pszFmt, ... )
	{
		va_list args;
		va_start(args, in_pszFmt);
		vfwprintf(stderr, in_pszFmt, args);
		va_end(args);
	}

	/// Output a debug message on the console (Ansi string) (variadic function)
	template <int MaxSize = 0> // Unused
	AkForceInline void OutputDebugMsgV( const char* in_pszFmt, ... )
	{
		va_list args;
		va_start(args, in_pszFmt);
		vfprintf(stderr, in_pszFmt, args);
		va_end(args);
	}
#else
	inline void OutputDebugMsg( const wchar_t* ){}
	inline void OutputDebugMsg( const char* ){}

	template <int MaxSize = 0> // Unused
	inline void OutputDebugMsgV( const wchar_t*, ... ){}

	template <int MaxSize = 0> // Unused
	inline void OutputDebugMsgV( const char*, ... ){}
#endif


	// Simple automatic event API
    // ------------------------------------------------------------------
	
	/// Platform Independent Helper
	AkForceInline void AkClearEvent( AkEvent & out_event )
    {		
		out_event = NULL;
	}

	AkForceInline AKRESULT AkCreateNamedEvent( AkEvent & out_event, const char* in_szName )
    {
		// NOTE: AkWaitForEvent uses the SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT flag
		// to get the same behavior as an auto-reset Win32 event
        int ret = sceKernelCreateEventFlag(
			&out_event,
			in_szName,
			SCE_KERNEL_EVF_ATTR_MULTI,
			0 /* not signalled by default */,
			NULL /* No optional params */ );

		if( ret == SCE_OK )
			return AK_Success;

		AkClearEvent( out_event );
		return AK_Fail;
	}

	/// Platform Independent Helper
	AkForceInline AKRESULT AkCreateEvent( AkEvent & out_event )
    {
		return AkCreateNamedEvent( out_event, "AkEvent" );
	}

	/// Platform Independent Helper
	AkForceInline void AkDestroyEvent( AkEvent & io_event )
	{
		sceKernelDeleteEventFlag(io_event);
		AkClearEvent( io_event );
	}

	/// Platform Independent Helper
	AkForceInline void AkWaitForEvent( AkEvent & in_event )
	{
		AKVERIFY( sceKernelWaitEventFlag(
			in_event,
			1,
			SCE_KERNEL_EVF_WAITMODE_OR | SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL,
			SCE_NULL,
			SCE_NULL) == 0 );
	}

	/// Platform Independent Helper
	AkForceInline void AkSignalEvent( const AkEvent & in_event )
	{
		AKVERIFY( sceKernelSetEventFlag( in_event, 1 ) == 0 );
	}

	/// Platform Independent Helper
	AkForceInline void AkClearSemaphore(AkSemaphore& io_semaphore)
	{
		io_semaphore = NULL;
	}

	/// Platform Independent Helper
	inline AKRESULT AkCreateSemaphore( AkSemaphore& out_semaphore, AkUInt32 in_initialCount )
	{
		int ret = sceKernelCreateSema(	
							&out_semaphore,
							"AkSemaphore",
							0,
							in_initialCount,
							INT_MAX,
							NULL );
		
		return ( ret == SCE_OK ) ? AK_Success : AK_Fail;
	}

	/// Platform Independent Helper
	inline void AkDestroySemaphore(AkSemaphore& io_semaphore)
	{
		AKVERIFY(sceKernelDeleteSema(io_semaphore) == SCE_OK);
	}

	/// Platform Independent Helper - Semaphore wait, aka Operation P. Decrements value of semaphore, and, if the semaphore would be less than 0, waits for the semaphore to be released.
	inline void AkWaitForSemaphore(AkSemaphore& in_semaphore)
	{
		AKVERIFY(sceKernelWaitSema(in_semaphore, 1, NULL) == SCE_OK);
	}

	/// Platform Independent Helper - Semaphore signal, aka Operation V. Increments value of semaphore by an arbitrary count.
	inline void AkReleaseSemaphore(AkSemaphore& in_semaphore, AkUInt32 in_count)
	{
		AKVERIFY(sceKernelSignalSema(in_semaphore, in_count) == SCE_OK);
	}


    // Threads
    // ------------------------------------------------------------------

	/// Platform Independent Helper
	AkForceInline bool AkIsValidThread( AkThread * in_pThread )
	{
		return ( *in_pThread != AK_NULL_THREAD );
	}

	/// Platform Independent Helper
	AkForceInline void AkClearThread( AkThread * in_pThread )
	{
		*in_pThread = AK_NULL_THREAD;
	}

	/// Platform Independent Helper
    AkForceInline void AkCloseThread( AkThread * in_pThread )
    {
        AKASSERT( in_pThread );
        AKASSERT( *in_pThread );

		// #define KILL_THREAD(t) do { void *ret; scePthreadJoin(t,&ret); } while(false)
		// AKVERIFY( SCE_OK == sceKernelDeleteThread( *in_pThread ) );
        AkClearThread( in_pThread );
    }

	#define AkExitThread( _result ) return _result; // ?????

	/// Platform Independent Helper
	AkForceInline void AkGetDefaultThreadProperties( AkThreadProperties & out_threadProperties )
	{
		out_threadProperties.uStackSize		= AK_DEFAULT_STACK_SIZE;
		out_threadProperties.uSchedPolicy	= AK_THREAD_DEFAULT_SCHED_POLICY;
		out_threadProperties.nPriority		= AK_THREAD_PRIORITY_NORMAL;
		out_threadProperties.dwAffinityMask = AK_THREAD_AFFINITY_DEFAULT;
	}

	/// Platform Independent Helper
	inline void AkCreateThread( 
		AkThreadRoutine pStartRoutine,					// Thread routine.
		void * pParams,									// Routine params.
		const AkThreadProperties & in_threadProperties,	// Properties. NULL for default.
		AkThread * out_pThread,							// Returned thread handle.
		const char * in_szThreadName )				// Opt thread name.
    {
		AKASSERT( out_pThread != NULL );
		
		ScePthreadAttr  attr;
		
		// Create the attr
		AKVERIFY(!scePthreadAttrInit(&attr));
		// Set the stack size
		AKVERIFY(!scePthreadAttrSetstacksize(&attr,in_threadProperties.uStackSize));
		AKVERIFY(!scePthreadAttrSetdetachstate(&attr, SCE_PTHREAD_CREATE_JOINABLE));
		AKVERIFY(!scePthreadAttrSetinheritsched(&attr, SCE_PTHREAD_EXPLICIT_SCHED));
		AKVERIFY(!scePthreadAttrSetaffinity(&attr,in_threadProperties.dwAffinityMask)); 
		
		// Try to set the thread policy
		int sched_policy = in_threadProperties.uSchedPolicy;
		if( scePthreadAttrSetschedpolicy( &attr, sched_policy )  )
		{
			AKASSERT( !"AKCreateThread invalid sched policy, will automatically set it to FIFO scheduling" );
			sched_policy = AK_THREAD_DEFAULT_SCHED_POLICY;
			AKVERIFY( !scePthreadAttrSetschedpolicy( &attr, sched_policy ));
		}

		int minPriority, maxPriority;
		minPriority = SCE_KERNEL_PRIO_FIFO_HIGHEST;
		maxPriority = SCE_KERNEL_PRIO_FIFO_LOWEST;
		
		// Set the thread priority if valid
		AKASSERT( in_threadProperties.nPriority >= minPriority && in_threadProperties.nPriority <= maxPriority );
		if(  in_threadProperties.nPriority >= minPriority && in_threadProperties.nPriority <= maxPriority )
		{
			SceKernelSchedParam schedParam;
			AKVERIFY( scePthreadAttrGetschedparam(&attr, &schedParam) == 0 );
			schedParam.sched_priority = in_threadProperties.nPriority;
			AKVERIFY( scePthreadAttrSetschedparam(&attr, &schedParam) == 0 );
		}

		// Create the tread
		int threadError = scePthreadCreate(out_pThread, &attr, pStartRoutine, pParams, in_szThreadName);
		AKASSERT( threadError == 0 );
		AKVERIFY(!scePthreadAttrDestroy(&attr));
		
		if( threadError != 0 )
		{
			AkClearThread( out_pThread );
			return;
		}
		
		// ::CreateThread() return NULL if it fails.
        if ( !*out_pThread )
        {
			AkClearThread( out_pThread );
            return;
        }		
    }

	/// Platform Independent Helper
    AkForceInline void AkWaitForSingleThread( AkThread * in_pThread )
    {
        AKASSERT( in_pThread );
        AKASSERT( *in_pThread );
		AKVERIFY(!scePthreadJoin( *in_pThread, NULL ));
    }

	inline AkThreadID CurrentThread()
	{
		return scePthreadSelf();
	}

	/// Platform Independent Helper
    AkForceInline void AkSleep( AkUInt32 in_ulMilliseconds )
    {
		usleep( in_ulMilliseconds * 1000 );
    }

    // Time functions
    // ------------------------------------------------------------------

	/// Platform Independent Helper
    AkForceInline void PerformanceCounter( AkInt64 * out_piLastTime )
	{
		uint64_t uTime = sceKernelGetProcessTimeCounter();
		*out_piLastTime = (AkInt64)uTime;
	}

	/// Frequency of the PerformanceCounter() (ticks per second)
	AkForceInline void PerformanceFrequency( AkInt64 * out_piFreq )
	{
		*out_piFreq = (AkInt64)sceKernelGetProcessTimeCounterFrequency();
	}

	/// Platform Independent Helper
    AkForceInline void UpdatePerformanceFrequency()
	{
        AkInt64 iFreq;
        PerformanceFrequency( &iFreq );
        AK::g_fFreqRatio = (AkReal32)((AkReal64)iFreq / 1000);
	}

	/// Returns a time range in milliseconds, using the sound engine's updated count->milliseconds ratio.
    AkForceInline AkReal32 Elapsed( const AkInt64 & in_iNow, const AkInt64 & in_iStart )
    {
        return ( in_iNow - in_iStart ) / AK::g_fFreqRatio;
    }

	/// String conversion helper
	AkForceInline AkInt32 AkWideCharToChar(	const wchar_t*	in_pszUnicodeString,
											AkUInt32		in_uiOutBufferSize,
											char*		io_pszAnsiString )
	{
		AKASSERT( io_pszAnsiString != NULL );

		mbstate_t state;
		memset (&state, '\0', sizeof (state));

		return (AkInt32)wcsrtombs(io_pszAnsiString,		// destination
							&in_pszUnicodeString,	// source
							in_uiOutBufferSize,		// destination length
							&state);				// 

	}
	
	/// String conversion helper
	AkForceInline AkInt32 AkCharToWideChar(	const char*	in_pszAnsiString,
											AkUInt32			in_uiOutBufferSize,
											void*			io_pvUnicodeStringBuffer )
	{
		AKASSERT( io_pvUnicodeStringBuffer != NULL );

		mbstate_t state;
		memset (&state, '\0', sizeof (state));

		return (AkInt32)mbsrtowcs((wchar_t*)io_pvUnicodeStringBuffer,	// destination
									&in_pszAnsiString,					// source
									in_uiOutBufferSize,					// destination length
									&state);							// 
	}

	AkForceInline AkInt32 AkUtf8ToWideChar( const char*	in_pszUtf8String,
									 AkUInt32		in_uiOutBufferSize,
									 void*			io_pvUnicodeStringBuffer )
	{
		return AkCharToWideChar( in_pszUtf8String, in_uiOutBufferSize, (wchar_t*)io_pvUnicodeStringBuffer );
	}

	/// Safe unicode string copy.
	AkForceInline void SafeStrCpy( wchar_t * in_pDest, const wchar_t* in_pSrc, size_t in_uDestMaxNumChars )
	{
		size_t uSizeCopy = AkMin( in_uDestMaxNumChars - 1, wcslen( in_pSrc ) + 1 );
		wcsncpy( in_pDest, in_pSrc, uSizeCopy );
		in_pDest[uSizeCopy] = '\0';
	}

	/// Safe ansi string copy.
	AkForceInline void SafeStrCpy( char * in_pDest, const char* in_pSrc, size_t in_uDestMaxNumChars )
	{
		size_t uSizeCopy = AkMin( in_uDestMaxNumChars - 1, strlen( in_pSrc ) + 1 );
		strncpy( in_pDest, in_pSrc, uSizeCopy );
		in_pDest[uSizeCopy] = '\0';
	}

	/// Safe unicode string concatenation.
	AkForceInline void SafeStrCat( wchar_t * in_pDest, const wchar_t* in_pSrc, size_t in_uDestMaxNumChars )
	{
		size_t uAvailableSize = ( in_uDestMaxNumChars - wcslen( in_pDest ) - 1 );
		wcsncat( in_pDest, in_pSrc, AkMin( uAvailableSize, wcslen( in_pSrc ) ) );
	}

	/// Safe ansi string concatenation.
	AkForceInline void SafeStrCat( char * in_pDest, const char* in_pSrc, size_t in_uDestMaxNumChars )
	{
		size_t uAvailableSize = ( in_uDestMaxNumChars - strlen( in_pDest ) - 1 );
		strncat( in_pDest, in_pSrc, AkMin( uAvailableSize, strlen( in_pSrc ) ) );
	}

	inline int SafeStrFormat(wchar_t * in_pDest, size_t in_uDestMaxNumChars, const wchar_t* in_pszFmt, ...)
	{
		va_list args;
		va_start(args, in_pszFmt);
		int r = vswprintf(in_pDest, in_uDestMaxNumChars, in_pszFmt, args);
		va_end(args);
		return r;
	}

	inline int SafeStrFormat(char * in_pDest, size_t in_uDestMaxNumChars, const char* in_pszFmt, ...)
	{
		va_list args;
		va_start(args, in_pszFmt);
		int r = vsnprintf(in_pDest, in_uDestMaxNumChars, in_pszFmt, args);
		va_end(args);
		return r;
	}

	/// Stack allocations.
	#define AkAlloca( _size_ ) alloca( _size_ )	

	

	/// Converts a wchar_t string to an AkOSChar string.
	/// \remark On some platforms the AkOSChar string simply points to the same string,
	/// on others a new buffer is allocated on the stack using AkAlloca. This means
	/// you must make sure that:
	/// - The source string stays valid and unmodified for as long as you need the
	///   AkOSChar string (for cases where they point to the same string)
	/// - The AkOSChar string is used within this scope only -- for example, do NOT
	///   return that string from a function (for cases where it is allocated on the stack)
	#define CONVERT_WIDE_TO_OSCHAR( _wstring_, _oscharstring_ ) \
		   _oscharstring_ = (AkOSChar*)AkAlloca( (1 + wcslen( _wstring_ )) * sizeof(AkOSChar) ); \
		   AKPLATFORM::AkWideCharToChar( _wstring_ , (AkUInt32)(1 + wcslen( _wstring_ )), (AkOSChar*)( _oscharstring_ ) )


	/// Converts a char string to an AkOSChar string.
	/// \remark On some platforms the AkOSChar string simply points to the same string,
	/// on others a new buffer is allocated on the stack using AkAlloca. This means
	/// you must make sure that:
	/// - The source string stays valid and unmodified for as long as you need the
	///   AkOSChar string (for cases where they point to the same string)
	/// - The AkOSChar string is used within this scope only -- for example, do NOT
	///   return that string from a function (for cases where it is allocated on the stack)
	#define CONVERT_CHAR_TO_OSCHAR( _astring_, _oscharstring_ ) ( _oscharstring_ ) = (AkOSChar*)( _astring_ )

	/// Converts a AkOSChar string into wide char string.
	/// \remark On some platforms the AkOSChar string simply points to the same string,
	/// on others a new buffer is allocated on the stack using AkAlloca. This means
	/// you must make sure that:
	/// - The source string stays valid and unmodified for as long as you need the
	///   AkOSChar string (for cases where they point to the same string)
	/// - The AkOSChar string is used within this scope only -- for example, do NOT
	///   return that string from a function (for cases where it is allocated on the stack)
	#define CONVERT_OSCHAR_TO_WIDE( _osstring_, _wstring_ ) \
		_wstring_ = (wchar_t*)AkAlloca((1+strlen(_osstring_)) * sizeof(wchar_t)); \
		AKPLATFORM::AkCharToWideChar( _osstring_, (AkUInt32)(1 + strlen(_osstring_ )), _wstring_ )

	/// Converts a AkOSChar string into char string.
	/// \remark On some platforms the AkOSChar string simply points to the same string,
	/// on others a new buffer is allocated on the stack using AkAlloca. This means
	/// you must make sure that:
	/// - The source string stays valid and unmodified for as long as you need the
	///   AkOSChar string (for cases where they point to the same string)
	/// - The AkOSChar string is used within this scope only -- for example, do NOT
	///   return that string from a function (for cases where it is allocated on the stack)
	#define CONVERT_OSCHAR_TO_CHAR( _osstring_, _astring_ ) _astring_ = (char*)_osstring_

	/// Get the length, in characters, of a NULL-terminated AkUtf16 string
	/// \return The length, in characters, of the specified string (excluding terminating NULL)
	AkForceInline size_t AkUtf16StrLen( const AkUtf16* in_pStr )
	{
		return ( wcslen( in_pStr ) );
	}

	/// Get the length, in characters, of a NULL-terminated AkOSChar string
	/// \return The length, in characters, of the specified string (excluding terminating NULL)
	AkForceInline size_t OsStrLen( const AkOSChar* in_pszString )
	{
		return ( strlen( in_pszString ) );
	}

	/// AkOSChar version of sprintf().
	#define AK_OSPRINTF snprintf

	/// Compare two NULL-terminated AkOSChar strings
	/// \return
	/// - \< 0 if in_pszString1 \< in_pszString2
	/// -    0 if the two strings are identical
	/// - \> 0 if in_pszString1 \> in_pszString2
	/// \remark The comparison is case-sensitive
	AkForceInline int OsStrCmp( const AkOSChar* in_pszString1, const AkOSChar* in_pszString2 )
	{
		return ( strcmp( in_pszString1,  in_pszString2 ) );
	}

	/// Compare two NULL-terminated AkOSChar strings up to the specified count of characters.
	/// \return
	/// - \< 0 if in_pszString1 \< in_pszString2
	/// -    0 if the two strings are identical
	/// - \> 0 if in_pszString1 \> in_pszString2
	/// \remark The comparison is case-sensitive
	inline int OsStrNCmp( const AkOSChar* in_pszString1, const AkOSChar* in_pszString2, size_t in_MaxCountSize )
	{
		return ( strncmp(in_pszString1, in_pszString2, in_MaxCountSize) );
	}
	
	/// Detects whether the string represents an absolute path to a file
	inline bool IsAbsolutePath(const AkOSChar* in_pszPath, size_t in_pathLen)
	{
		return in_pathLen >= 1 && in_pszPath[0] == '/';
	}

	#define AK_UTF16_TO_WCHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::SafeStrCpy(		in_pdDest, in_pSrc, in_MaxSize )
	#define AK_WCHAR_TO_UTF16(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::SafeStrCpy(		in_pdDest, in_pSrc, in_MaxSize )
	#define AK_UTF8_TO_OSCHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::SafeStrCpy(		in_pdDest, in_pSrc, in_MaxSize )
	#define AK_UTF16_TO_OSCHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkWideCharToChar( in_pSrc, in_MaxSize, in_pdDest )
	#define AK_UTF16_TO_CHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkWideCharToChar( in_pSrc, in_MaxSize, in_pdDest )
	#define AK_CHAR_TO_UTF16(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkCharToWideChar( in_pSrc, in_MaxSize, in_pdDest )		
	#define AK_OSCHAR_TO_UTF16(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkCharToWideChar( in_pSrc, in_MaxSize, in_pdDest )	

	// Use with AkOSChar.
	#define AK_PATH_SEPARATOR	"/"
	#define AK_LIBRARY_PREFIX	""
	#define AK_DYNAMIC_LIBRARY_EXTENSION	".prx"

	#define AK_FILEHANDLE_TO_UINTPTR(_h) ((AkUIntPtr)_h)
	#define AK_SET_FILEHANDLE_TO_UINTPTR(_h,_u) _h = (AkFileHandle)_u
}

#endif  // _AK_PLATFORM_FUNCS_H_