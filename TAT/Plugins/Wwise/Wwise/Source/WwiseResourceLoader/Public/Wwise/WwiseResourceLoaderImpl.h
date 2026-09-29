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

#include "Wwise/Stats/ResourceLoader.h"
#include "Wwise/WwiseExecutionQueue.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseResourceLoaderState.h"
#include "Wwise/WwiseSwitchContainerLoadedGroupValueInfo.h"

/**
 * @brief Operations available to manage and handle Wwise SoundBanks in Unreal.
 * Default implementation.
*/
class WWISERESOURCELOADER_API FWwiseResourceLoaderImpl : public FWwiseResourceLoader
{
public:
	using FWwiseSetLanguageFuture = TWwiseFuture<void>;
	using FWwiseSetLanguagePromise = TWwisePromise<void>;

	EWwiseResourceLoaderState WwiseResourceLoaderState = EWwiseResourceLoaderState::Enabled;

	/**
	 * Constructor used when creating a default Resource Loader implementation. You should not create or use one yourself,
	 * and you should use the WwiseResourceLoader::Get class accessor to retrieve a valid Resource Loader instance.
	 *
	 * This can be called by derived classes if you want to modify the default Resource Loader behavior.
	 */
	FWwiseResourceLoaderImpl();
	
	/**
	 * Constructor used for mock testing the Resource Loader. Hard-codes the different managers.
	 * 
	 * @param ExternalSourceManager External Source Manager file handler to be used in this instance
	 * @param MediaManager Media Manager file handler to be used in this instance
	 * @param SoundBankManager  SoundBank Manager file handler to be used in this instance
	 */
	FWwiseResourceLoaderImpl(const TSharedRef<IWwiseExternalSourceManager>& ExternalSourceManager,
	                         const TSharedRef<IWwiseMediaManager>& MediaManager,
	                         const TSharedRef<IWwiseSoundBankManager>& SoundBankManager);

	virtual ~FWwiseResourceLoaderImpl() override;

	virtual EWwiseResourceLoaderState GetResourceLoaderState();
	virtual void SetResourceLoaderState(EWwiseResourceLoaderState State);
	virtual bool IsEnabled() const override;
	virtual void Disable() override;
	virtual void Enable() override;

	virtual void UpdateLanguage(FWwiseSetLanguagePromise&& Promise, const FWwiseLanguageCookedData& InLanguage, EWwiseReloadLanguage InReloadLanguage) override;
	virtual void SetPlatform(const FWwiseSharedPlatformId& InPlatform) override;

