/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#include "Wwise/API_Null/WwiseMemoryMgrAPI_Null.h"
#include "Wwise/Stats/SoundEngine.h"

AKRESULT FWwiseMemoryMgrAPI_Null::Init(
	AkMemSettings* in_pSettings
)
{
	return AK_NotImplemented;
}

void FWwiseMemoryMgrAPI_Null::GetDefaultSettings(
	AkMemSettings& out_pMemSettings
)
{
}

bool FWwiseMemoryMgrAPI_Null::IsInitialized()
{
	return false;
}

void FWwiseMemoryMgrAPI_Null::Term()
{
}

void FWwiseMemoryMgrAPI_Null::InitForThread()
{
}

void FWwiseMemoryMgrAPI_Null::TermForThread()
{
}

void FWwiseMemoryMgrAPI_Null::TrimForThread()
{
}

void* FWwiseMemoryMgrAPI_Null::dMalloc(
	AkMemPoolId in_poolId,
	size_t		in_uSize,
	const char* in_pszFile,
	AkUInt32	in_uLine
)
{
	return nullptr;
}

void* FWwiseMemoryMgrAPI_Null::Malloc(
	AkMemPoolId in_poolId,
	size_t		in_uSize
)
{
	return nullptr;
}

void* FWwiseMemoryMgrAPI_Null::dRealloc(
	AkMemPoolId	in_poolId,
	void* in_pAlloc,
	size_t		in_uSize,
	const char* in_pszFile,
	AkUInt32	in_uLine
)
{
	return nullptr;
}

void* FWwiseMemoryMgrAPI_Null::Realloc(
	AkMemPoolId in_poolId,
	void* in_pAlloc,
	size_t		in_uSize
)
{
	return nullptr;
}

void* FWwiseMemoryMgrAPI_Null::dReallocAligned(
	AkMemPoolId	in_poolId,
	void* in_pAlloc,
	size_t		in_uSize,
	AkUInt32	in_uAlignment,
	const char* in_pszFile,
	AkUInt32	in_uLine
)
{
	return nullptr;
}

void* FWwiseMemoryMgrAPI_Null::ReallocAligned(
	AkMemPoolId in_poolId,
	void* in_pAlloc,
	size_t		in_uSize,
	AkUInt32	in_uAlignment
)
{
	return nullptr;
}

void FWwiseMemoryMgrAPI_Null::Free(
	AkMemPoolId in_poolId,
	void* in_pMemAddress
)
{
}

void* FWwiseMemoryMgrAPI_Null::dMalign(
	AkMemPoolId in_poolId,
	size_t		in_uSize,
	AkUInt32	in_uAlignment,
	const char* in_pszFile,
	AkUInt32	in_uLine
)
{
	return nullptr;
}

void* FWwiseMemoryMgrAPI_Null::Malign(
	AkMemPoolId in_poolId,
	size_t		in_uSize,
	AkUInt32	in_uAlignment
)
{
	return nullptr;
}

void FWwiseMemoryMgrAPI_Null::GetCategoryStats(
	AkMemPoolId	in_poolId,
	AK::MemoryMgr::CategoryStats& out_poolStats
)
{
}

void FWwiseMemoryMgrAPI_Null::GetGlobalStats(
	AK::MemoryMgr::GlobalStats& out_stats
)
{
}

void FWwiseMemoryMgrAPI_Null::StartProfileThreadUsage(
)
{
}

AkUInt64 FWwiseMemoryMgrAPI_Null::StopProfileThreadUsage(
)
{
	return 0;
}

void FWwiseMemoryMgrAPI_Null::DumpToFile(
	const AkOSChar* pszFilename
)
{
}

