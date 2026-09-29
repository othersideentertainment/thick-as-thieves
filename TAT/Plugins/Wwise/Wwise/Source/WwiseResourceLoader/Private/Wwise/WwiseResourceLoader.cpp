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

#include "Wwise/WwiseResourceLoader.h"

#include "Wwise/CookedData/WwiseInitBankCookedData.h"
#include "Wwise/CookedData/WwiseLanguageCookedData.h"
#include "Wwise/CookedData/WwiseLocalizedAuxBusCookedData.h"
#include "Wwise/CookedData/WwiseLocalizedEventCookedData.h"
#include "Wwise/CookedData/WwiseLocalizedShareSetCookedData.h"
#include "Wwise/CookedData/WwiseLocalizedSoundBankCookedData.h"
#include "Wwise/Stats/ResourceLoader.h"

#include "Async/Async.h"
#include "Wwise/WwiseTask.h"

#if WITH_EDITORONLY_DATA && PLATFORM_WINDOWS
static const auto DefaultPlatform = MakeShared<FWwisePlatformId, ESPMode::ThreadSafe>(FGuid(0x6E0CB257, 0xC6C84C5C, 0x83662740, 0xDFC441EC), TEXT("Windows"), TEXT("Windows"));
#elif WITH_EDITORONLY_DATA && PLATFORM_MAC
static const auto DefaultPlatform = MakeShared<FWwisePlatformId, ESPMode::ThreadSafe>(FGuid(0x02DC7702, 0x6E7B4AE4, 0xBAE464D2, 0xB1057150), TEXT("Mac"), TEXT("Mac"));
#elif WITH_EDITORONLY_DATA && PLATFORM_LINUX
static const auto DefaultPlatform = MakeShared<FWwisePlatformId, ESPMode::ThreadSafe>(FGuid(0xBD0BDF13, 0x3125454F, 0x8BFD3195, 0x37169F81), TEXT("Linux"), TEXT("Linux"));
#else
static const auto DefaultPlatform = MakeShared<FWwisePlatformId, ESPMode::ThreadSafe>(FGuid(0x6E0CB257, 0xC6C84C5C, 0x83662740, 0xDFC441EC), TEXT("Windows"));
#endif

FWwiseResourceLoader::FWwiseResourceLoader()
{
	CurrentPlatform = SystemPlatform();
}

void FWwiseResourceLoader::SetLanguage(FWwiseLanguageCookedData InLanguage, EWwiseReloadLanguage InReloadLanguage)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::SetLanguage"));
	SetLanguageAsync(InLanguage, InReloadLanguage).Wait();
}

void FWwiseResourceLoader::PrepareResourceLoaderForPlatform(const FWwiseResourceLoader& InDefaultResourceLoader, const FWwiseSharedPlatformId& InPlatform)
{
	CurrentPlatform = InPlatform;
}

FWwiseLanguageCookedData FWwiseResourceLoader::GetCurrentLanguage() const
{
	return CurrentLanguage;
}

FWwiseSharedPlatformId FWwiseResourceLoader::GetCurrentPlatform() const
{
	return CurrentPlatform;
}

//
// User-facing loading and unloading operations
//

FWwiseLoadedAssetLibraryPtr FWwiseResourceLoader::LoadAssetLibrary(const FWwiseAssetLibraryCookedData& InAssetLibraryCookedData)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadAssetLibrary"));
	return LoadAssetLibraryAsync(InAssetLibraryCookedData).Get();
}

void FWwiseResourceLoader::UnloadAssetLibrary(FWwiseLoadedAssetLibraryPtr&& InAssetLibrary)
{
}

FWwiseLoadedAuxBusPtr FWwiseResourceLoader::LoadAuxBus(const FWwiseLocalizedAuxBusCookedData& InAuxBusCookedData,
	const FWwiseLanguageCookedData* InLanguageOverride)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadAuxBus"));
	return LoadAuxBusAsync(InAuxBusCookedData, InLanguageOverride).Get();
}

void FWwiseResourceLoader::UnloadAuxBus(FWwiseLoadedAuxBusPtr&& InAuxBus)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadAuxBus"));
	FWwiseLoadedAuxBusPromise Promise;
	Promise.EmplaceValue(MoveTemp(InAuxBus));
	UnloadAuxBusAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedEventPtr FWwiseResourceLoader::LoadEvent(const FWwiseLocalizedEventCookedData& InEventCookedData,
	const FWwiseLanguageCookedData* InLanguageOverride)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadEvent"));
	return LoadEventAsync(InEventCookedData, InLanguageOverride).Get();
}

