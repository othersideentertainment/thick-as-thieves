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

#include "Wwise/Ref/WwiseRefEvent.h"

class WwiseRefSwitchContainer : public WwiseRefEvent
{
public:
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::SwitchContainer;

	WwiseDBArray<WwiseRefIndexType> ChildrenIndices;

	WwiseRefSwitchContainer() {}
	WwiseRefSwitchContainer(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath,
		WwiseRefIndexType InSoundBankIndex, WwiseDBShortId InLanguageId,
		WwiseRefIndexType InEventIndex,
		const WwiseDBArray<WwiseRefIndexType>& InChildrenIndices) :
		WwiseRefEvent(InRootMediaRef, InJsonFilePath, InSoundBankIndex, InLanguageId, InEventIndex),
		ChildrenIndices(InChildrenIndices)
	{}
	const WwiseMetadataSwitchContainer* GetSwitchContainer() const;
	WwiseAnyRef GetSwitchValue(const WwiseSwitchGlobalIdsMap& SwitchGlobalMap, const WwiseStateGlobalIdsMap& StateGlobalMap) const;
	WwiseMediaIdsMap GetSwitchContainerMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const;
	WwiseExternalSourceIdsMap GetSwitchContainerExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const;
	WwiseCustomPluginIdsMap GetSwitchContainerCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const;
	WwisePluginShareSetIdsMap GetSwitchContainerPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const;
	WwiseAudioDeviceIdsMap GetSwitchContainerAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const;
	WwiseDBArray<WwiseAnyRef> GetSwitchValues(const WwiseSwitchGlobalIdsMap& SwitchGlobalMap, const WwiseStateGlobalIdsMap& StateGlobalMap) const;

	WwiseDBShortId Hash() const override;
	WwiseRefType Type() const override { return TYPE; }
	bool operator==(const WwiseRefSwitchContainer& Rhs) const
	{
		return WwiseRefEvent::operator ==(Rhs)
			&& ChildrenIndices == Rhs.ChildrenIndices;
	}
	bool operator!=(const WwiseRefSwitchContainer& Rhs) const { return !operator==(Rhs); }
};
