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

#include "Wwise/Ref/WwiseRefSoundBank.h"

#include "Wwise/Ref/WwiseRefAcousticTexture.h"
#include "Wwise/Ref/WwiseRefAudioDevice.h"
#include "Wwise/Ref/WwiseRefAuxBus.h"
#include "Wwise/Ref/WwiseRefBus.h"
#include "Wwise/Ref/WwiseRefCustomPlugin.h"
#include "Wwise/Ref/WwiseRefDialogueArgument.h"
#include "Wwise/Ref/WwiseRefEvent.h"
#include "Wwise/Ref/WwiseRefExternalSource.h"
#include "Wwise/Ref/WwiseRefGameParameter.h"
#include "Wwise/Ref/WwiseRefMedia.h"
#include "Wwise/Ref/WwiseRefPluginShareSet.h"
#include "Wwise/Ref/WwiseRefState.h"
#include "Wwise/Ref/WwiseRefStateGroup.h"
#include "Wwise/Ref/WwiseRefSwitch.h"
#include "Wwise/Ref/WwiseRefSwitchGroup.h"
#include "Wwise/Ref/WwiseRefTrigger.h"

#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Metadata/WwiseMetadataSoundBanksInfo.h"


const WwiseDBString WwiseRefSoundBank::NAME = "SoundBank"_wwise_db;

const WwiseMetadataSoundBank* WwiseRefSoundBank::GetSoundBank() const
{
	const auto* SoundBanksInfo = GetSoundBanksInfo();
	if (!SoundBanksInfo) [[unlikely]]
	{
		return nullptr;
	}
	const auto& SoundBanks = SoundBanksInfo->SoundBanks;
	if (SoundBanks.IsValidIndex(SoundBankIndex))
	{
		return &SoundBanks[SoundBankIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get SoundBank index #%zu", SoundBankIndex);
		return nullptr;
	}
}

WwiseMediaIdsMap WwiseRefSoundBank::GetSoundBankMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& Media = SoundBank->Media;

	WwiseMediaIdsMap Result;
	Result.Empty(Media.Size());
	for (const auto& Elem : Media)
	{
		WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);

		const WwiseRefMedia* MediaInGlobalMap = GlobalMap.Find(Id);
		if (MediaInGlobalMap)
		{
			Result.Add(Elem.Id, *MediaInGlobalMap);
		}
	}
	return Result;
}

WwiseCustomPluginIdsMap WwiseRefSoundBank::GetSoundBankCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank || !SoundBank->Plugins) [[unlikely]]
	{
		return {};
	}
	const auto& CustomPlugins = SoundBank->Plugins->Custom;

	WwiseCustomPluginIdsMap Result;
	Result.Empty(CustomPlugins.Size());
	for (const auto& Elem : CustomPlugins)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);

		const WwiseRefCustomPlugin* InGlobalMap = GlobalMap.Find(Id);
		if (InGlobalMap)
		{
			Result.Add(Elem.Id, *InGlobalMap);
		}
	}
	return Result;
}

WwisePluginShareSetIdsMap WwiseRefSoundBank::GetSoundBankPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank || !SoundBank->Plugins) [[unlikely]]
	{
		return {};
	}
	const auto& PluginShareSets = SoundBank->Plugins->ShareSets;

	WwisePluginShareSetIdsMap Result;
	Result.Empty(PluginShareSets.Size());
	for (const auto& Elem : PluginShareSets)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);

		const WwiseRefPluginShareSet* InGlobalMap = GlobalMap.Find(Id);
		if (InGlobalMap)
		{
			Result.Add(Elem.Id, *InGlobalMap);
		}
	}
	return Result;
}

WwiseAudioDeviceIdsMap WwiseRefSoundBank::GetSoundBankAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank || !SoundBank->Plugins) [[unlikely]]
	{
		return {};
	}
	const auto& AudioDevices = SoundBank->Plugins->AudioDevices;

	WwiseAudioDeviceIdsMap Result;
	Result.Empty(AudioDevices.Size());
	for (const auto& Elem : AudioDevices)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);

		const WwiseRefAudioDevice* InGlobalMap = GlobalMap.Find(Id);
		if (InGlobalMap)
		{
			Result.Add(Elem.Id, *InGlobalMap);
		}
	}
	return Result;
}

WwiseEventIdsMap WwiseRefSoundBank::GetSoundBankEvents(const WwiseEventGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& Events = SoundBank->Events;

	WwiseEventIdsMap Result;
	Result.Empty(Events.Size());
	for (const auto& Elem : Events)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);

		const WwiseRefEvent* InGlobalMap = GlobalMap.Find(Id);
		if (InGlobalMap)
		{
			Result.Add(Elem.Id, *InGlobalMap);
		}
	}
	return Result;
}