void FWwiseResourceLoader::UnloadEvent(FWwiseLoadedEventPtr&& InEvent)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadEvent"));
	FWwiseLoadedEventPromise Promise;
	Promise.EmplaceValue(MoveTemp(InEvent));
	UnloadEventAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedExternalSourcePtr FWwiseResourceLoader::LoadExternalSource(
	const FWwiseExternalSourceCookedData& InExternalSourceCookedData)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadExternalSource"));
	return LoadExternalSourceAsync(InExternalSourceCookedData).Get();
}

void FWwiseResourceLoader::UnloadExternalSource(FWwiseLoadedExternalSourcePtr&& InExternalSource)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadExternalSource"));
	FWwiseLoadedExternalSourcePromise Promise;
	Promise.EmplaceValue(MoveTemp(InExternalSource));
	UnloadExternalSourceAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedGroupValuePtr FWwiseResourceLoader::LoadGroupValue(const FWwiseGroupValueCookedData& InGroupValueCookedData)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadGroupValue"));
	return LoadGroupValueAsync(InGroupValueCookedData).Get();
}

void FWwiseResourceLoader::UnloadGroupValue(FWwiseLoadedGroupValuePtr&& InGroupValue)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadGroupValue"));
	FWwiseLoadedGroupValuePromise Promise;
	Promise.EmplaceValue(MoveTemp(InGroupValue));
	UnloadGroupValueAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedInitBankPtr FWwiseResourceLoader::LoadInitBank(const FWwiseInitBankCookedData& InInitBankCookedData)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadInitBank"));
	return LoadInitBankAsync(InInitBankCookedData).Get();
}

void FWwiseResourceLoader::UnloadInitBank(FWwiseLoadedInitBankPtr&& InInitBank)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadInitBank"));
	FWwiseLoadedInitBankPromise Promise;
	Promise.EmplaceValue(MoveTemp(InInitBank));
	UnloadInitBankAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedMediaPtr FWwiseResourceLoader::LoadMedia(const FWwiseMediaCookedData& InMediaCookedData)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadMedia"));
	return LoadMediaAsync(InMediaCookedData).Get();
}

void FWwiseResourceLoader::UnloadMedia(FWwiseLoadedMediaPtr&& InMedia)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadMedia"));
	FWwiseLoadedMediaPromise Promise;
	Promise.EmplaceValue(MoveTemp(InMedia));
	UnloadMediaAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedShareSetPtr FWwiseResourceLoader::LoadShareSet(const FWwiseLocalizedShareSetCookedData& InShareSetCookedData,
	const FWwiseLanguageCookedData* InLanguageOverride)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadShareSet"));
	return LoadShareSetAsync(InShareSetCookedData, InLanguageOverride).Get();
}

void FWwiseResourceLoader::UnloadShareSet(FWwiseLoadedShareSetPtr&& InShareSet)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadShareSet"));
	FWwiseLoadedShareSetPromise Promise;
	Promise.EmplaceValue(MoveTemp(InShareSet));
	UnloadShareSetAsync(Promise.GetFuture()).Wait();
}

FWwiseLoadedSoundBankPtr FWwiseResourceLoader::LoadSoundBank(
	const FWwiseLocalizedSoundBankCookedData& InSoundBankCookedData, const FWwiseLanguageCookedData* InLanguageOverride)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::LoadSoundBank"));
	return LoadSoundBankAsync(InSoundBankCookedData, InLanguageOverride).Get();
}

void FWwiseResourceLoader::UnloadSoundBank(FWwiseLoadedSoundBankPtr&& InSoundBank)
{
	SCOPED_WWISERESOURCELOADER_EVENT_4(TEXT("FWwiseResourceLoader::UnloadSoundBank"));
	FWwiseLoadedSoundBankPromise Promise;
	Promise.EmplaceValue(MoveTemp(InSoundBank));
	UnloadSoundBankAsync(Promise.GetFuture()).Wait();
}

FWwiseResourceLoader::FWwiseSetLanguageFuture FWwiseResourceLoader::SetLanguageAsync(
	FWwiseLanguageCookedData InLanguage, EWwiseReloadLanguage InReloadLanguage)
{
	FWwiseSetLanguagePromise Promise;
	auto Future = Promise.GetFuture();
	UpdateLanguage(MoveTemp(Promise), InLanguage, InReloadLanguage);
	return Future;
}

