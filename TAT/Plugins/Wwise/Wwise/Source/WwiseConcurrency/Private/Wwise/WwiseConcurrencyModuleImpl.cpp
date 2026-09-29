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

#include "Wwise/WwiseConcurrencyModuleImpl.h"

#include "Misc/QueuedThreadPool.h"
#include "Wwise/WwiseExecutionQueue.h"
#include "Wwise/Stats/Concurrency.h"

IMPLEMENT_MODULE(FWwiseConcurrencyModule, WwiseConcurrency)

FWwiseConcurrencyModule::FWwiseConcurrencyModule()
{
}

void FWwiseConcurrencyModule::StartupModule()
{
	UE_LOG(LogWwiseConcurrency, Log, TEXT("Initializing default Concurrency."));

	DefaultQueue = new FWwiseExecutionQueue(WWISE_EQ_NAME("Default"));
	
	IWwiseConcurrencyModule::StartupModule();
}

void FWwiseConcurrencyModule::ShutdownModule()
{
	UE_LOG(LogWwiseConcurrency, Log, TEXT("Shutting down default Concurrency."));

	if (DefaultQueue)
	{
		auto* Closing{ DefaultQueue };
		DefaultQueue = nullptr;
		
		Closing->CloseAndDelete();
	}
	IWwiseConcurrencyModule::ShutdownModule();
}
