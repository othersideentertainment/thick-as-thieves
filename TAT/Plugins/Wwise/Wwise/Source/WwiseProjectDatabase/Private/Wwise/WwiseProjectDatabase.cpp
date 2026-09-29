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

#include "Wwise/WwiseProjectDatabase.h"

#include "WwiseUnrealHelper.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseProjectDatabaseDelegates.h"

#include "Async/Async.h"
#include "Misc/ScopedSlowTask.h"
#include "Wwise/WwiseStringConverter.h"
#include "Wwise/Metadata/WwiseMetadataLanguage.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Stats/Global.h"

#define LOCTEXT_NAMESPACE "WwiseProjectDatabase"

WwiseDataStructureScopeLock::WwiseDataStructureScopeLock(const FWwiseProjectDatabase& InProjectDatabase) :
	Lock(InProjectDatabase.GetLockedDataStructure()->Lock),
	DataStructure(*InProjectDatabase.GetLockedDataStructure()),
	bDisableDefaultPlatforms(InProjectDatabase.DisableDefaultPlatforms())
{
	auto Language = InProjectDatabase.GetCurrentLanguage();
	auto Platform = InProjectDatabase.GetCurrentPlatform();
	CurrentLanguage = WwiseDBSharedLanguageId(Language.GetLanguageId(), FWwiseStringConverter::ToWwiseDBString(Language.GetLanguageName().ToString()), (WwiseDBLanguageRequirement)Language.LanguageRequirement);
	FGuid Guid = Platform.GetPlatformGuid();
	CurrentPlatform = WwiseDBSharedPlatformId(WwiseDBGuid(Guid.A, Guid.B, Guid.C, Guid.D),
		FWwiseStringConverter::ToWwiseDBString(Platform.Platform->PlatformName.ToString()), FWwiseStringConverter::ToWwiseDBString(Platform.Platform->PathRelativeToGeneratedSoundBanks.ToString()));
	CurrentPlatform.Platform->ExternalSourceRootPath = FWwiseStringConverter::ToWwiseDBString(Platform.Platform->PathRelativeToGeneratedSoundBanks.ToString());
	Lock.mutex()->lock_shared();
}

const WwiseAcousticTextureGlobalIdsMap& WwiseDataStructureScopeLock::GetAcousticTextures() const
{
	static const auto Empty = WwiseAcousticTextureGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->AcousticTextures;
}