FWwiseLoadedAssetLibraryFuture FWwiseResourceLoader::LoadAssetLibraryAsync(const FWwiseAssetLibraryCookedData& InAssetLibraryCookedData)
{
	FWwiseLoadedAssetLibraryPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		auto* AssetLibraryNode = CreateAssetLibraryNode(InAssetLibraryCookedData);
		if (UNLIKELY(!AssetLibraryNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadAssetLibraryNode(MoveTemp(Promise), MoveTemp(AssetLibraryNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadAssetLibraryAsync(FWwiseLoadedAssetLibraryFuture&& InAssetLibrary)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InAssetLibrary.IsReady() && InAssetLibrary.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InAssetLibrary.IsReady()))
	{
		auto* AssetLibrary = InAssetLibrary.Get();
		UnloadAssetLibraryNode(MoveTemp(Promise), MoveTemp(AssetLibrary));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadAssetLibraryAsync"), [WeakThis=AsWeak(), InAssetLibrary = MoveTemp(InAssetLibrary), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InAssetLibrary.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for Asset Library load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for an Asset Library to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for an Asset Library to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* AssetLibrary = InAssetLibrary.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadAssetLibraryAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!AssetLibrary))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadAssetLibraryNode(MoveTemp(Promise), MoveTemp(AssetLibrary));
			}
		});
	}

	return Future;
}

FWwiseLoadedAuxBusFuture FWwiseResourceLoader::LoadAuxBusAsync(const FWwiseLocalizedAuxBusCookedData& InAuxBusCookedData,
                                                               const FWwiseLanguageCookedData* InLanguageOverride)
{
	FWwiseLoadedAuxBusPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_AuxBusses);
		auto* AuxBusNode = CreateAuxBusNode(InAuxBusCookedData, InLanguageOverride);
		if (UNLIKELY(!AuxBusNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadAuxBusNode(MoveTemp(Promise), MoveTemp(AuxBusNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadAuxBusAsync(FWwiseLoadedAuxBusFuture&& InAuxBus)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InAuxBus.IsReady() && InAuxBus.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InAuxBus.IsReady()))
	{
		auto* AuxBus = InAuxBus.Get();
		UnloadAuxBusNode(MoveTemp(Promise), MoveTemp(AuxBus));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadAuxBusAsync"), [WeakThis=AsWeak(), InAuxBus = MoveTemp(InAuxBus), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InAuxBus.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for Aux Bus load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for an Aux Bus to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for an Aux Bus to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* AuxBus = InAuxBus.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadAuxBusAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!AuxBus))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadAuxBusNode(MoveTemp(Promise), MoveTemp(AuxBus));
			}
		});
	}

	return Future;
}

FWwiseLoadedEventFuture FWwiseResourceLoader::LoadEventAsync(const FWwiseLocalizedEventCookedData& InEventCookedData,
                                                       const FWwiseLanguageCookedData* InLanguageOverride)
{
	FWwiseLoadedEventPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_Events);
		auto* EventNode = CreateEventNode(InEventCookedData, InLanguageOverride);
		if (UNLIKELY(!EventNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadEventNode(MoveTemp(Promise), MoveTemp(EventNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadEventAsync(FWwiseLoadedEventFuture&& InEvent)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InEvent.IsReady() && InEvent.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InEvent.IsReady()))
	{
		auto* Event = InEvent.Get();
		UnloadEventNode(MoveTemp(Promise), MoveTemp(Event));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadEventAsync"), [WeakThis=AsWeak(), InEvent = MoveTemp(InEvent), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InEvent.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for Event load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a Event to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a Event to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* Event = InEvent.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadEventAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!Event))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadEventNode(MoveTemp(Promise), MoveTemp(Event));
			}
		});
	}

	return Future;
}

FWwiseLoadedExternalSourceFuture FWwiseResourceLoader::LoadExternalSourceAsync(
	const FWwiseExternalSourceCookedData& InExternalSourceCookedData)
{
	FWwiseLoadedExternalSourcePromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_ExternalSources);
		auto* ExternalSourceNode = CreateExternalSourceNode(InExternalSourceCookedData);
		if (UNLIKELY(!ExternalSourceNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadExternalSourceNode(MoveTemp(Promise), MoveTemp(ExternalSourceNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadExternalSourceAsync(FWwiseLoadedExternalSourceFuture&& InExternalSource)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InExternalSource.IsReady() && InExternalSource.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InExternalSource.IsReady()))
	{
		auto* ExternalSource = InExternalSource.Get();
		UnloadExternalSourceNode(MoveTemp(Promise), MoveTemp(ExternalSource));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadExternalSourceAsync"), [WeakThis=AsWeak(), InExternalSource = MoveTemp(InExternalSource), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InExternalSource.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for External Source load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a External Source to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a External Source to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* ExternalSource = InExternalSource.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadExternalSourceAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!ExternalSource))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadExternalSourceNode(MoveTemp(Promise), MoveTemp(ExternalSource));
			}
		});
	}

	return Future;
}

