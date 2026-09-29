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

#include "Wwise/Packaging/WwiseAssetLibraryInfo.h"
#include "Wwise/Packaging/WwiseAssetLibraryFilter.h"
#include "Wwise/Packaging/WwiseSharedAssetLibraryFilter.h"
#include "Wwise/Stats/PackagingRuntime.h"

#if WITH_EDITORONLY_DATA
void FWwiseAssetLibraryInfo::PreloadFilters()
{
	checkf(IsInGameThread(), TEXT("PreloadFilters must be called on GameThread"));

	PreloadedFilters.Empty(SharedFilters.Num());
	for (const auto& SharedFilter: SharedFilters)
	{
		if (auto* Obj = SharedFilter.LoadSynchronous())
		{
			UE_LOG(LogWwisePackagingRuntime, Verbose,
				TEXT("FWwiseAssetLibraryInfo::PreloadFilters: Preloaded filter %s"), *Obj->GetName());
			PreloadedFilters.Add(ToObjectPtr(Obj));
		}
		else
		{
			UE_CLOG(!SharedFilter.IsNull(),
				LogWwisePackagingRuntime, Warning, TEXT("FWwiseAssetLibraryInfo::PreloadFilters: Could not preload filter %s"), *SharedFilter.GetAssetName());
		}
	}
}

TArray<TObjectPtr<UWwiseAssetLibraryFilter>> FWwiseAssetLibraryInfo::GetFilters(TSet<const UObject*>* ProcessedFilters) const
{
	TSet<const UObject*> NewProcessedFilters;
	if (!ProcessedFilters)
	{
		ProcessedFilters = &NewProcessedFilters;
	}

	TArray<TObjectPtr<UWwiseAssetLibraryFilter>> Result;
	Result.Reserve(Filters.Num() + SharedFilters.Num() * 10);		// Have some leeway
	
	for (const auto& FilterPtr : Filters)
	{
		if (const auto* Filter = FilterPtr.Get())
		{
			bool bIsAlreadyInSetPtr{ false };
			ProcessedFilters->FindOrAdd(Filter, &bIsAlreadyInSetPtr);
			if (UNLIKELY(bIsAlreadyInSetPtr))
			{
				UE_LOG(LogWwisePackagingRuntime, Verbose, TEXT("FWwiseAssetLibraryInfo::GetFilters: Filter re-use detected: %s is processed twice. Skipping"), *Filter->GetName());
			}
			else
			{
				Result.Add(FilterPtr);
			}
		}
	}

	for (const auto& SharedFilter: PreloadedFilters)
	{
		if (const auto* Filter = SharedFilter.Get())
		{
			bool bIsAlreadyInSetPtr{ false };
			ProcessedFilters->FindOrAdd(Filter, &bIsAlreadyInSetPtr);
			if (UNLIKELY(bIsAlreadyInSetPtr))
			{
				UE_LOG(LogWwisePackagingRuntime, Verbose, TEXT("FWwiseAssetLibraryInfo::GetFilters: Shared filter re-use detected: %s is processed twice. Skipping"), *Filter->GetName());
			}
			else
			{
				Result.Append(Filter->Info.GetFilters(ProcessedFilters));
			}
		}
	}
	return Result;
}
#endif
