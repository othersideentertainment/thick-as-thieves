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

#pragma once

#include "Wwise/CookedData/WwiseLanguageCookedData.h"
#include "Wwise/Loaded/WwiseLoadedAssetLibrary.h"
#include "Wwise/Loaded/WwiseLoadedAuxBus.h"
#include "Wwise/Loaded/WwiseLoadedEvent.h"
#include "Wwise/Loaded/WwiseLoadedExternalSource.h"
#include "Wwise/Loaded/WwiseLoadedInitBank.h"
#include "Wwise/Loaded/WwiseLoadedMedia.h"
#include "Wwise/Loaded/WwiseLoadedShareSet.h"
#include "Wwise/Loaded/WwiseLoadedSoundBank.h"
#include "Wwise/WwiseFuture.h"
#include "Wwise/WwiseReloadLanguage.h"
#include "Wwise/WwiseResourceLoaderFuture.h"
#include "Wwise/WwiseResourceLoaderModule.h"
#include "Wwise/WwiseSharedPlatformId.h"

class IWwiseSoundBankManager;
class IWwiseExternalSourceManager;
class IWwiseMediaManager;
typedef TSharedPtr<IWwiseSoundBankManager> IWwiseSoundBankManagerPtr;
typedef TSharedPtr<IWwiseExternalSourceManager> IWwiseExternalSourceManagerPtr;
typedef TSharedPtr<IWwiseMediaManager> IWwiseMediaManagerPtr;

typedef TSharedPtr<FWwiseResourceLoader> FWwiseResourceLoaderPtr;
/**
 * @brief Operations available to manage and handle Wwise SoundBanks in Unreal.
 * Requires TSharedFromThis as operations using the Resource Loader can be executed after 
*/
class WWISERESOURCELOADER_API FWwiseResourceLoader: public TSharedFromThis<FWwiseResourceLoader>
{
public:
	using FWwiseSetLanguageFuture = TWwiseFuture<void>;
	using FWwiseSetLanguagePromise = TWwisePromise<void>;

	inline static FWwiseResourceLoaderPtr Get()
	{
		if (auto* Module = IWwiseResourceLoaderModule::GetModule())
		{
			return Module->GetResourceLoader();
		}
		return nullptr;
	}
	static FWwiseResourceLoaderPtr Instantiate()
	{
		if (auto* Module = IWwiseResourceLoaderModule::GetModule())
		{
			return Module->InstantiateResourceLoader();
		}
		return nullptr;
	}

#if WITH_EDITORONLY_DATA
	static FWwiseResourceLoaderPtr Instantiate(const FWwiseResourceLoader& InDefaultResourceLoader, const FWwiseSharedPlatformId& InPlatform)
	{
		if (FWwiseResourceLoaderPtr ResourceLoader = Instantiate())
		{
			ResourceLoader->PrepareResourceLoaderForPlatform(InDefaultResourceLoader, InPlatform);
			return ResourceLoader;
		}
		return nullptr;
	}
#endif

	FWwiseResourceLoader();
	virtual ~FWwiseResourceLoader() {}

	virtual bool IsEnabled() const = 0;
	virtual void Enable() = 0;
	virtual void Disable() = 0;

	virtual void PrepareResourceLoaderForPlatform(const FWwiseResourceLoader& InDefaultResourceLoader, const FWwiseSharedPlatformId& InPlatform);

	virtual FWwiseLanguageCookedData GetCurrentLanguage() const;
	virtual FWwiseSharedPlatformId GetCurrentPlatform() const;

	/**
	 * @brief Sets the language for the current runtime, optionally reloading all affected assets immediately
	 * @param InLanguage The current language being processed, or 0 if none
	 * @param InReloadLanguage What reload strategy should be used for language changes 
	*/
	virtual void SetLanguage(FWwiseLanguageCookedData InLanguage, EWwiseReloadLanguage InReloadLanguage);
	virtual void SetPlatform(const FWwiseSharedPlatformId& InPlatform) = 0;

	virtual FWwiseLoadedAssetLibraryPtr LoadAssetLibrary(const FWwiseAssetLibraryCookedData& InAssetLibraryCookedData);
	virtual void UnloadAssetLibrary(FWwiseLoadedAssetLibraryPtr&& InAssetLibrary);
	