WwiseDialogueEventIdsMap WwiseRefSoundBank::GetSoundBankDialogueEvents(const WwiseDialogueEventGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& DialogueEvents = SoundBank->DialogueEvents;
	WwiseDialogueEventIdsMap Result;
	Result.Empty(DialogueEvents.Size());
	for (const auto& Elem : DialogueEvents)
	{
		const WwiseRefDialogueEvent* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseDialogueArgumentIdsMap WwiseRefSoundBank::GetAllSoundBankDialogueArguments(const WwiseDialogueArgumentGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}

	WwiseDialogueArgumentIdsMap Result;

	for (const auto& DialogueEvent : SoundBank->DialogueEvents)
	{
		for (const auto& Elem : DialogueEvent.Arguments)
		{
			const auto* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
			if (GlobalRef)
			{
				Result.Add(Elem.Id, *GlobalRef);
			}
		}
	}

	return Result;
}

WwiseBusIdsMap WwiseRefSoundBank::GetSoundBankBusses(const WwiseBusGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& Busses = SoundBank->Busses;
	WwiseBusIdsMap Result;
	Result.Empty(Busses.Size());
	for (const auto& Elem : Busses)
	{
		const WwiseRefBus* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseAuxBusIdsMap WwiseRefSoundBank::GetSoundBankAuxBusses(const WwiseAuxBusGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& AuxBusses = SoundBank->AuxBusses;
	WwiseAuxBusIdsMap Result;
	Result.Empty(AuxBusses.Size());
	for (const auto& Elem : AuxBusses)
	{
		const WwiseRefAuxBus* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseGameParameterIdsMap WwiseRefSoundBank::GetSoundBankGameParameters(const WwiseGameParameterGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& GameParameters = SoundBank->GameParameters;
	WwiseGameParameterIdsMap Result;
	Result.Empty(GameParameters.Size());
	for (const auto& Elem : GameParameters)
	{
		const WwiseRefGameParameter* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseStateGroupIdsMap WwiseRefSoundBank::GetSoundBankStateGroups(const WwiseStateGroupGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& StateGroups = SoundBank->StateGroups;
	WwiseStateGroupIdsMap Result;
	Result.Empty(StateGroups.Size());
	for (const auto& Elem : StateGroups)
	{
		const WwiseRefStateGroup* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseStateIdsMap WwiseRefSoundBank::GetAllSoundBankStates(const WwiseStateGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	WwiseStateIdsMap Result;

	for (const auto& StateGroup : SoundBank->StateGroups)
	{
		for (const auto& State : StateGroup.States)
		{
			const auto* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(StateGroup.Id, State.Id, LanguageId));
			if (GlobalRef)
			{
				Result.Add(WwiseDatabaseGroupValueKey(StateGroup.Id, State.Id), *GlobalRef);
			}
		}
	}

	return Result;
}

WwiseSwitchGroupIdsMap WwiseRefSoundBank::GetSoundBankSwitchGroups(const WwiseSwitchGroupGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& SwitchGroups = SoundBank->SwitchGroups;
	WwiseSwitchGroupIdsMap Result;
	Result.Empty(SwitchGroups.Size());
	for (const auto& Elem : SwitchGroups)
	{
		const WwiseRefSwitchGroup* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseSwitchIdsMap WwiseRefSoundBank::GetAllSoundBankSwitches(const WwiseSwitchGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto Switches = SoundBank->GetAllSwitches();

	WwiseSwitchIdsMap Result;
	for (const auto& SwitchGroup : SoundBank->SwitchGroups)
	{
		for (const auto& Switch : SwitchGroup.Switches)
		{
			const auto* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SwitchGroup.Id, Switch.Id, LanguageId));
			if (GlobalRef)
			{
				Result.Add(WwiseDatabaseGroupValueKey(SwitchGroup.Id, Switch.Id), *GlobalRef);
			}
		}
	}

	return Result;
}

WwiseTriggerIdsMap WwiseRefSoundBank::GetSoundBankTriggers(const WwiseTriggerGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& Triggers = SoundBank->Triggers;
	WwiseTriggerIdsMap Result;
	Result.Empty(Triggers.Size());
	for (const auto& Elem : Triggers)
	{
		const WwiseRefTrigger* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseExternalSourceIdsMap WwiseRefSoundBank::GetSoundBankExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& ExternalSources = SoundBank->ExternalSources;
	WwiseExternalSourceIdsMap Result;
	Result.Empty(ExternalSources.Size());
	for (const auto& Elem : ExternalSources)
	{
		const WwiseRefExternalSource* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Cookie, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Cookie, *GlobalRef);
		}
	}

	return Result;
}

WwiseAcousticTextureIdsMap WwiseRefSoundBank::GetSoundBankAcousticTextures(const WwiseAcousticTextureGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& AcousticTextures = SoundBank->AcousticTextures;
	WwiseAcousticTextureIdsMap Result;
	Result.Empty(AcousticTextures.Size());
	for (const auto& Elem : AcousticTextures)
	{
		const WwiseRefAcousticTexture* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Elem.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}

	return Result;
}

bool WwiseRefSoundBank::IsUserBank() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return false;
	}
	return SoundBank->Type == EMetadataSoundBankType::User;
}

bool WwiseRefSoundBank::IsInitBank() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return false;
	}
	return SoundBank->IsInitBank();
}


WwiseDBShortId WwiseRefSoundBank::SoundBankId() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return 0;
	}
	return SoundBank->Id;
}

const WwiseDBGuid* WwiseRefSoundBank::SoundBankGuid() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	return &SoundBank->GUID;
}

const WwiseDBString* WwiseRefSoundBank::SoundBankLanguage() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	return &SoundBank->Language;
}

const WwiseDBString* WwiseRefSoundBank::SoundBankShortName() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	return &SoundBank->ShortName;
}

const WwiseDBString* WwiseRefSoundBank::SoundBankObjectPath() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return {};
	}
	return &SoundBank->ObjectPath;
}

WwiseDBShortId WwiseRefSoundBank::Hash() const
{
	auto Result = WwiseRefSoundBanksInfo::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(SoundBankIndex));
	return Result;
}
