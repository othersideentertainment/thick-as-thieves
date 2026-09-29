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

#pragma once

#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkTypes.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#define AK_SEC_TO_NANOSEC 1000000000ULL

#define AK_THREAD_INIT_CODE(_threadProperties) \
	if (_threadProperties.dwAffinityMask != AK_THREAD_AFFINITY_DEFAULT) \
	{ \
		cpu_set_t affinity; \
		CPU_ZERO(&affinity); \
		for (AkUInt32 i = 0; i < 32; ++i) { \
			if (_threadProperties.dwAffinityMask & ( 1 << i )){\
				CPU_SET(i, &affinity); \
			} \
		} \
		sched_setaffinity(0, sizeof(cpu_set_t), &affinity); \
	}

namespace AKPLATFORM
{
    // Time functions
    // ------------------------------------------------------------------

	/// Platform Independent Helper
    inline void PerformanceCounter( AkInt64 * out_piLastTime )
	{
		struct timespec clockNow;
		clock_gettime(CLOCK_MONOTONIC, &clockNow);
		//This give the wallclock time in NS
		*out_piLastTime = clockNow.tv_sec*AK_SEC_TO_NANOSEC + clockNow.tv_nsec;
	}

	/// Platform Independent Helper
	inline void PerformanceFrequency( AkInt64 * out_piFreq )
	{
		//Since Wall Clock is used, 1 NS is the frequency independent of the clock resolution
		*out_piFreq = AK_SEC_TO_NANOSEC;
	}

	template<class destType, class srcType>
	inline size_t AkSimpleConvertString( destType* in_pdDest, const srcType* in_pSrc, size_t in_MaxSize, size_t destStrLen(const destType *),  size_t srcStrLen(const srcType *) )
	{ 
		size_t i;
		size_t lenToCopy = srcStrLen(in_pSrc);
		
		lenToCopy = (lenToCopy > in_MaxSize-1) ? in_MaxSize-1 : lenToCopy;
		for(i = 0; i < lenToCopy; i++)
		{
			in_pdDest[i] = (destType) in_pSrc[i];
		}
		in_pdDest[lenToCopy] = (destType)0;
		
		return lenToCopy;
	}

	#define CONVERT_UTF16_TO_CHAR( _astring_, _charstring_ ) \
		_charstring_ = (char*)AkAlloca( (1 + AKPLATFORM::AkUtf16StrLen((const AkUtf16*)_astring_)) * sizeof(char) ); \
		AK_UTF16_TO_CHAR( _charstring_, (const AkUtf16*)_astring_, AKPLATFORM::AkUtf16StrLen((const AkUtf16*)_astring_)+1 ) 

    #define AK_UTF16_TO_CHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, strlen, AKPLATFORM::AkUtf16StrLen )
	#define AK_UTF8_TO_OSCHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, strlen, strlen )
    #define AK_UTF16_TO_OSCHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, strlen, AKPLATFORM::AkUtf16StrLen )
    #define AK_UTF16_TO_WCHAR(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, wcslen, AKPLATFORM::AkUtf16StrLen )
    #define AK_CHAR_TO_UTF16(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, AKPLATFORM::AkUtf16StrLen, strlen )
    #define AK_OSCHAR_TO_UTF16(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, AKPLATFORM::AkUtf16StrLen, strlen )
    #define AK_WCHAR_TO_UTF16(	in_pdDest, in_pSrc, in_MaxSize )	AKPLATFORM::AkSimpleConvertString( in_pdDest, in_pSrc, in_MaxSize, AKPLATFORM::AkUtf16StrLen, wcslen )

	#define AK_FILEHANDLE_TO_UINTPTR(_h) ((AkUIntPtr)_h)
	#define AK_SET_FILEHANDLE_TO_UINTPTR(_h,_u) _h = (AkFileHandle)_u

	/// Stack allocations.
	#define AkAlloca( _size_ ) __builtin_alloca( _size_ )
}