WwiseRefAcousticTexture WwiseDataStructureScopeLock::GetAcousticTexture(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefAcousticTexture Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseAudioDeviceGlobalIdsMap& WwiseDataStructureScopeLock::GetAudioDevices() const
{
	static const auto Empty = WwiseAudioDeviceGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->AudioDevices;
}

WwiseRefAudioDevice WwiseDataStructureScopeLock::GetAudioDevice(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefAudioDevice Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseAuxBusGlobalIdsMap& WwiseDataStructureScopeLock::GetAuxBusses() const
{
	static const auto Empty = WwiseAuxBusGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->AuxBusses;
}

WwiseRefAuxBus WwiseDataStructureScopeLock::GetAuxBus(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefAuxBus Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseBusGlobalIdsMap& WwiseDataStructureScopeLock::GetBusses() const
{
	static const auto Empty = WwiseBusGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->Busses;
}

WwiseRefBus WwiseDataStructureScopeLock::GetBus(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefBus Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseCustomPluginGlobalIdsMap& WwiseDataStructureScopeLock::GetCustomPlugins() const
{
	static const auto Empty = WwiseCustomPluginGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->CustomPlugins;
}

WwiseRefCustomPlugin WwiseDataStructureScopeLock::GetCustomPlugin(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefCustomPlugin Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseDialogueArgumentGlobalIdsMap& WwiseDataStructureScopeLock::GetDialogueArguments() const
{
	static const auto Empty = WwiseDialogueArgumentGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->DialogueArguments;
}

WwiseRefDialogueArgument WwiseDataStructureScopeLock::GetDialogueArgument(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefDialogueArgument Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseDialogueEventGlobalIdsMap& WwiseDataStructureScopeLock::GetDialogueEvents() const
{
	static const auto Empty = WwiseDialogueEventGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->DialogueEvents;
}

WwiseRefDialogueEvent WwiseDataStructureScopeLock::GetDialogueEvent(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefDialogueEvent Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseEventGlobalIdsMap& WwiseDataStructureScopeLock::GetEvents() const
{
	static const auto Empty = WwiseEventGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->Events;
}

WwiseDBSet<WwiseRefEvent> WwiseDataStructureScopeLock::GetEvent(const FWwiseEventInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseDBSet<WwiseRefEvent> Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseExternalSourceGlobalIdsMap& WwiseDataStructureScopeLock::GetExternalSources() const
{
	static const auto Empty = WwiseExternalSourceGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->ExternalSources;
}

WwiseRefExternalSource WwiseDataStructureScopeLock::GetExternalSource(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefExternalSource Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseGameParameterGlobalIdsMap& WwiseDataStructureScopeLock::GetGameParameters() const
{
	static const auto Empty = WwiseGameParameterGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->GameParameters;
}

WwiseRefGameParameter WwiseDataStructureScopeLock::GetGameParameter(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefGameParameter Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseMediaGlobalIdsMap& WwiseDataStructureScopeLock::GetMediaFiles() const
{
	static const auto Empty = WwiseMediaGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->MediaFiles;
}

WwiseRefMedia WwiseDataStructureScopeLock::GetMediaFile(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefMedia Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwisePluginLibGlobalIdsMap& WwiseDataStructureScopeLock::GetPluginLibs() const
{
	static const auto Empty = WwisePluginLibGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->PluginLibs;
}

WwiseRefPluginLib WwiseDataStructureScopeLock::GetPluginLib(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefPluginLib Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwisePluginShareSetGlobalIdsMap& WwiseDataStructureScopeLock::GetPluginShareSets() const
{
	static const auto Empty = WwisePluginShareSetGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->PluginShareSets;
}

WwiseRefPluginShareSet WwiseDataStructureScopeLock::GetPluginShareSet(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefPluginShareSet Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseSoundBankGlobalIdsMap& WwiseDataStructureScopeLock::GetSoundBanks() const
{
	static const auto Empty = WwiseSoundBankGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->SoundBanks;
}

WwiseRefSoundBank WwiseDataStructureScopeLock::GetSoundBank(const FWwiseObjectInfo& InInfo, uint32 LanguageId) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefSoundBank Result;
	const WwiseDBSharedLanguageId* LanguagePtr = &GetCurrentLanguage();
	if (LanguageId != 0)
	{
		const auto& Languages{ GetLanguages() };
		for (const auto& Language : Languages)
		{
			if (Language.GetLanguageId() == LanguageId)
			{
				LanguagePtr = &Language;
				break;
			}
		}
	}
		
	PlatformData->GetRef(Result, *LanguagePtr, ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseStateGlobalIdsMap& WwiseDataStructureScopeLock::GetStates() const
{
	static const auto Empty = WwiseStateGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->States;
}

WwiseRefState WwiseDataStructureScopeLock::GetState(const FWwiseGroupValueInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefState Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseGroupValueInfo(InInfo));
	return Result;
}

const WwiseStateGroupGlobalIdsMap& WwiseDataStructureScopeLock::GetStateGroups() const
{
	static const auto Empty = WwiseStateGroupGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->StateGroups;
}

WwiseRefStateGroup WwiseDataStructureScopeLock::GetStateGroup(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefStateGroup Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseSwitchGlobalIdsMap& WwiseDataStructureScopeLock::GetSwitches() const
{
	static const auto Empty = WwiseSwitchGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->Switches;
}

WwiseRefSwitch WwiseDataStructureScopeLock::GetSwitch(const FWwiseGroupValueInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefSwitch Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseGroupValueInfo(InInfo));
	return Result;
}

const WwiseSwitchGroupGlobalIdsMap& WwiseDataStructureScopeLock::GetSwitchGroups() const
{
	static const auto Empty = WwiseSwitchGroupGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->SwitchGroups;
}

WwiseRefSwitchGroup WwiseDataStructureScopeLock::GetSwitchGroup(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefSwitchGroup Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwiseTriggerGlobalIdsMap& WwiseDataStructureScopeLock::GetTriggers() const
{
	static const auto Empty = WwiseTriggerGlobalIdsMap();

	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return Empty;

	return PlatformData->Triggers;
}

WwiseRefTrigger WwiseDataStructureScopeLock::GetTrigger(const FWwiseObjectInfo& InInfo) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return {};

	WwiseRefTrigger Result;
	PlatformData->GetRef(Result, GetCurrentLanguage(), ConvertWwiseObjectInfo(InInfo));
	return Result;
}

const WwisePlatformDataStructure* WwiseDataStructureScopeLock::GetCurrentPlatformData() const
{
	if (DisableDefaultPlatforms())
	{
		WWISE_DB_LOG(VeryVerbose, "Trying to access current platform data when none is loaded by design (cooking)");
		return nullptr;
	}

	const auto& Platform = GetCurrentPlatform();
	const auto* PlatformData = DataStructure.Platforms.Find(Platform);

	if (UNLIKELY(!PlatformData))
	{
		if(Platform.GetPlatformName() != "None"_wwise_db)
		{
			UE_LOG(LogWwiseProjectDatabase, Error, TEXT("Current platform %s not found."), *FWwiseStringConverter::ToFString(Platform.GetPlatformName()));
		}
		else
		{
			UE_LOG(LogWwiseProjectDatabase, Error, TEXT("No JSON Metadata file found. Have SoundBanks been generated?"));
		}


		if (!UE_LOG_ACTIVE(LogWwiseProjectDatabase, Verbose) && !UE_LOG_ACTIVE(LogWwiseProjectDatabase, VeryVerbose))
		{
			UE_LOG(LogWwiseHints, Warning,
			       TEXT("Enable Verbose or VeryVerbose logs for LogWwiseProjectDatabase for more details on why %s is missing from your current platforms."),
			       *FWwiseStringConverter::ToFString(Platform.GetPlatformName()));
		}

		if (DataStructure.RootData.JsonFiles.Size() == 0 &&  Platform.GetPlatformName() != "None"_wwise_db)
		{
			FString SoundBankPath = WwiseUnrealHelper::GetSoundBankDirectory() / *Platform.Platform->PathRelativeToGeneratedSoundBanks;
			UE_LOG(LogWwiseProjectDatabase, Error,
			       TEXT("No JSON Metadata file found for platform %s at %s. Have SoundBanks been generated?"),
			       *SoundBankPath,
			       *FWwiseStringConverter::ToFString(Platform.GetPlatformName()));
		}

		return nullptr;
	}
	return PlatformData;
}

int WwiseDataStructureScopeLock::GetUsageCount(const WwiseAnyRef& Asset) const
{
	if (Asset.GetType() == WwiseRefType::Media)
	{
		return GetMediaUsageCount(Asset.GetId());
	}
	else if (Asset.GetType() == WwiseRefType::SoundBank)
	{
		const auto Language = Asset.GetLanguage();
		return GetSoundBankUsageCount(Asset.GetId(), Language ? Language->Id : 0);
	}
	else
	{
		return 0;
	}
}

int WwiseDataStructureScopeLock::GetMediaUsageCount(uint32 InId) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return 0;

	if (const auto* UsageCount = PlatformData->MediaUsageCount.Find(WwiseDatabaseMediaIdKey(InId, 0)))
	{
		return *UsageCount;
	}
	return 0;
}

int WwiseDataStructureScopeLock::GetSoundBankUsageCount(uint32 InId, uint32 InLanguageId) const
{
	const auto* PlatformData = GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData)) return 0;		// Cannot say. Err to safety.

	if (const auto* UsageCount = PlatformData->SoundBankUsageCount.Find(WwiseDatabaseLocalizableIdKey(InId, InLanguageId)))
	{
		return *UsageCount;
	}
	return 0;
}

int WwiseDataStructureScopeLock::GetSoundBankUsageCount(uint32 InId, const WwiseDBString& InLanguage) const
{
	return GetSoundBankUsageCount(InId, GetLanguageId(InLanguage));
}

uint32 WwiseDataStructureScopeLock::GetLanguageId(const WwiseDBString& Name) const
{
	return DataStructure.RootData.GetLanguageId(Name);
}

WwiseDBString WwiseDataStructureScopeLock::GetLanguageName(uint32 InId) const
{
	return DataStructure.RootData.GetLanguageName(InId);
}

const WwiseDBSet<WwiseDBSharedLanguageId>& WwiseDataStructureScopeLock::GetLanguages() const
{
	return DataStructure.RootData.Languages;
}

const WwiseDBSet<WwiseDBSharedPlatformId>& WwiseDataStructureScopeLock::GetPlatforms() const
{
	return DataStructure.RootData.Platforms;
}

WwiseRefPlatform WwiseDataStructureScopeLock::GetPlatform(const FWwiseSharedPlatformId& InPlatformId) const
{
	FGuid Guid = InPlatformId.GetPlatformGuid();
	if (const auto* Platform = DataStructure.RootData.PlatformGuids.Find(WwiseDBGuid(Guid.A, Guid.B, Guid.C, Guid.D)))
	{
		return *Platform;
	}
	return {};
}


WwiseDataStructureWriteScopeLock::WwiseDataStructureWriteScopeLock(FWwiseProjectDatabase& InProjectDatabase) :
	Lock(InProjectDatabase.GetLockedDataStructure()->Lock),
	DataStructure(*InProjectDatabase.GetLockedDataStructure())
{
	Lock.lock();
}

#if PLATFORM_LINUX
const WwiseDBGuid FWwiseProjectDatabase::BasePlatformGuid(0xbd0bdf13, 0x3125454f, 0x8bfd3195, 0x37169f81);
#elif PLATFORM_MAC
const WwiseDBGuid FWwiseProjectDatabase::BasePlatformGuid(0x9c6217d5, 0xdd114795, 0x87c16ce0, 0x2853c540);
#elif PLATFORM_WINDOWS
const WwiseDBGuid FWwiseProjectDatabase::BasePlatformGuid(0x6e0cb257, 0xc6c84c5c, 0x83662740, 0xdfc441eb);
#else
static_assert(false);
#endif

FWwiseSharedLanguageId FWwiseProjectDatabase::GetCurrentLanguage() const
{
	FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
	if (UNLIKELY(!ResourceLoader))
	{
		return {};
	}

	const auto CurrentLanguage = ResourceLoader->GetCurrentLanguage();
	return FWwiseSharedLanguageId(CurrentLanguage.GetLanguageId(), CurrentLanguage.GetLanguageName(), CurrentLanguage.LanguageRequirement);
}

FWwiseSharedPlatformId FWwiseProjectDatabase::GetCurrentPlatform() const
{
	FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
	if (UNLIKELY(!ResourceLoader))
	{
		return {};
	}

	return ResourceLoader->GetCurrentPlatform();
}

void FWwiseProjectDatabase::SetGeneratedSoundBanksPath(const FDirectoryPath& DirectoryPath)
{
	UE_CLOG(GeneratedSoundBanksPath.Path != DirectoryPath.Path, LogWwiseProjectDatabase, Display, TEXT("FWwiseProjectDatabase::SetGeneratedSoundBanksPath: Updating path to \"%s\""), *DirectoryPath.Path);
	GeneratedSoundBanksPath = DirectoryPath;
}

void FWwiseProjectDatabase::InitForStaging(const FWwiseProjectDatabase& DefaultProjectDatabase)
{
	SetGeneratedSoundBanksPath(DefaultProjectDatabase.GetGeneratedSoundBanksPath());
}

bool FWwiseProjectDatabase::DisableDefaultPlatforms() const
{
	return UNLIKELY(!IWwiseProjectDatabaseModule::ShouldInitializeProjectDatabase()) && (Get() == this);
}

#undef LOCTEXT_NAMESPACE
