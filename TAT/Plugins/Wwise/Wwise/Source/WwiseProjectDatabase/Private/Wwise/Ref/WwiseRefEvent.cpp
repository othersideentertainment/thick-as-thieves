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

#include "Wwise/Ref/WwiseRefEvent.h"

#include "Wwise/Ref/WwiseRefAudioDevice.h"
#include "Wwise/Ref/WwiseRefAuxBus.h"
#include "Wwise/Ref/WwiseRefCustomPlugin.h"
#include "Wwise/Ref/WwiseRefExternalSource.h"
#include "Wwise/Ref/WwiseRefMedia.h"
#include "Wwise/Ref/WwiseRefPluginShareSet.h"
#include "Wwise/Ref/WwiseRefState.h"
#include "Wwise/Ref/WwiseRefSwitch.h"
#include "Wwise/Ref/WwiseRefSwitchContainer.h"
#include "Wwise/Ref/WwiseRefTrigger.h"

#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"

#include <inttypes.h>

const WwiseDBString WwiseRefEvent::NAME = "Event"_wwise_db;

const WwiseMetadataEvent* WwiseRefEvent::GetEvent() const
{
	const auto* SoundBank = GetSoundBank();
	if (!SoundBank) [[unlikely]]
	{
		return nullptr;
	}
	const auto& Events = SoundBank->Events;
	if (Events.IsValidIndex(EventIndex))
	{
		return &Events[EventIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Event index #%zu", EventIndex);
		return nullptr;
	}
}

WwiseMediaIdsMap WwiseRefEvent::GetEventMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	const auto* Event = GetEvent();
	if (!Event || !SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& MediaRefs = Event->MediaRefs;
	WwiseMediaIdsMap Result;
	Result.Empty(MediaRefs.Size());
	for (const auto& Elem : MediaRefs)
	{
		WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);
		const WwiseRefMedia* GlobalRef = GlobalMap.Find(Id);
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}
	return Result;
}

WwiseMediaIdsMap WwiseRefEvent::GetAllMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const
{
	const auto* SoundBank = GetSoundBank();
	const auto* Event = GetEvent();
	if (!Event || !SoundBank) [[unlikely]]
	{
		return {};
	}
	WwiseMediaIdsMap Result = GetEventMedia(GlobalMap);

	const auto& SwitchContainers = Event->SwitchContainers;
	for (const auto& SwitchContainer : SwitchContainers)
	{
		for (const auto& Elem : SwitchContainer.GetAllMedia())
		{
			WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);
			const WwiseRefMedia* GlobalRef = GlobalMap.Find(Id);
			if (GlobalRef)
			{
				Result.Add(Elem.Id, *GlobalRef);
			}
		}
	}

	if (Event->PluginRefs && SoundBank->Plugins)
	{
		const auto& CustomPlugins = Event->PluginRefs->Custom;
		for (const auto& CustomPlugin : CustomPlugins)
		{
			const auto PluginId = CustomPlugin.Id;
			const auto* Plugin = SoundBank->Plugins->Custom.FindByPredicate([PluginId](const WwiseMetadataPlugin& RhsValue)
			{
				return RhsValue.Id == PluginId;
			});
			if (Plugin) [[likely]]
			{
				for (const auto& Elem : Plugin->MediaRefs)
				{
					WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);
					const WwiseRefMedia* GlobalRef = GlobalMap.Find(Id);
					if (GlobalRef)
					{
						Result.Add(Elem.Id, *GlobalRef);
					}
				}
			}
			else
			{
				WWISE_DB_LOG(Error, "Cannot find Plugin %" PRIu32, PluginId);
			}
		}

		const auto& PluginShareSets = Event->PluginRefs->ShareSets;
		for (const auto& PluginShareSet : PluginShareSets)
		{
			const auto PluginId = PluginShareSet.Id;
			const auto* Plugin = SoundBank->Plugins->ShareSets.FindByPredicate([PluginId](const WwiseMetadataPlugin& RhsValue)
			{
				return RhsValue.Id == PluginId;
			});
			if (Plugin) [[likely]]
			{
				for (const auto& Elem : Plugin->MediaRefs)
				{
					WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);
					const WwiseRefMedia* GlobalRef = GlobalMap.Find(Id);
					if (GlobalRef)
					{
						Result.Add(Elem.Id, *GlobalRef);
					}
				}
			}
			else
			{
				WWISE_DB_LOG(Error, "Cannot find Plugin %" PRIu32, PluginId);
			}
		}

		const auto& AudioDevices = Event->PluginRefs->AudioDevices;
		for (const auto& AudioDevice : AudioDevices)
		{
			const auto PluginId = AudioDevice.Id;
			const auto* Plugin = SoundBank->Plugins->AudioDevices.FindByPredicate([PluginId](const WwiseMetadataPlugin& RhsValue)
			{
				return RhsValue.Id == PluginId;
			});
			if (Plugin) [[likely]]
			{
				for (const auto& Elem : Plugin->MediaRefs)
				{
					WwiseDatabaseMediaIdKey Id(Elem.Id, SoundBank->Id);
					const WwiseRefMedia* GlobalRef = GlobalMap.Find(Id);
					if (GlobalRef)
					{
						Result.Add(Elem.Id, *GlobalRef);
					}
				}
			}
			else
			{
				WWISE_DB_LOG(Error, "Cannot find Plugin %u", PluginId);
			}
		}
	}
	return Result;
}