	virtual FWwiseLoadedAssetLibraryPtr CreateAssetLibraryNode(const FWwiseAssetLibraryCookedData& InAssetLibraryCookedData) override;
	virtual void LoadAssetLibraryNode(FWwiseLoadedAssetLibraryPromise&& Promise, FWwiseLoadedAssetLibraryPtr&& InAssetLibraryListNode) override;
	virtual void UnloadAssetLibraryNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedAssetLibraryPtr&& InAssetLibraryListNode) override;

	virtual FWwiseLoadedAuxBusPtr CreateAuxBusNode(const FWwiseLocalizedAuxBusCookedData& InAuxBusCookedData, const FWwiseLanguageCookedData* InLanguageOverride) override;
	virtual void LoadAuxBusNode(FWwiseLoadedAuxBusPromise&& Promise, FWwiseLoadedAuxBusPtr&& InAuxBusListNode) override;
	virtual void UnloadAuxBusNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedAuxBusPtr&& InAuxBusListNode) override;

	virtual FWwiseLoadedEventPtr CreateEventNode(const FWwiseLocalizedEventCookedData& InEventCookedData, const FWwiseLanguageCookedData* InLanguageOverride) override;
	virtual void LoadEventNode(FWwiseLoadedEventPromise&& Promise, FWwiseLoadedEventPtr&& InEventListNode) override;
	virtual void UnloadEventNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedEventPtr&& InEventListNode) override;

	virtual FWwiseLoadedExternalSourcePtr CreateExternalSourceNode(const FWwiseExternalSourceCookedData& InExternalSourceCookedData) override;
	virtual void LoadExternalSourceNode(FWwiseLoadedExternalSourcePromise&& Promise, FWwiseLoadedExternalSourcePtr&& InExternalSourceListNode) override;
	virtual void UnloadExternalSourceNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedExternalSourcePtr&& InExternalSourceListNode) override;

	virtual FWwiseLoadedGroupValuePtr CreateGroupValueNode(const FWwiseGroupValueCookedData& InGroupValueCookedData) override;
	virtual void LoadGroupValueNode(FWwiseLoadedGroupValuePromise&& Promise, FWwiseLoadedGroupValuePtr&& InGroupValueListNode) override;
	virtual void UnloadGroupValueNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedGroupValuePtr&& InGroupValueListNode) override;

	virtual FWwiseLoadedInitBankPtr CreateInitBankNode(const FWwiseInitBankCookedData& InInitBankCookedData) override;
	virtual void LoadInitBankNode(FWwiseLoadedInitBankPromise&& Promise, FWwiseLoadedInitBankPtr&& InInitBankListNode) override;
	virtual void UnloadInitBankNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedInitBankPtr&& InInitBankListNode) override;

	virtual FWwiseLoadedMediaPtr CreateMediaNode(const FWwiseMediaCookedData& InMediaCookedData) override;
	virtual void LoadMediaNode(FWwiseLoadedMediaPromise&& Promise, FWwiseLoadedMediaPtr&& InMediaListNode) override;
	virtual void UnloadMediaNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedMediaPtr&& InMediaListNode) override;

	virtual FWwiseLoadedShareSetPtr CreateShareSetNode(const FWwiseLocalizedShareSetCookedData& InShareSetCookedData, const FWwiseLanguageCookedData* InLanguageOverride) override;
	virtual void LoadShareSetNode(FWwiseLoadedShareSetPromise&& Promise, FWwiseLoadedShareSetPtr&& InShareSetListNode) override;
	virtual void UnloadShareSetNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedShareSetPtr&& InShareSetListNode) override;

	virtual FWwiseLoadedSoundBankPtr CreateSoundBankNode(const FWwiseLocalizedSoundBankCookedData& InSoundBankCookedData, const FWwiseLanguageCookedData* InLanguageOverride) override;
	virtual void LoadSoundBankNode(FWwiseLoadedSoundBankPromise&& Promise, FWwiseLoadedSoundBankPtr&& InSoundBankListNode) override;
	virtual void UnloadSoundBankNode(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedSoundBankPtr&& InSoundBankListNode) override;

	// Checks whether no element remains pending in the Resource Loader at this point. Used for unit testing. 
	bool IsEmpty() const
	{
		return LoadedAuxBusList.Num() == 0
			&& LoadedEventList.Num() == 0
			&& LoadedExternalSourceList.Num() == 0
			&& LoadedGroupValueList.Num() == 0
			&& LoadedInitBankList.Num() == 0
			&& LoadedMediaList.Num() == 0
			&& LoadedShareSetList.Num() == 0
			&& LoadedSoundBankList.Num() == 0
			&& LoadedGroupValueInfo.Num() == 0;
	}

