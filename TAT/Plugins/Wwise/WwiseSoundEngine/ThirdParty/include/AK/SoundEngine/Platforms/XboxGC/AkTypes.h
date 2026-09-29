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

// AkTypes.h

/// \file 
/// Data type definitions.

#pragma once

#if defined(_GAMING_XBOX_SCARLETT)
	#if !defined(AK_XBOXSERIESX)
		#define AK_XBOXSERIESX
	#endif
	#define AK_WEM_OPUS_HW_SUPPORTED
#elif defined(_GAMING_XBOX_XBOXONE)
	#if !defined(AK_XBOXONEGC)
		#define AK_XBOXONEGC
	#endif
#else
#error "Unsupported version of Gaming.Xbox platform"
#endif


#include <AK/SoundEngine/Common/AkNumeralTypes.h>

#include <limits.h>
#include <winapifamily.h>

#ifndef __cplusplus
	#include <wchar.h> // wchar_t not a built-in type in C
#endif

#if !defined(AK_XBOX)
	#define AK_XBOX
#endif
#if !defined(AK_XBOXGC)
	#define AK_XBOXGC
#endif
#define AK_CPU_X86_64							///< Compiling for 64-bit x86 CPU

#define AK_DVR_BYPASS_SUPPORTED                 ///< Supports feature which blocks DVRs from recording BGM

#if defined(AK_XBOXSERIESX)
#define AK_HARDWARE_DECODING_SUPPORTED          ///< For OpusHW
#define AK_MONITORX_SUPPORTED                   ///< Supports direct use of the monitorx intrinsic (only on Xbox Series)
#endif

#define AK_WASAPI								///< Enable WASAPI sink

#define AK_COMM_NO_DYNAMIC_PORTS				///< Platform does not support dynamic/ephemeral ports for communication
#define AK_DEVICE_CACHE_SUPPORT					///< Supports output device notifications & cache

#define AK_SUPPORT_THREADS
#define AK_SUPPORT_WCHAR						///< Can support wchar
#define AK_OS_WCHAR								///< Use wchar natively

#define AK_RESTRICT		__restrict				///< Refers to the __restrict compilation flag available on some platforms
#define AK_EXPECT_FALSE( _x )	(_x)
#define AkForceInline	__forceinline			///< Force inlining
#define AkNoInline		__declspec(noinline)	///< Disable inlining

#define AK_SIMD_ALIGNMENT	16					///< Platform-specific alignment requirement for SIMD data
#define AK_ALIGN_SIMD( _declaration_ )	AK_ALIGN( _declaration_, AK_SIMD_ALIGNMENT )	///< Platform-specific alignment requirement for SIMD data
#define AK_BUFFER_ALIGNMENT AK_SIMD_ALIGNMENT

/// These flags define that a given class of SIMD extensions is available.
/// Note that runtime checks MUST be done before entering code that explicitly utilizes one of these classes
#define AKSIMD_V4F32_SUPPORTED
#if defined(_GAMING_XBOX_XBOXONE)
#define AKSIMD_AVX_SUPPORTED // AVX supported on XB1
#elif defined(_GAMING_XBOX_SCARLETT)
#define AKSIMD_AVX_SUPPORTED // AVX supported on Xbox Series X
#define AKSIMD_AVX2_SUPPORTED // ..and AVX2 as welll
#endif

#if defined(_GAMING_XBOX_SCARLETT)
#define AK_THREAD_AFFINITY_ALL       16383 // from 0b0011'1111'1111'1111 -- 14 cores available
#define AK_THREAD_AFFINITY_DEFAULT   16383 // from 0b0011'1111'1111'1111 -- Default to 14 fully-available cores
#elif defined(_GAMING_XBOX_XBOXONE)
#define AK_THREAD_AFFINITY_ALL         127 // from 0b0111'1111 -- 7 cores available
#define AK_THREAD_AFFINITY_DEFAULT      63 // from 0b0011'1111 -- Default to only 6 fully-available cores. 7th core is half-available.
#endif

#define AK_DLLEXPORT __declspec(dllexport)
#define AK_DLLIMPORT __declspec(dllimport)

typedef wchar_t					AkOSChar;		///< Generic character string
typedef wchar_t					AkUtf16;		///< Type for 2 byte chars. Used for communication
												///< with the authoring tool.

typedef void *					AkThread;		///< Thread handle
typedef unsigned long			AkThreadID;		///< Thread ID
typedef unsigned long (__stdcall *AkThreadRoutine)(	void* lpThreadParameter	); ///< Thread routine
typedef void *					AkEvent;		///< Event handle
typedef void *					AkSemaphore;	///< Semaphore handle

typedef void *					AkFileHandle;	///< File handle

typedef void* AkStackTrace[ 64 ];

#define AK_UINT_MAX		UINT_MAX

// For strings.
#define AK_MAX_PATH     260						///< Maximum path length.

typedef AkUInt32			AkFourcc;			///< Riff chunk

/// Create Riff chunk
#define AkmmioFOURCC( ch0, ch1, ch2, ch3 )									    \
		( (AkFourcc)(AkUInt8)(ch0) | ( (AkFourcc)(AkUInt8)(ch1) << 8 ) |		\
		( (AkFourcc)(AkUInt8)(ch2) << 16 ) | ( (AkFourcc)(AkUInt8)(ch3) << 24 ) )

#define AK_BANK_PLATFORM_DATA_ALIGNMENT	(16)	///< Required memory alignment for bank loading by memory address (see LoadBank())

/// Format for printing AkOSChar string using OutputDebugMsgV
/// Corresponds to "%ls" if AK_OS_WCHAR, else "%s".
/// \remark Usage: AKPLATFORM::OutputDebugMsgV(AKTEXT("Print this string: " AK_OSCHAR_FMT "\n", msg));
#define AK_OSCHAR_FMT "%ls"

/// Macro that takes a string litteral and changes it to an AkOSChar string at compile time
/// \remark This is similar to the TEXT() and _T() macros that can be used to turn string litterals into wchar_t strings
/// \remark Usage: AKTEXT( "Some Text" )
#define AKTEXT(x) L ## x

/// Default open should be asyunchronous on Xbox.
#define AK_ASYNC_OPEN_DEFAULT	(true)	///< Refers to asynchronous file opening in default low-level IO.

#define AK_COMM_NO_DYNAMIC_PORTS		///< Debugging ports must be defined in advance in MicrosoftGame.config

#define AK_WWISE_XMEMALLOC_ALLOCATORID 215	///< eXALLOCAllocatorId_MiddlewareReservedMin + 23 (...as in "W" for Wwise)