WwiseExternalSourceIdsMap WwiseRefEvent::GetEventExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	const auto& ExternalSourceRefs = Event->ExternalSourceRefs;
	WwiseExternalSourceIdsMap Result;
	Result.Empty(ExternalSourceRefs.Size());
	for (const auto& Elem : ExternalSourceRefs)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Cookie, LanguageId);
		const WwiseRefExternalSource* GlobalRef = GlobalMap.Find(Id);
		if (GlobalRef)
		{
			Result.Add(Elem.Cookie, *GlobalRef);
		}
	}
	return Result;
}

WwiseExternalSourceIdsMap WwiseRefEvent::GetAllExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	WwiseExternalSourceIdsMap Result = GetEventExternalSources(GlobalMap);

	const auto& SwitchContainers = Event->SwitchContainers;
	for (const auto& SwitchContainer : SwitchContainers)
	{
		for (const auto& Elem : SwitchContainer.GetAllExternalSources())
		{
			WwiseDatabaseLocalizableIdKey Id(Elem.Cookie, LanguageId);
			const WwiseRefExternalSource* GlobalRef = GlobalMap.Find(Id);
			if (GlobalRef)
			{
				Result.Add(Elem.Cookie, *GlobalRef);
			}
		}
	}
	return Result;
}

WwiseCustomPluginIdsMap WwiseRefEvent::GetEventCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event || !Event->PluginRefs)
	{
		return {};
	}
	const auto& Plugins = Event->PluginRefs->Custom;
	WwiseCustomPluginIdsMap Result;
	Result.Empty(Plugins.Size());
	for (const auto& Elem : Plugins)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);
		const WwiseRefCustomPlugin* GlobalRef = GlobalMap.Find(Id);
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}
	return Result;
}

WwiseCustomPluginIdsMap WwiseRefEvent::GetAllCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event || !Event->PluginRefs)
	{
		return {};
	}
	WwiseCustomPluginIdsMap Result = GetEventCustomPlugins(GlobalMap);

	const auto& SwitchContainers = Event->SwitchContainers;
	for (const auto& SwitchContainer : SwitchContainers)
	{
		for (const auto& Elem : SwitchContainer.GetAllCustomPlugins())
		{
			WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);
			const WwiseRefCustomPlugin* GlobalRef = GlobalMap.Find(Id);
			if (GlobalRef)
			{
				Result.Add(Elem.Id, *GlobalRef);
			}
		}
	}
	return Result;
}

WwisePluginShareSetIdsMap WwiseRefEvent::GetEventPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event || !Event->PluginRefs)
	{
		return {};
	}
	const auto& Plugins = Event->PluginRefs->ShareSets;
	WwisePluginShareSetIdsMap Result;
	Result.Empty(Plugins.Size());
	for (const auto& Elem : Plugins)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);
		const WwiseRefPluginShareSet* GlobalRef = GlobalMap.Find(Id);
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}
	return Result;
}

WwisePluginShareSetIdsMap WwiseRefEvent::GetAllPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event || !Event->PluginRefs)
	{
		return {};
	}
	WwisePluginShareSetIdsMap Result = GetEventPluginShareSets(GlobalMap);

	const auto& SwitchContainers = Event->SwitchContainers;
	for (const auto& SwitchContainer : SwitchContainers)
	{
		for (const auto& Elem : SwitchContainer.GetAllPluginShareSets())
		{
			WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);
			const WwiseRefPluginShareSet* GlobalRef = GlobalMap.Find(Id);
			if (GlobalRef)
			{
				Result.Add(Elem.Id, *GlobalRef);
			}
		}
	}
	return Result;
}

WwiseAudioDeviceIdsMap WwiseRefEvent::GetEventAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event || !Event->PluginRefs)
	{
		return {};
	}
	const auto& Plugins = Event->PluginRefs->AudioDevices;
	WwiseAudioDeviceIdsMap Result;
	Result.Empty(Plugins.Size());
	for (const auto& Elem : Plugins)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);
		const WwiseRefAudioDevice* GlobalRef = GlobalMap.Find(Id);
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}
	return Result;
}