protected:
	using FLoadFileCallback = TUniqueFunction<void(bool bInResult)>;
	using FUnloadFileCallback = TUniqueFunction<void()>;
	using FCompletionCallback = TUniqueFunction<void()>;
	using FCompletionPromise = TWwisePromise<void>;
	using FCompletionFuture = TWwiseFuture<void>;
	using FCompletionFutureArray = TArray<FCompletionFuture>;

	FCriticalSection ListUpdateCriticalSection;
	
	/**
	 * @brief List of all the loaded Asset Library Wwise Objects.
	 *
	 * This list is maintained through the LoadAssetLibraryAsync and UnloadAssetLibraryAsync operations.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedAssetLibraryList LoadedAssetLibraryList;

	/**
	 * @brief List of all the loaded Auxiliary Bus Wwise Objects.
	 *
	 * This list is maintained through the LoadAuxBusAsync and UnloadAuxBusAsync operations.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedAuxBusList LoadedAuxBusList;

	/**
	 * @brief List of all the loaded Event Wwise Objects.
	 *
	 * This list is maintained through the LoadEventAsync and UnloadEventAsync operations.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedEventList LoadedEventList;

	/**
	 * @brief List of all the loaded External Source Wwise Objects.
	 *
	 * This list is maintained through the LoadExternalSourceAsync and UnloadExternalSourceAsync operations.
	 *
	 * External Sources are component parts of many other Wwise objects. Events, for example, can contain
	 * their own External Sources. This list only contains the External Sources that were added independently,
	 * not those that are already included in objects such as Events.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedExternalSourceList LoadedExternalSourceList;

	/**
	 * @brief List of all the loaded GroupValue (States, Switches) Wwise Objects.
	 *
	 * This list is maintained through the LoadGroupValueAsync and UnloadGroupValueAsync operations.
	 *
	 * GroupValues are component parts of many other Wwise objects. Events, for example, can contain
	 * their own GroupValues. This list only contains the GroupValues that were added independently,
	 * not those that are already included in objects such as Events.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedGroupValueList LoadedGroupValueList;

	/**
	 * @brief List of all the loaded Init Bank Wwise Objects.
	 *
	 * This list is maintained through the LoadInitBankAsync and UnloadInitBankAsync operations.
	 *
	 * @note In order to modify this list (add or remove), you must call the operation asynchronously through the
	 *       ListExecutionQueue.
	 *
	 * @warning Although this is a list, it can only include a single Init Bank. Each project supports one Init Bank,
	 *          which is used for the duration of the Sound Engine's lifespan.
	*/
	FWwiseLoadedInitBankList LoadedInitBankList;

	/**
	 * @brief List of all the loaded Media Wwise Objects.
	 *
	 * This list is maintained through the LoadMediaAsync and UnloadMediaAsync operations.
	 *
	 * Media objects are component parts of many other Wwise objects. Events, for example, can contain their
	 * own Media objects. This list only contains the Media objects that were added independently, not those
	 * that are already included in objects such as Events.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedMediaList LoadedMediaList;

	/**
	 * @brief List of all the loaded Share Set Wwise Objects.
	 *
	 * This list is maintained through the LoadShareSetAsync and UnloadShareSetAsync operations.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedShareSetList LoadedShareSetList;

	/**
	 * @brief List of all the loaded SoundBank Wwise Objects.
	 *
	 * This list is maintained through the LoadSoundBankAsync and UnloadSoundBankAsync operations.
	 *
	 * SoundBanks are building blocks of multiple other Wwise objects. An Event is included (and thus require) inside
	 * a SoundBank. This list only contains the independently added SoundBanks, as the different objects, such as
	 * Events, are responsible for keeping track of their own required sub-objects.
	 *
	 * @note To modify this list, you must call the operation asynchronously through ListExecutionQueue.
	*/
	FWwiseLoadedSoundBankList LoadedSoundBankList;


	/**
	 * @brief Set of all known GroupValues, each of which contains the list of the Switch Containers that require them.
	 *
	 * @note To modify this list, you must call the operation asynchronously through FileExecutionQueue.
	*/
	TSet<FWwiseSwitchContainerLoadedGroupValueInfo> LoadedGroupValueInfo;

	mutable FWwiseExecutionQueue* ExecutionQueue;

	mutable IWwiseExternalSourceManagerPtr ExternalSourceManager{nullptr};
	mutable IWwiseMediaManagerPtr MediaManager{nullptr};
	mutable IWwiseSoundBankManagerPtr SoundBankManager{nullptr};

	virtual void LoadAuxBusResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedAuxBusInfo::FLoadedData& LoadedData, const FWwiseAuxBusCookedData& InCookedData);
	virtual void LoadEventResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedEventInfo::FLoadedData& LoadedData, const FWwiseEventCookedData& InCookedData);
	virtual void LoadEventSwitchContainerResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedEventInfo::FLoadedData& LoadedData, const FWwiseEventCookedData& InCookedData);
	virtual void LoadExternalSourceResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedExternalSourceInfo::FLoadedData& LoadedData, const FWwiseExternalSourceCookedData& InCookedData);
	virtual void LoadGroupValueResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedGroupValueInfo::FLoadedData& LoadedData, const FWwiseGroupValueCookedData& InCookedData);
	virtual void LoadInitBankResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedInitBankInfo::FLoadedData& LoadedData, const FWwiseInitBankCookedData& InCookedData);
	virtual void LoadMediaResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedMediaInfo::FLoadedData& LoadedData, const FWwiseMediaCookedData& InCookedData);
	virtual void LoadShareSetResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedShareSetInfo::FLoadedData& LoadedData, const FWwiseShareSetCookedData& InCookedData);
	virtual void LoadSoundBankResources(FWwiseResourceLoadPromise&& Promise, FWwiseLoadedSoundBankInfo::FLoadedData& LoadedData, const FWwiseSoundBankCookedData& InCookedData);
	virtual void LoadSwitchContainerLeafResources(FCompletionPromise&& Promise, TSharedRef<FWwiseSwitchContainerLeafGroupValueUsageCount, ESPMode::ThreadSafe> UsageCount);

	virtual void UnloadAuxBusResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedAuxBusInfo::FLoadedData& LoadedData, const FWwiseAuxBusCookedData& InCookedData);
	virtual void UnloadEventResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedEventInfo::FLoadedData& LoadedData, const FWwiseEventCookedData& InCookedData);
	virtual void UnloadEventSwitchContainerResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedEventInfo::FLoadedData& LoadedData, const FWwiseEventCookedData& InCookedData);
	virtual void UnloadExternalSourceResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedExternalSourceInfo::FLoadedData& LoadedData, const FWwiseExternalSourceCookedData& InCookedData);
	virtual void UnloadGroupValueResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedGroupValueInfo::FLoadedData& LoadedData, const FWwiseGroupValueCookedData& InCookedData);
	virtual void UnloadInitBankResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedInitBankInfo::FLoadedData& LoadedData, const FWwiseInitBankCookedData& InCookedData);
	virtual void UnloadMediaResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedMediaInfo::FLoadedData& LoadedData, const FWwiseMediaCookedData& InCookedData);
	virtual void UnloadShareSetResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedShareSetInfo::FLoadedData& LoadedData, const FWwiseShareSetCookedData& InCookedData);
	virtual void UnloadSoundBankResources(FWwiseResourceUnloadPromise&& Promise, FWwiseLoadedSoundBankInfo::FLoadedData& LoadedData, const FWwiseSoundBankCookedData& InCookedData);
	virtual void UnloadSwitchContainerLeafResources(FWwiseResourceUnloadPromise&& Promise, TSharedRef<FWwiseSwitchContainerLeafGroupValueUsageCount, ESPMode::ThreadSafe> UsageCount);
	virtual void DeleteSwitchContainerLeafGroupValueUsageCount(FWwiseResourceUnloadPromise&& Promise, TSharedRef<FWwiseSwitchContainerLeafGroupValueUsageCount, ESPMode::ThreadSafe>& UsageCount);

	virtual void AttachAssetLibraryNode(FWwiseLoadedAssetLibraryListNode* AssetLibraryListNode);
	virtual void AttachAuxBusNode(FWwiseLoadedAuxBusListNode* AuxBusListNode);
	virtual void AttachEventNode(FWwiseLoadedEventListNode* EventListNode);
	virtual void AttachExternalSourceNode(FWwiseLoadedExternalSourceListNode* ExternalSourceListNode);
	virtual void AttachGroupValueNode(FWwiseLoadedGroupValueListNode* GroupValueListNode);
	virtual void AttachInitBankNode(FWwiseLoadedInitBankListNode* InitBankListNode);
	virtual void AttachMediaNode(FWwiseLoadedMediaListNode* MediaListNode);
	virtual void AttachShareSetNode(FWwiseLoadedShareSetListNode* ShareSetListNode);
	virtual void AttachSoundBankNode(FWwiseLoadedSoundBankListNode* SoundBankListNode);

	virtual void DetachAssetLibraryNode(FWwiseLoadedAssetLibraryListNode* AssetLibraryListNode);
	virtual void DetachAuxBusNode(FWwiseLoadedAuxBusListNode* AuxBusListNode);
	virtual void DetachEventNode(FWwiseLoadedEventListNode* EventListNode);
	virtual void DetachExternalSourceNode(FWwiseLoadedExternalSourceListNode* ExternalSourceListNode);
	virtual void DetachGroupValueNode(FWwiseLoadedGroupValueListNode* GroupValueListNode);
	virtual void DetachInitBankNode(FWwiseLoadedInitBankListNode* InitBankListNode);
	virtual void DetachMediaNode(FWwiseLoadedMediaListNode* MediaListNode);
	virtual void DetachShareSetNode(FWwiseLoadedShareSetListNode* ShareSetListNode);
	virtual void DetachSoundBankNode(FWwiseLoadedSoundBankListNode* SoundBankListNode);

	void AddLoadExternalSourceFutures(FCompletionFutureArray& FutureArray, TArray<const FWwiseExternalSourceCookedData*>& LoadedExternalSources,
	                                  const TArray<FWwiseExternalSourceCookedData>& InExternalSources, const TCHAR* InType, const FString& InDebugName, uint32 InShortId) const;
	void AddUnloadExternalSourceFutures(FCompletionFutureArray& FutureArray, TArray<const FWwiseExternalSourceCookedData*>& LoadedExternalSources,
										const TCHAR* InType, const FString& InDebugName, uint32 InShortId) const;
	void AddLoadMediaFutures(FCompletionFutureArray& FutureArray, TArray<const FWwiseMediaCookedData*>& LoadedMedia,
	                         const TArray<FWwiseMediaCookedData>& InMedia, const TCHAR* InType, const FString& InDebugName, uint32 InShortId) const;
	void AddUnloadMediaFutures(FCompletionFutureArray& FutureArray, TArray<const FWwiseMediaCookedData*>& LoadedMedia,
							   const TCHAR* InType, const FString& InDebugName, uint32 InShortId) const;
	void AddLoadSoundBankFutures(FCompletionFutureArray& FutureArray, TArray<const FWwiseSoundBankCookedData*>& LoadedSoundBanks,
	                             const TArray<FWwiseSoundBankCookedData>& InSoundBank, const TCHAR* InType, const FString& InDebugName, uint32 InShortId) const;
	void AddUnloadSoundBankFutures(FCompletionFutureArray& FutureArray, TArray<const FWwiseSoundBankCookedData*>& LoadedSoundBanks,
	                               const TCHAR* InType, const FString& InDebugName, uint32 InShortId) const;
	void WaitForFutures(FCompletionFutureArray&& FutureArray, FCompletionCallback&& Callback, int NextId = 0) const;

	void LoadSoundBankFile(const FWwiseSoundBankCookedData& InSoundBank, FLoadFileCallback&& InCallback) const;
	void UnloadSoundBankFile(const FWwiseSoundBankCookedData& InSoundBank, FUnloadFileCallback&& InCallback) const;
	void LoadMediaFile(const FWwiseMediaCookedData& InMedia, FLoadFileCallback&& InCallback) const;
	void UnloadMediaFile(const FWwiseMediaCookedData& InMedia, FUnloadFileCallback&& InCallback) const;
	void LoadExternalSourceFile(const FWwiseExternalSourceCookedData& InExternalSource, FLoadFileCallback&& InCallback) const;
	void UnloadExternalSourceFile(const FWwiseExternalSourceCookedData& InExternalSource, FUnloadFileCallback&& InCallback) const;

	template<typename MapValue>
	inline const FWwiseLanguageCookedData* GetLanguageMapKey(const TMap<FWwiseLanguageCookedData, MapValue>& Map, const FWwiseLanguageCookedData* InLanguageOverride, const FName& InDebugName) const;

	template<typename T>
	static inline void LogLoad(const T& Object);
	template<typename T>
	static inline void LogUnload(const T& Object);
	template<typename T>
	static inline void LogLoadResources(const T& Object);
	template<typename T>
	static inline void LogUnloadResources(const T& Object);
	template<typename T>
	static inline void LogLoadResources(const T& Object, void* Ptr);
	template<typename T>
	static inline void LogUnloadResources(const T& Object, void* Ptr);

