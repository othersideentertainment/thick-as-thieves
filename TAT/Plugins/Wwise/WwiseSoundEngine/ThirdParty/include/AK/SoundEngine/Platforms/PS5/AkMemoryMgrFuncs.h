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
#include <sys/dmem.h>
#include <sceerror.h>

#define AK_VM_PAGE_SIZE		   (16*1024)
#define AK_VM_HUGE_PAGE_SIZE   (2*1024*1024)
#define AK_VM_DEVICE_PAGE_SIZE (16*1024)

namespace AKPLATFORM
{
	// In order to use ACM hardware accelerated functions, Wwise must have GPU-visible memory
	const int kMemoryFlags = SCE_KERNEL_PROT_CPU_RW | SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_AMPR_ALL;
	const int kDeviceMemoryFlags = SCE_KERNEL_PROT_CPU_RW | SCE_KERNEL_PROT_GPU_RW | SCE_KERNEL_PROT_AMPR_ALL | SCE_KERNEL_PROT_ACP_RW;

	AkForceInline void* AllocSpan(size_t size, size_t* out_userData)
	{
		AkUInt32 uRoundedSize = AK_ALIGN_TO_NEXT_BOUNDARY(size, AK_VM_PAGE_SIZE);
		off_t directMemStart = 0;
		void* ptr = NULL;

		int32_t err;
		// if size lines up with huge pages, set alignment up similarly
		int32_t alignment = (uRoundedSize % AK_VM_HUGE_PAGE_SIZE == 0) ? AK_VM_HUGE_PAGE_SIZE : AK_VM_PAGE_SIZE;
		err = sceKernelAllocateMainDirectMemory(uRoundedSize, alignment, SCE_KERNEL_MTYPE_C_SHARED, &directMemStart);
		if (err == SCE_OK)
		{
			// allocate with device memory flags all of the time, in case a separate device heap is not in use
			err = sceKernelMapDirectMemory(&ptr, uRoundedSize, AKPLATFORM::kDeviceMemoryFlags, 0, directMemStart, alignment);
			AKASSERT(ptr);
			AKASSERT(err == SCE_OK);

			*out_userData = (size_t)directMemStart;
		}
		return ptr;
	}

	AkForceInline void FreeSpan(void* address, size_t size, size_t in_userData)
	{
		AKASSERT(in_userData);
		AkUInt32 uRoundedSize = AK_ALIGN_TO_NEXT_BOUNDARY(size, AK_VM_PAGE_SIZE);
		int32_t err = sceKernelReleaseDirectMemory((off_t)in_userData, uRoundedSize);
		AKASSERT(err == SCE_OK);
	}

	// Default device-specific sample allocation functions can call stock sample allocation functions -- they handle device memory just fine
	AkForceInline void* AllocDeviceSpan(size_t size, size_t* out_userData)
	{
		return AllocSpan(size, out_userData);
	}

	AkForceInline void FreeDeviceSpan(void* address, size_t size, size_t in_userData)
	{
		FreeSpan(address, size, in_userData);
	}

	AkForceInline bool CheckMemoryProtection(void* address, size_t size, int expectedProt)
	{
		AkUIntPtr addressEnd = (AkUIntPtr)address + size - 1;

		// Check protection flags apply to the whole range
		while ((AkUIntPtr)address < addressEnd)
		{
			SceKernelVirtualQueryInfo vqInfo;
			int err = sceKernelVirtualQuery(address, SCE_KERNEL_VQ_FIND_NEXT, &vqInfo, sizeof(vqInfo));
			if (err != SCE_OK)
				return false;

			// Check protection flags
			if ((vqInfo.protection & expectedProt) != expectedProt)
				return false;

			if ((AkUIntPtr)vqInfo.start > (AkUIntPtr)address)
				return false;

			address = vqInfo.end;
		}

		return true;
	}
}
