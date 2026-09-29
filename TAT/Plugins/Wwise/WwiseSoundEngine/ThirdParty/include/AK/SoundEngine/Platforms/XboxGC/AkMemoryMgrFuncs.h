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

#include <AK/SoundEngine/Common/AkTypes.h>
#include <xmem.h>

#define AK_VM_PAGE_SIZE                         (64*1024)
#define AK_VM_HUGE_PAGE_SIZE                    (2*1024*1024)

namespace AKPLATFORM
{
	static const ULONGLONG XMemAllocLargePage = MAKE_XALLOC_ATTRIBUTES(
		AK_WWISE_XMEMALLOC_ALLOCATORID,
		0,
		XALLOC_MEMTYPE_HEAP_CACHEABLE,
		XALLOC_PAGESIZE_64KB,
		XALLOC_ALIGNMENT_64K,
		FALSE
	);

	static const ULONGLONG XMemAllocHugePage = MAKE_XALLOC_ATTRIBUTES(
		AK_WWISE_XMEMALLOC_ALLOCATORID,
		0,
		XALLOC_MEMTYPE_HEAP_CACHEABLE,
		XALLOC_PAGESIZE_2MB,
		XALLOC_ALIGNMENT_64K,
		FALSE
	);

	AkForceInline void* AllocSpan(size_t size, size_t* out_userData)
	{
		AkUInt32 uRoundedSize = AK_ALIGN_TO_NEXT_BOUNDARY(size, AK_VM_PAGE_SIZE);
		ULONGLONG attrib = (uRoundedSize % AK_VM_HUGE_PAGE_SIZE == 0) ? XMemAllocHugePage : XMemAllocLargePage;
		*out_userData = (size_t)attrib;
		return XMemAlloc(uRoundedSize, attrib);
	}

	AkForceInline  void FreeSpan(void* address, size_t size, size_t in_userData)
	{
		XMemFree(address, in_userData);
	}
}
