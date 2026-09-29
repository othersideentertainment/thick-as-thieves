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

#include <shared_mutex>

#include "Wwise/WwiseStringConverter.h"
#include "Wwise/WwiseDataStructure.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseProjectDatabaseModule.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/WwiseProjectDatabaseLogging.h"

#include "Misc/CommandLine.h"
#include "Wwise/Info/WwiseEventInfo.h"
#include "Wwise/Info/WwiseGroupValueInfo.h"

struct FWwiseObjectInfo;
class FWwiseProjectDatabase;
using FSharedWwiseDataStructure = TSharedRef<WwiseDataStructure, ESPMode::ThreadSafe>;

class WWISEPROJECTDATABASE_API WwiseDataStructureScopeLock
{
	std::shared_lock<std::shared_mutex> Lock;
public:
	WwiseDataStructureScopeLock(const FWwiseProjectDatabase& InProjectDatabase);

	~WwiseDataStructureScopeLock()
	{
		Lock.mutex()->unlock_shared();
	}
	
	const WwiseDataStructure& operator*() const
	{
		return DataStructure;
	}

	const WwiseDataStructure* operator->() const
	{
		return &DataStructure;
	}

	const WwiseAcousticTextureGlobalIdsMap& GetAcousticTextures() const;
	WwiseRefAcousticTexture GetAcousticTexture(const FWwiseObjectInfo& InInfo) const;

