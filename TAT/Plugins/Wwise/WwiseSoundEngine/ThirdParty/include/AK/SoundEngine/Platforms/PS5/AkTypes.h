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

#ifdef __PROSPERO__

#include <AK/SoundEngine/Common/AkNumeralTypes.h>

#include <kernel.h>
#include <sceconst.h>
#include <scetypes.h>
#include <sdk_version.h>
#include <stddef.h>

#if !defined(AK_PS5)
	#define AK_PS5							///< Compiling for PS5
#endif
#if !defined(AK_SONY)
	#define AK_SONY									///< Sony platform
#endif

#define AK_SCE_AJM_SUPPORTED                    ///< Supports SCE AJM library
#define AK_ATRAC9_SUPPORTED                     ///< Supports ATRAC9 codec
#define AK_DVR_BYPASS_SUPPORTED                 ///< Supports feature which blocks DVRs from recording BGM
#define AK_HARDWARE_DECODING_SUPPORTED          ///< Supports LEngine callbacks for hardware decoding
#define AK_WEM_OPUS_HW_SUPPORTED                ///< Supports hardware decoding of WEM Opus codec
#define AK_VORBIS_HW_SUPPORTED                  ///< Supports hardware decoding of Vorbis codec
#define AK_DEVICE_MEMORY_SUPPORTED              ///< Supports special memory allocations shared with the audio co-processor
#define AK_CUSTOM_INTERNAL_JOBS_SUPPORTED       ///< Supports extra, internal, job queues for jobmgr
#define AK_HARDWARE_FILTER_MIX_SUPPORTED        ///< Supports a hardware-assisted filter-and-mix operation, instead of the common software version
#define AK_MONITORX_SUPPORTED                   ///< Supports direct use of the monitorx intrinsic

#define AK_SUPPORT_WCHAR						///< Can support wchar
#define AK_SUPPORT_THREADS
#define AK_71FROM51MIXER						///< Internal use
#define AK_71FROMSTEREOMIXER					///< Internal use

//#define AK_ENABLE_RAZOR_PROFILING

#define AK_CPU_X86_64

#define AK_RESTRICT				__restrict								///< Refers to the __restrict compilation flag available on some platforms
#define AK_EXPECT_FALSE( _x )	( _x )
#define AkRegister
#define AkForceInline			inline __attribute__((always_inline))	///< Force inlining
#define AkNoInline				__attribute__((noinline))

#define AK_SIMD_ALIGNMENT	16					///< Platform-specific alignment requirement for SIMD data
#define AK_ALIGN_SIMD( _declaration_ )	AK_ALIGN( _declaration_, AK_SIMD_ALIGNMENT )	///< Platform-specific alignment requirement for SIMD data

#define AK_BUFFER_ALIGNMENT 128					///< Equal to ACM data alignment, so that we can use any audio buffer as an input/output for ACM processing

/// These flags define that a given class of SIMD extensions is available.
/// Note that runtime checks MUST be done before entering code that explicitly utilizes one of these classes
#define AKSIMD_V4F32_SUPPORTED
#define AKSIMD_AVX_SUPPORTED
#define AKSIMD_AVX2_SUPPORTED

#define AK_DLLEXPORT __declspec(dllexport)
#define AK_DLLIMPORT __declspec(dllimport)

typedef char					AkOSChar;				///< Generic character string
typedef wchar_t					AkUtf16;				///< Type for 2 byte chars. Used for communication
														///< with the authoring tool.

typedef ScePthread				AkThread;				///< Thread handle
typedef ScePthread				AkThreadID;				///< Thread ID
typedef void* 					(*AkThreadRoutine)(	void* lpThreadParameter	);		///< Thread routine
typedef SceKernelEventFlag		AkEvent;				///< Event handle
typedef SceKernelSema			AkSemaphore;			///< Semaphore handle

typedef int				AkFileHandle;			///< File handle to be used with sceKernel file system calls

#define AK_STACKTRACE_MAX_FRAMES 64
typedef uintptr_t AkStackTrace[ AK_STACKTRACE_MAX_FRAMES ];

#define AK_UINT_MAX				UINT_MAX

// For strings.
#define AK_MAX_PATH				SCE_KERNEL_PATH_MAX						///< Maximum path length (each file/dir name is max 255 char)

typedef AkUInt32				AkFourcc;				///< Riff chunk

/// Create Riff chunk
#define AkmmioFOURCC( ch0, ch1, ch2, ch3 )									    \
		( (AkFourcc)(AkUInt8)(ch0) | ( (AkFourcc)(AkUInt8)(ch1) << 8 ) |		\
		( (AkFourcc)(AkUInt8)(ch2) << 16 ) | ( (AkFourcc)(AkUInt8)(ch3) << 24 ) )

#define AK_BANK_PLATFORM_DATA_ALIGNMENT (128) ///< Required memory alignment for bank loading by memory address (for ACM-compatible data -- SCE_ACM_DATA_ALIGNMENT)

#define AK_COMM_CONSOLE_TYPE ConsolePS5

/// Format for printing AkOSChar string using OutputDebugMsgV
/// Corresponds to "%ls" if AK_OS_WCHAR, else "%s".
/// \remark Usage: AKPLATFORM::OutputDebugMsgV(AKTEXT("Print this string: " AK_OSCHAR_FMT "\n", msg));
#define AK_OSCHAR_FMT "%s"

/// Macro that takes a string litteral and changes it to an AkOSChar string at compile time
/// \remark This is similar to the TEXT() and _T() macros that can be used to turn string litterals into Unicode strings
/// \remark Usage: AKTEXT( "Some Text" )
#define AKTEXT(x) x

// easy define for checking for SDK8.00 and its corresponding 12-channel-output support (Atmos) and other versions
#if (SCE_PROSPERO_SDK_VERSION >= 0x08000026u)
#define AK_PROSPERO_SDK_800_OR_LATER true
#endif
#if (SCE_PROSPERO_SDK_VERSION >= 0x09000040u)
#define AK_PROSPERO_SDK_900_OR_LATER true
#endif

#endif // __PROSPERO__