FWwiseLoadedGroupValueFuture FWwiseResourceLoader::LoadGroupValueAsync(const FWwiseGroupValueCookedData& InGroupValueCookedData)
{
	FWwiseLoadedGroupValuePromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_GroupValues);
		auto* GroupValueNode = CreateGroupValueNode(InGroupValueCookedData);
		if (UNLIKELY(!GroupValueNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadGroupValueNode(MoveTemp(Promise), MoveTemp(GroupValueNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadGroupValueAsync(FWwiseLoadedGroupValueFuture&& InGroupValue)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InGroupValue.IsReady() && InGroupValue.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InGroupValue.IsReady()))
	{
		auto* GroupValue = InGroupValue.Get();
		UnloadGroupValueNode(MoveTemp(Promise), MoveTemp(GroupValue));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadGroupValueAsync"), [WeakThis=AsWeak(), InGroupValue = MoveTemp(InGroupValue), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InGroupValue.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for Group Value load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a Group Value to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a Group Value to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* GroupValue = InGroupValue.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadGroupValueAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!GroupValue))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadGroupValueNode(MoveTemp(Promise), MoveTemp(GroupValue));
			}
		});
	}

	return Future;
}

FWwiseLoadedInitBankFuture FWwiseResourceLoader::LoadInitBankAsync(const FWwiseInitBankCookedData& InInitBankCookedData)
{
	FWwiseLoadedInitBankPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_InitBanks);
		auto* InitBankNode = CreateInitBankNode(InInitBankCookedData);
		if (UNLIKELY(!InitBankNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadInitBankNode(MoveTemp(Promise), MoveTemp(InitBankNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadInitBankAsync(FWwiseLoadedInitBankFuture&& InInitBank)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InInitBank.IsReady() && InInitBank.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InInitBank.IsReady()))
	{
		auto* InitBank = InInitBank.Get();
		UnloadInitBankNode(MoveTemp(Promise), MoveTemp(InitBank));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadInitBankAsync"), [WeakThis=AsWeak(), InInitBank = MoveTemp(InInitBank), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InInitBank.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for Init Bank load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a Init Bank to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a Init Bank to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* InitBank = InInitBank.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadInitBankAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!InitBank))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadInitBankNode(MoveTemp(Promise), MoveTemp(InitBank));
			}
		});
	}

	return Future;
}

FWwiseLoadedMediaFuture FWwiseResourceLoader::LoadMediaAsync(const FWwiseMediaCookedData& InMediaCookedData)
{
	FWwiseLoadedMediaPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_Media);
		auto* MediaNode = CreateMediaNode(InMediaCookedData);
		if (UNLIKELY(!MediaNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadMediaNode(MoveTemp(Promise), MoveTemp(MediaNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadMediaAsync(FWwiseLoadedMediaFuture&& InMedia)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InMedia.IsReady() && InMedia.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InMedia.IsReady()))
	{
		auto* Media = InMedia.Get();
		UnloadMediaNode(MoveTemp(Promise), MoveTemp(Media));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadMediaAsync"), [WeakThis=AsWeak(), InMedia = MoveTemp(InMedia), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InMedia.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for Media load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a Media to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a Media to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* Media = InMedia.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadMediaAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!Media))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadMediaNode(MoveTemp(Promise), MoveTemp(Media));
			}
		});
	}

	return Future;
}

