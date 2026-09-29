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

#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Ref/WwiseRefSoundBank.h"

class WwiseRefEvent : public WwiseRefSoundBank
{
public:
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::Event;
	struct FGlobalIdsMap;

	WwiseRefIndexType EventIndex = 0;

	WwiseRefEvent() {}
	WwiseRefEvent(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath,
		WwiseRefIndexType InSoundBankIndex, WwiseDBShortId InLanguageId,
		WwiseRefIndexType InEventIndex) :
		WwiseRefSoundBank(InRootMediaRef, InJsonFilePath, InSoundBankIndex, InLanguageId),
		EventIndex(InEventIndex)
	{}
	const WwiseMetadataEvent* GetEvent() const;
	WwiseMediaIdsMap GetEventMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const;
	WwiseMediaIdsMap GetAllMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const;
	WwiseExternalSourceIdsMap GetEventExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const;
	WwiseExternalSourceIdsMap GetAllExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const;
	WwiseCustomPluginIdsMap GetEventCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const;
	WwiseCustomPluginIdsMap GetAllCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const;
	WwisePluginShareSetIdsMap GetEventPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const;
	WwisePluginShareSetIdsMap GetAllPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const;
	WwiseAudioDeviceIdsMap GetEventAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const;
	WwiseAudioDeviceIdsMap GetAllAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const;
	WwiseSwitchContainerArray GetSwitchContainers(const WwiseSwitchContainersByEvent& GlobalMap) const;
	WwiseEventIdsMap GetActionPostEvent(const WwiseEventGlobalIdsMap& GlobalMap) const;
	WwiseStateIdsMap GetActionSetState(const WwiseStateGlobalIdsMap& GlobalMap) const;
	WwiseSwitchIdsMap GetActionSetSwitch(const WwiseSwitchGlobalIdsMap& GlobalMap) const;
	WwiseTriggerIdsMap GetActionTrigger(const WwiseTriggerGlobalIdsMap& GlobalMap) const;
	WwiseAuxBusIdsMap GetEventAuxBusses(const WwiseAuxBusGlobalIdsMap& GlobalMap) const;

	WwiseDBShortId EventId() const;
	const WwiseDBGuid* EventGuid() const;
	const WwiseDBString* EventName() const;
	const WwiseDBString* EventObjectPath() const;
	float MaxAttenuation() const;
	bool GetDuration(WwiseMetadataEventDurationType& OutDurationType, float& OutDurationMin, float& OutDurationMax) const;

	WwiseDBShortId Hash() const override;
	WwiseRefType Type() const override { return TYPE; }
	bool operator==(const WwiseRefEvent& Rhs) const
	{
		return WwiseRefSoundBank::operator ==(Rhs)
			&& EventIndex == Rhs.EventIndex;
	}
	
	bool operator!=(const WwiseRefEvent& Rhs) const { return !operator==(Rhs); }
};

struct WwiseRefEvent::FGlobalIdsMap
{
	WwiseEventGlobalIdsMap GlobalIdsMap;
};
