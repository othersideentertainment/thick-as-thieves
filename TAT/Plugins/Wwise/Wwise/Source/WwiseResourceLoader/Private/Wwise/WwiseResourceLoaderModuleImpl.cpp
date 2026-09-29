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

#include "Wwise/WwiseResourceLoaderModuleImpl.h"
#include "Wwise/WwiseResourceLoaderImpl.h"

IMPLEMENT_MODULE(FWwiseResourceLoaderModule, WwiseResourceLoader)

FWwiseResourceLoaderPtr FWwiseResourceLoaderModule::GetResourceLoader()
{
	Lock.ReadLock();
	if (LIKELY(ResourceLoader))
	{
		Lock.ReadUnlock();
	}
	else
	{
		Lock.ReadUnlock();
		Lock.WriteLock();
		if (LIKELY(!ResourceLoader.IsValid()))
		{
			UE_LOG(LogWwiseResourceLoader, Log, TEXT("Initializing default Resource Loader."));
			ResourceLoader.Reset();
			ResourceLoader = InstantiateResourceLoader();
		}
		Lock.WriteUnlock();
	}
	return ResourceLoader;
}

FWwiseResourceLoaderPtr FWwiseResourceLoaderModule::InstantiateResourceLoader()
{
	SCOPED_WWISERESOURCELOADER_EVENT(TEXT("FWwiseResourceLoaderModule::InstantiateResourceLoader"));
	return MakeShared<FWwiseResourceLoaderImpl>();
}

void FWwiseResourceLoaderModule::ShutdownModule()
{
	Lock.WriteLock();
	if (ResourceLoader.IsValid())
	{
		UE_LOG(LogWwiseResourceLoader, Log, TEXT("Shutting down default Resource Loader."));
		ResourceLoader.Reset();
	}
	Lock.WriteUnlock();
	IWwiseResourceLoaderModule::ShutdownModule();
}