public:
	struct WWISERESOURCELOADER_API Test
	{
#if defined(WITH_LOW_LEVEL_TESTS) && WITH_LOW_LEVEL_TESTS || defined(WITH_AUTOMATION_TESTS) || (WITH_DEV_AUTOMATION_TESTS || WITH_PERF_AUTOMATION_TESTS)
#define WWISE_RESOURCELOADERIMPL_TEST_CONST
#else
#define WWISE_RESOURCELOADERIMPL_TEST_CONST const
#endif
		static WWISE_RESOURCELOADERIMPL_TEST_CONST bool bMockSleepOnMediaLoad;
	};
};

template <typename T>
void FWwiseResourceLoaderImpl::LogLoad(const T& Object)
{
	UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Loading %s"), *Object.GetDebugString());
}

template <typename T>
void FWwiseResourceLoaderImpl::LogUnload(const T& Object)
{
	UE_LOG(LogWwiseResourceLoader, Verbose, TEXT("Unloading %s"), *Object.GetDebugString());
}

template <typename T>
void FWwiseResourceLoaderImpl::LogLoadResources(const T& Object)
{
	UE_LOG(LogWwiseResourceLoader, VeryVerbose, TEXT("Loading resources for %s"), *Object.GetDebugString());
}

template <typename T>
void FWwiseResourceLoaderImpl::LogUnloadResources(const T& Object)
{
	UE_LOG(LogWwiseResourceLoader, VeryVerbose, TEXT("Unloading resources for %s"), *Object.GetDebugString());
}