	const WwiseAudioDeviceGlobalIdsMap& GetAudioDevices() const;
	WwiseRefAudioDevice GetAudioDevice(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseAuxBusGlobalIdsMap& GetAuxBusses() const;
	WwiseRefAuxBus GetAuxBus(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseBusGlobalIdsMap& GetBusses() const;
	WwiseRefBus GetBus(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseCustomPluginGlobalIdsMap& GetCustomPlugins() const;
	WwiseRefCustomPlugin GetCustomPlugin(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseDialogueArgumentGlobalIdsMap& GetDialogueArguments() const;
	WwiseRefDialogueArgument GetDialogueArgument(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseDialogueEventGlobalIdsMap& GetDialogueEvents() const;
	WwiseRefDialogueEvent GetDialogueEvent(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseEventGlobalIdsMap& GetEvents() const;
	WwiseDBSet<WwiseRefEvent> GetEvent(const FWwiseEventInfo& InInfo) const;
	
	const WwiseExternalSourceGlobalIdsMap& GetExternalSources() const;
	WwiseRefExternalSource GetExternalSource(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseGameParameterGlobalIdsMap& GetGameParameters() const;
	WwiseRefGameParameter GetGameParameter(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseMediaGlobalIdsMap& GetMediaFiles() const;
	WwiseRefMedia GetMediaFile(const FWwiseObjectInfo& InInfo) const;
	
	const WwisePluginLibGlobalIdsMap& GetPluginLibs() const;
	WwiseRefPluginLib GetPluginLib(const FWwiseObjectInfo& InInfo) const;
	
	const WwisePluginShareSetGlobalIdsMap& GetPluginShareSets() const;
	WwiseRefPluginShareSet GetPluginShareSet(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseSoundBankGlobalIdsMap& GetSoundBanks() const;
	WwiseRefSoundBank GetSoundBank(const FWwiseObjectInfo& InInfo, uint32 InLanguageId = 0) const;
	
	const WwiseStateGlobalIdsMap& GetStates() const;
	WwiseRefState GetState(const FWwiseGroupValueInfo& InInfo) const;
	
	const WwiseStateGroupGlobalIdsMap& GetStateGroups() const;
	WwiseRefStateGroup GetStateGroup(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseSwitchGlobalIdsMap& GetSwitches() const;
	WwiseRefSwitch GetSwitch(const FWwiseGroupValueInfo& InInfo) const;
	
	const WwiseSwitchGroupGlobalIdsMap& GetSwitchGroups() const;
	WwiseRefSwitchGroup GetSwitchGroup(const FWwiseObjectInfo& InInfo) const;
	
	const WwiseTriggerGlobalIdsMap& GetTriggers() const;
	WwiseRefTrigger GetTrigger(const FWwiseObjectInfo& InInfo) const;

	const WwiseDBSet<WwiseDBSharedLanguageId>& GetLanguages() const;
	const WwiseDBSet<WwiseDBSharedPlatformId>& GetPlatforms() const;
	WwiseRefPlatform GetPlatform(const FWwiseSharedPlatformId& InPlatformId) const;

	const WwisePlatformDataStructure* GetCurrentPlatformData() const;

	const WwiseDBSharedLanguageId& GetCurrentLanguage() const { return CurrentLanguage; }
	const WwiseDBSharedPlatformId& GetCurrentPlatform() const { return CurrentPlatform; }
	bool DisableDefaultPlatforms() const { return bDisableDefaultPlatforms; }

	int GetUsageCount(const WwiseAnyRef& Asset) const;
	int GetMediaUsageCount(uint32 InId) const;
	int GetSoundBankUsageCount(uint32 InId, uint32 InLanguageId) const;
	int GetSoundBankUsageCount(uint32 InId, const WwiseDBString& InLanguage) const;

	uint32 GetLanguageId(const WwiseDBString& Name) const;
	WwiseDBString GetLanguageName(uint32 InId) const;

private:
	const WwiseDataStructure& DataStructure;

	WwiseDBSharedLanguageId CurrentLanguage;
	WwiseDBSharedPlatformId CurrentPlatform;
	bool bDisableDefaultPlatforms;

	UE_NONCOPYABLE(WwiseDataStructureScopeLock);
};

class WWISEPROJECTDATABASE_API WwiseDataStructureWriteScopeLock
{
	std::unique_lock<std::shared_mutex> Lock;
public:
	WwiseDataStructureWriteScopeLock(FWwiseProjectDatabase& InProjectDatabase);

	~WwiseDataStructureWriteScopeLock()
	{
		Lock.unlock();
	}
	
	WwiseDataStructure& operator*()
	{
		return DataStructure;
	}

	WwiseDataStructure* operator->()
	{
		return &DataStructure;
	}

private:
	WwiseDataStructure& DataStructure;
	UE_NONCOPYABLE(WwiseDataStructureWriteScopeLock);
};

class WWISEPROJECTDATABASE_API FWwiseProjectDatabase
{
	friend class WwiseDataStructureScopeLock;
	friend class WwiseDataStructureWriteScopeLock;

public:
	static const WwiseDBGuid BasePlatformGuid;

	inline static FWwiseProjectDatabase* Get()
	{
		if (auto* Module = IWwiseProjectDatabaseModule::GetModule())
		{
			if(Module->CanHaveDefaultInstance())
			{
				return Module->GetProjectDatabase();
			}
		}
		return nullptr;
	}
	static FWwiseProjectDatabase* Instantiate()
	{
		if (auto* Module = IWwiseProjectDatabaseModule::GetModule())
		{
			return Module->InstantiateProjectDatabase();
		}
		return nullptr;
	}

	static FWwiseProjectDatabase* Instantiate(const FWwiseProjectDatabase& DefaultProjectDatabase)
	{
		if (auto* ProjectDatabase = Instantiate())
		{
			ProjectDatabase->InitForStaging(DefaultProjectDatabase);
			return ProjectDatabase;
		}
		return nullptr;
	}


	FWwiseProjectDatabase() {}
	virtual ~FWwiseProjectDatabase() {}

	virtual void UpdateDataStructure(
		const WwiseDBGuid* InBasePlatformGuid = &BasePlatformGuid) {}

	virtual void PrepareProjectDatabaseForPlatform(FWwiseResourceLoaderPtr&& InResourceLoader) {}
	virtual FWwiseResourceLoaderPtr GetResourceLoader() { return nullptr; }
	virtual const FWwiseResourceLoaderPtr GetResourceLoader() const { return nullptr; }

	FWwiseSharedLanguageId GetCurrentLanguage() const;
	FWwiseSharedPlatformId GetCurrentPlatform() const;
	virtual bool IsProjectDatabaseParsed() const { return bIsDatabaseParsed; };

	/**
	 * Sets the path where the Generated SoundBanks are located, relative to the Unreal Content folder.
	 *
	 * This is typically used to synchronize the setting in UAkSettings.
	 * 
	 * @param DirectoryPath Path where the Generated SoundBanks are located.
	 */
	virtual void SetGeneratedSoundBanksPath(const FDirectoryPath& DirectoryPath);
	virtual const FDirectoryPath& GetGeneratedSoundBanksPath() const { return GeneratedSoundBanksPath; }
	virtual FString GetGeneratedSoundBanksPathFor(const FName& InPath) const { return GetGeneratedSoundBanksPathFor(InPath.ToString()); }
	virtual FString GetGeneratedSoundBanksPathFor(const FString& InPath) const { return GetGeneratedSoundBanksPath().Path / InPath; }

protected:
	/**
	 * This is called by Instantiate when a new Project Database is instantiated for staging purposes.
	 * 
	 * @param DefaultProjectDatabase The original Project Database
	 */
	virtual void InitForStaging(const FWwiseProjectDatabase& DefaultProjectDatabase);
	
	virtual FSharedWwiseDataStructure& GetLockedDataStructure() { check(false); UE_ASSUME(false); }
	virtual const FSharedWwiseDataStructure& GetLockedDataStructure() const { check(false); UE_ASSUME(false); }

	template <typename RequiredRef>
	bool GetRef(RequiredRef& OutRef, const FWwiseObjectInfo& InInfo)
	{
		const FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
		check(ResourceLoader);
		const auto& PlatformRef = ResourceLoader->GetCurrentPlatform();

		const auto& DataStructure = *GetLockedDataStructure();

		WwiseDBGuid Guid(PlatformRef.Platform->PlatformGuid.A, PlatformRef.Platform->PlatformGuid.B,
			PlatformRef.Platform->PlatformGuid.C, PlatformRef.Platform->PlatformGuid.D);
		const auto* Platform = DataStructure.Platforms.Find(WwiseDBSharedPlatformId(Guid, FWwiseStringConverter::ToWwiseDBString(PlatformRef.Platform->PlatformName.ToString())));
		if (UNLIKELY(!Platform))
		{
			WWISE_DB_LOG(Error, "GetRef: Platform not found");
			return false;
		}

		return Platform->GetRef(OutRef, InInfo);
	}

	bool DisableDefaultPlatforms() const;
	
	bool bIsDatabaseParsed = false;
	FDirectoryPath GeneratedSoundBanksPath;
};

inline WwiseDBObjectInfo ConvertWwiseObjectInfo(const FWwiseObjectInfo& InInfo)
{
	return WwiseDBObjectInfo
	(WwiseDBGuid(InInfo.WwiseGuid.A, InInfo.WwiseGuid.B, InInfo.WwiseGuid.C, InInfo.WwiseGuid.D), InInfo.WwiseShortId,
		FWwiseStringConverter::ToWwiseDBString(InInfo.WwiseName.ToString()), InInfo.HardCodedSoundBankShortId);
}

inline WwiseDBGroupValueInfo ConvertWwiseGroupValueInfo(const FWwiseGroupValueInfo& InInfo)
{
	return WwiseDBGroupValueInfo
		(WwiseDBGuid(InInfo.WwiseGuid.A, InInfo.WwiseGuid.B, InInfo.WwiseGuid.C, InInfo.WwiseGuid.D),
		InInfo.GroupShortId, InInfo.WwiseShortId,
		FWwiseStringConverter::ToWwiseDBString(InInfo.WwiseName.ToString()));
}