	virtual FWwiseLoadedAuxBusPtr LoadAuxBus(const FWwiseLocalizedAuxBusCookedData& InAuxBusCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual void UnloadAuxBus(FWwiseLoadedAuxBusPtr&& InAuxBus);

	virtual FWwiseLoadedEventPtr LoadEvent(const FWwiseLocalizedEventCookedData& InEventCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual void UnloadEvent(FWwiseLoadedEventPtr&& InEvent);

	virtual FWwiseLoadedExternalSourcePtr LoadExternalSource(const FWwiseExternalSourceCookedData& InExternalSourceCookedData);
	virtual void UnloadExternalSource(FWwiseLoadedExternalSourcePtr&& InExternalSource);

	virtual FWwiseLoadedGroupValuePtr LoadGroupValue(const FWwiseGroupValueCookedData& InGroupValueCookedData);
	virtual void UnloadGroupValue(FWwiseLoadedGroupValuePtr&& InGroupValue);

	virtual FWwiseLoadedInitBankPtr LoadInitBank(const FWwiseInitBankCookedData& InInitBankCookedData);
	virtual void UnloadInitBank(FWwiseLoadedInitBankPtr&& InInitBank);

	virtual FWwiseLoadedMediaPtr LoadMedia(const FWwiseMediaCookedData& InMediaCookedData);
	virtual void UnloadMedia(FWwiseLoadedMediaPtr&& InMedia);

	virtual FWwiseLoadedShareSetPtr LoadShareSet(const FWwiseLocalizedShareSetCookedData& InShareSetCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual void UnloadShareSet(FWwiseLoadedShareSetPtr&& InShareSet);

	virtual FWwiseLoadedSoundBankPtr LoadSoundBank(const FWwiseLocalizedSoundBankCookedData& InSoundBankCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual void UnloadSoundBank(FWwiseLoadedSoundBankPtr&& InSoundBank);

	virtual FWwiseSetLanguageFuture SetLanguageAsync(FWwiseLanguageCookedData InLanguage, EWwiseReloadLanguage InReloadLanguage);

	virtual FWwiseLoadedAssetLibraryFuture LoadAssetLibraryAsync(const FWwiseAssetLibraryCookedData& InAssetLibraryCookedData);
	virtual FWwiseResourceUnloadFuture UnloadAssetLibraryAsync(FWwiseLoadedAssetLibraryFuture&& InAssetLibrary);
	
	virtual FWwiseLoadedAuxBusFuture LoadAuxBusAsync(const FWwiseLocalizedAuxBusCookedData& InAuxBusCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual FWwiseResourceUnloadFuture UnloadAuxBusAsync(FWwiseLoadedAuxBusFuture&& InAuxBus);

	virtual FWwiseLoadedEventFuture LoadEventAsync(const FWwiseLocalizedEventCookedData& InEventCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual FWwiseResourceUnloadFuture UnloadEventAsync(FWwiseLoadedEventFuture&& InEvent);

	virtual FWwiseLoadedExternalSourceFuture LoadExternalSourceAsync(const FWwiseExternalSourceCookedData& InExternalSourceCookedData);
	virtual FWwiseResourceUnloadFuture UnloadExternalSourceAsync(FWwiseLoadedExternalSourceFuture&& InExternalSource);

	virtual FWwiseLoadedGroupValueFuture LoadGroupValueAsync(const FWwiseGroupValueCookedData& InGroupValueCookedData);
	virtual FWwiseResourceUnloadFuture UnloadGroupValueAsync(FWwiseLoadedGroupValueFuture&& InGroupValue);

	virtual FWwiseLoadedInitBankFuture LoadInitBankAsync(const FWwiseInitBankCookedData& InInitBankCookedData);
	virtual FWwiseResourceUnloadFuture UnloadInitBankAsync(FWwiseLoadedInitBankFuture&& InInitBank);

	virtual FWwiseLoadedMediaFuture LoadMediaAsync(const FWwiseMediaCookedData& InMediaCookedData);
	virtual FWwiseResourceUnloadFuture UnloadMediaAsync(FWwiseLoadedMediaFuture&& InMedia);

	virtual FWwiseLoadedShareSetFuture LoadShareSetAsync(const FWwiseLocalizedShareSetCookedData& InShareSetCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual FWwiseResourceUnloadFuture UnloadShareSetAsync(FWwiseLoadedShareSetFuture&& InShareSet);

	virtual FWwiseLoadedSoundBankFuture LoadSoundBankAsync(const FWwiseLocalizedSoundBankCookedData& InSoundBankCookedData, const FWwiseLanguageCookedData* InLanguageOverride = nullptr);
	virtual FWwiseResourceUnloadFuture UnloadSoundBankAsync(FWwiseLoadedSoundBankFuture&& InSoundBank);

	virtual FWwiseSharedPlatformId SystemPlatform() const;

protected:
	/**
	 * @brief Currently targeted platform for this runtime
	 */
	FWwiseSharedPlatformId CurrentPlatform;

	/**
	 * @brief Currently targeted language for this runtime
	 */
	FWwiseLanguageCookedData CurrentLanguage;

	virtual void UpdateLanguage(FWwiseSetLanguagePromise&& Promise, const FWwiseLanguageCookedData& InLanguage, EWwiseReloadLanguage InReloadLanguage) = 0;

	virtual FWwiseLoadedAssetLibraryPtr CreateAssetLibraryNode(const FWwiseAssetLibraryCookedData& InAssetLibraryCookedData) = 0;
	virtual void LoadAssetLibraryNode(FWwiseLoadedAssetLibraryPromise&& Promise, FWwiseLoadedAssetLibraryPtr&& InAssetLibraryListNode) = 0;
	virtual void UnloadAssetLibraryNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedAssetLibraryPtr&& InAssetLibraryListNode) = 0;

	virtual FWwiseLoadedAuxBusPtr CreateAuxBusNode(const FWwiseLocalizedAuxBusCookedData& InAuxBusCookedData, const FWwiseLanguageCookedData* InLanguageOverride) = 0;
	virtual void LoadAuxBusNode(FWwiseLoadedAuxBusPromise&& Promise, FWwiseLoadedAuxBusPtr&& InAuxBusListNode) = 0;
	virtual void UnloadAuxBusNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedAuxBusPtr&& InAuxBusListNode) = 0;

	virtual FWwiseLoadedEventPtr CreateEventNode(const FWwiseLocalizedEventCookedData& InEventCookedData, const FWwiseLanguageCookedData* InLanguageOverride) = 0;
	virtual void LoadEventNode(FWwiseLoadedEventPromise&& Promise, FWwiseLoadedEventPtr&& InEventListNode) = 0;
	virtual void UnloadEventNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedEventPtr&& InEventListNode) = 0;

	virtual FWwiseLoadedExternalSourcePtr CreateExternalSourceNode(const FWwiseExternalSourceCookedData& InExternalSourceCookedData) = 0;
	virtual void LoadExternalSourceNode(FWwiseLoadedExternalSourcePromise&& Promise, FWwiseLoadedExternalSourcePtr&& InExternalSourceListNode) = 0;
	virtual void UnloadExternalSourceNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedExternalSourcePtr&& InExternalSourceListNode) = 0;

	virtual FWwiseLoadedGroupValuePtr CreateGroupValueNode(const FWwiseGroupValueCookedData& InGroupValueCookedData) = 0;
	virtual void LoadGroupValueNode(FWwiseLoadedGroupValuePromise&& Promise, FWwiseLoadedGroupValuePtr&& InGroupValueListNode) = 0;
	virtual void UnloadGroupValueNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedGroupValuePtr&& InGroupValueListNode) = 0;

	virtual FWwiseLoadedInitBankPtr CreateInitBankNode(const FWwiseInitBankCookedData& InInitBankCookedData) = 0;
	virtual void LoadInitBankNode(FWwiseLoadedInitBankPromise&& Promise, FWwiseLoadedInitBankPtr&& InInitBankListNode) = 0;
	virtual void UnloadInitBankNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedInitBankPtr&& InInitBankListNode) = 0;

	virtual FWwiseLoadedMediaPtr CreateMediaNode(const FWwiseMediaCookedData& InMediaCookedData) = 0;
	virtual void LoadMediaNode(FWwiseLoadedMediaPromise&& Promise, FWwiseLoadedMediaPtr&& InMediaListNode) = 0;
	virtual void UnloadMediaNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedMediaPtr&& InMediaListNode) = 0;

	virtual FWwiseLoadedShareSetPtr CreateShareSetNode(const FWwiseLocalizedShareSetCookedData& InShareSetCookedData, const FWwiseLanguageCookedData* InLanguageOverride) = 0;
	virtual void LoadShareSetNode(FWwiseLoadedShareSetPromise&& Promise, FWwiseLoadedShareSetPtr&& InShareSetListNode) = 0;
	virtual void UnloadShareSetNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedShareSetPtr&& InShareSetListNode) = 0;

	virtual FWwiseLoadedSoundBankPtr CreateSoundBankNode(const FWwiseLocalizedSoundBankCookedData& InSoundBankCookedData, const FWwiseLanguageCookedData* InLanguageOverride) = 0;
	virtual void LoadSoundBankNode(FWwiseLoadedSoundBankPromise&& Promise, FWwiseLoadedSoundBankPtr&& InSoundBankListNode) = 0;
	virtual void UnloadSoundBankNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedSoundBankPtr&& InSoundBankListNode) = 0;
};