FWwiseLoadedShareSetFuture FWwiseResourceLoader::LoadShareSetAsync(
	const FWwiseLocalizedShareSetCookedData& InShareSetCookedData, const FWwiseLanguageCookedData* InLanguageOverride)
{
	FWwiseLoadedShareSetPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_ShareSets);
		auto* ShareSetNode = CreateShareSetNode(InShareSetCookedData, InLanguageOverride);
		if (UNLIKELY(!ShareSetNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadShareSetNode(MoveTemp(Promise), MoveTemp(ShareSetNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadShareSetAsync(FWwiseLoadedShareSetFuture&& InShareSet)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InShareSet.IsReady() && InShareSet.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InShareSet.IsReady()))
	{
		auto* ShareSet = InShareSet.Get();
		UnloadShareSetNode(MoveTemp(Promise), MoveTemp(ShareSet));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadShareSetAsync"), [WeakThis=AsWeak(), InShareSet = MoveTemp(InShareSet), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InShareSet.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for ShareSet load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a ShareSet to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a ShareSet to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* ShareSet = InShareSet.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadShareSetAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!ShareSet))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadShareSetNode(MoveTemp(Promise), MoveTemp(ShareSet));
			}
		});
	}

	return Future;
}

FWwiseLoadedSoundBankFuture FWwiseResourceLoader::LoadSoundBankAsync(
	const FWwiseLocalizedSoundBankCookedData& InSoundBankCookedData, const FWwiseLanguageCookedData* InLanguageOverride)
{
	FWwiseLoadedSoundBankPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue(nullptr);
	}
	else
	{
		LLM_SCOPE_BYTAG(Audio_Wwise_ResourceLoader_SoundBanks);
		auto* SoundBankNode = CreateSoundBankNode(InSoundBankCookedData, InLanguageOverride);
		if (UNLIKELY(!SoundBankNode))
		{
			Promise.EmplaceValue(nullptr);
		}
		else
		{
			LoadSoundBankNode(MoveTemp(Promise), MoveTemp(SoundBankNode));
		}
	}

	return Future;
}

FWwiseResourceUnloadFuture FWwiseResourceLoader::UnloadSoundBankAsync(FWwiseLoadedSoundBankFuture&& InSoundBank)
{
	FWwiseResourceUnloadPromise Promise;
	auto Future = Promise.GetFuture();

	if (!IsEnabled())
	{
		UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
		Promise.EmplaceValue();
	}
	else if (UNLIKELY(InSoundBank.IsReady() && InSoundBank.Get() == nullptr))
	{
		Promise.EmplaceValue();
	}
	else if (LIKELY(InSoundBank.IsReady()))
	{
		auto* SoundBank = InSoundBank.Get();
		UnloadSoundBankNode(MoveTemp(Promise), MoveTemp(SoundBank));
	}
	else
	{
		LaunchWwiseTask(WWISERESOURCELOADER_ASYNC_NAME("FWwiseResourceLoader::UnloadSoundBankAsync"), [WeakThis=AsWeak(), InSoundBank = MoveTemp(InSoundBank), Promise = MoveTemp(Promise)]() mutable
		{
			{
				int WaitCount = 0;
				while (!InSoundBank.WaitFor(FTimespan::FromSeconds(1.0)))
				{
					if (IsEngineExitRequested())
					{
						UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Giving up on waiting for SoundBank load since we are exiting. Gave up on count [%d]"), WaitCount);
						Promise.EmplaceValue();
						return;
					}
					else
					{
						UE_CLOG(WaitCount != 10, LogWwiseResourceLoader, Verbose, TEXT("Waiting for a SoundBank to be fully loaded so we can unload it [%d]"), WaitCount);
						UE_CLOG(WaitCount == 10, LogWwiseResourceLoader, Warning, TEXT("Waited 10 seconds for a SoundBank to be loaded so we can unload it."));
						++WaitCount;
					}
				}
			}
			auto* SoundBank = InSoundBank.Get();

			auto SharedResourceLoader = WeakThis.Pin();
			if (!SharedResourceLoader.IsValid())
			{
				UE_LOG(LogWwiseResourceLoader, Error, TEXT("FWwiseResourceLoader::UnloadInitBankAsync: Failed. ResourceLoader is not valid"))
				return Promise.EmplaceValue();
			}
			if (!SharedResourceLoader->IsEnabled())
			{
				UE_LOG(LogWwiseResourceLoader, Warning, TEXT("ResourceLoader is disabled"));
				Promise.EmplaceValue();
			}
			else if (UNLIKELY(!SoundBank))
			{
				Promise.EmplaceValue();
			}
			else
			{
				SharedResourceLoader->UnloadSoundBankNode(MoveTemp(Promise), MoveTemp(SoundBank));
			}
		});
	}

	return Future;
}

//
// Basic info
//

FWwiseSharedPlatformId FWwiseResourceLoader::SystemPlatform() const
{
	auto Result = FWwiseSharedPlatformId();
	Result.Platform = DefaultPlatform;
	return Result;
}