WwiseAudioDeviceIdsMap WwiseRefEvent::GetAllAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event || !Event->PluginRefs)
	{
		return {};
	}
	WwiseAudioDeviceIdsMap Result = GetEventAudioDevices(GlobalMap);

	const auto& SwitchContainers = Event->SwitchContainers;
	for (const auto& SwitchContainer : SwitchContainers)
	{
		for (const auto& Elem : SwitchContainer.GetAllPluginShareSets())
		{
			WwiseDatabaseLocalizableIdKey Id(Elem.Id, LanguageId);
			const WwiseRefAudioDevice* GlobalRef = GlobalMap.Find(Id);
			if (GlobalRef)
			{
				Result.Add(Elem.Id, *GlobalRef);
			}
		}
	}
	return Result;
}

WwiseSwitchContainerArray WwiseRefEvent::GetSwitchContainers(const WwiseSwitchContainersByEvent& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	WwiseDatabaseLocalizableIdKey LocId(Event->Id, LanguageId);

	WwiseSwitchContainerArray Result;
	GlobalMap.MultiFind(LocId, Result);
	return Result;
}

WwiseEventIdsMap WwiseRefEvent::GetActionPostEvent(const WwiseEventGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	const auto& PostEvents = Event->ActionPostEvent;
	WwiseEventIdsMap Result;
	Result.Empty(PostEvents.Size());
	for (const auto& PostEvent : PostEvents)
	{
		const WwiseRefEvent* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(PostEvent.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(PostEvent.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseStateIdsMap WwiseRefEvent::GetActionSetState(const WwiseStateGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	const auto& SetStates = Event->ActionSetState;
	WwiseStateIdsMap Result;
	Result.Empty(SetStates.Size());
	for (const auto& SetState : SetStates)
	{
		const WwiseRefState* StateRef = GlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SetState.GroupId, SetState.Id, LanguageId));
		if (StateRef)
		{
			const auto* State = StateRef->GetState();
			const auto* StateGroup = StateRef->GetStateGroup();
			if (State && StateGroup)
			{
				Result.Add(WwiseDatabaseGroupValueKey(StateGroup->Id, State->Id), *StateRef);
			}
		}
	}

	return Result;
}

WwiseSwitchIdsMap WwiseRefEvent::GetActionSetSwitch(const WwiseSwitchGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	const auto& SetSwitches = Event->ActionSetSwitch;
	WwiseSwitchIdsMap Result;
	Result.Empty(SetSwitches.Size());
	for (const auto& SetSwitch : SetSwitches)
	{
		const WwiseRefSwitch* SwitchRef = GlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SetSwitch.GroupId, SetSwitch.Id, LanguageId));
		if (SwitchRef)
		{
			const auto* Switch = SwitchRef->GetSwitch();
			const auto* SwitchGroup = SwitchRef->GetSwitchGroup();
			if (Switch && SwitchGroup)
			{
				Result.Add(WwiseDatabaseGroupValueKey(SwitchGroup->Id, Switch->Id), *SwitchRef);
			}
		}
	}

	return Result;
}

WwiseTriggerIdsMap WwiseRefEvent::GetActionTrigger(const WwiseTriggerGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	const auto& Triggers = Event->ActionTrigger;
	WwiseTriggerIdsMap Result;
	Result.Empty(Triggers.Size());
	for (const auto& Trigger : Triggers)
	{
		const WwiseRefTrigger* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(Trigger.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(Trigger.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseAuxBusIdsMap WwiseRefEvent::GetEventAuxBusses(const WwiseAuxBusGlobalIdsMap& GlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	const auto& AuxBusRefs = Event->AuxBusRefs;
	WwiseAuxBusIdsMap Result;
	Result.Empty(AuxBusRefs.Size());
	for (const auto& AuxBusRef : AuxBusRefs)
	{
		const WwiseRefAuxBus* GlobalRef = GlobalMap.Find(WwiseDatabaseLocalizableIdKey(AuxBusRef.Id, LanguageId));
		if (GlobalRef)
		{
			Result.Add(AuxBusRef.Id, *GlobalRef);
		}
	}

	return Result;
}

WwiseDBShortId WwiseRefEvent::EventId() const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	return Event->Id;
}

const WwiseDBGuid* WwiseRefEvent::EventGuid() const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}
	return &Event->GUID;
}

const WwiseDBString* WwiseRefEvent::EventName() const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return &emptyString;
	}
	return &Event->Name;
}

const WwiseDBString* WwiseRefEvent::EventObjectPath() const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return &emptyString;
	}
	return &Event->ObjectPath;
}

float WwiseRefEvent::MaxAttenuation() const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return 0.f;
	}
	return Event->MaxAttenuation;
}

bool WwiseRefEvent::GetDuration(WwiseMetadataEventDurationType& OutDurationType, float& OutDurationMin, float& OutDurationMax) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return false;
	}

	OutDurationType = Event->DurationType;
	OutDurationMin = Event->DurationMin;
	OutDurationMax = Event->DurationMax;
	return true;
}

WwiseDBShortId WwiseRefEvent::Hash() const
{
	auto Result = WwiseRefSoundBank::Hash();
	Result = WwiseDBHashCombine(Result, GetTypeHash(EventIndex));
	return Result;
}