template <typename T>
void FWwiseResourceLoaderImpl::LogLoadResources(const T& Object, void* Ptr)
{
	UE_LOG(LogWwiseResourceLoader, VeryVerbose, TEXT("Loading resources for %s @ %p"), *Object.GetDebugString(), Ptr);
}

template <typename T>
void FWwiseResourceLoaderImpl::LogUnloadResources(const T& Object, void* Ptr)
{
	UE_LOG(LogWwiseResourceLoader, VeryVerbose, TEXT("Unloading resources for %s @ %p"), *Object.GetDebugString(), Ptr);
}

template<typename MapValue>
inline const FWwiseLanguageCookedData* FWwiseResourceLoaderImpl::GetLanguageMapKey(const TMap<FWwiseLanguageCookedData, MapValue>& Map, const FWwiseLanguageCookedData* InLanguageOverride, const FName& InDebugName) const
{
	if (InLanguageOverride)
	{
		if (Map.Find(*InLanguageOverride))
		{
			return InLanguageOverride;
		}
		UE_LOG(LogWwiseResourceLoader, Log, TEXT("GetLanguageMapKey: Could not find overridden language %s while processing asset %s. Defaulting to language %s"),
			*InLanguageOverride->GetLanguageName().ToString(), *InDebugName.ToString(), *CurrentLanguage.GetLanguageName().ToString());
	}

	if (LIKELY(Map.Contains(FWwiseLanguageCookedData::Sfx)))
	{
		return &FWwiseLanguageCookedData::Sfx;
	}

	if (Map.Find(CurrentLanguage))
	{
		return &CurrentLanguage;
	}

	UE_LOG(LogWwiseResourceLoader, Log, TEXT("GetLanguageMapKey: Could not find language %s while processing asset %s."),
		*CurrentLanguage.GetLanguageName().ToString(), *InDebugName.ToString());
	return nullptr;
}

