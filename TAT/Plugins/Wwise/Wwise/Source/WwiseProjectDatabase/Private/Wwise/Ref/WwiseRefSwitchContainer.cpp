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

#include "Wwise/Ref/WwiseRefSwitchContainer.h"

#include "Wwise/Ref/WwiseAnyRef.h"
#include "Wwise/Ref/WwiseRefAudioDevice.h"
#include "Wwise/Ref/WwiseRefCustomPlugin.h"
#include "Wwise/Ref/WwiseRefExternalSource.h"
#include "Wwise/Ref/WwiseRefMedia.h"
#include "Wwise/Ref/WwiseRefPluginShareSet.h"
#include "Wwise/Ref/WwiseRefState.h"
#include "Wwise/Ref/WwiseRefSwitch.h"
#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Metadata/WwiseMetadataMedia.h"
#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Metadata/WwiseMetadataSwitchContainer.h"
#include "Wwise/Metadata/WwiseMetadataSwitchValue.h"

const WwiseDBString WwiseRefSwitchContainer::NAME = "SwitchContainer"_wwise_db;

const WwiseMetadataSwitchContainer* WwiseRefSwitchContainer::GetSwitchContainer() const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return nullptr;
	}

	const auto* SwitchContainers = &Event->SwitchContainers;
	const WwiseMetadataSwitchContainer* Result = nullptr;
	for (auto Index : ChildrenIndices)
	{
		if (!SwitchContainers->IsValidIndex(Index))
		{
			WWISE_DB_LOG(Error, "Could not get Switch Container index #%zu", Index);
			return nullptr;
		}
		Result = &(*SwitchContainers)[Index];
		SwitchContainers = &Result->Children;
	}
	return Result;
}

WwiseAnyRef WwiseRefSwitchContainer::GetSwitchValue(const WwiseSwitchGlobalIdsMap& SwitchGlobalMap, const WwiseStateGlobalIdsMap& StateGlobalMap) const
{
	const auto* Container = GetSwitchContainer();
	if (!Container) [[unlikely]]
	{
		return {};
	}
	const auto& SwitchValue = Container->SwitchValue;
	switch (SwitchValue.GroupType)
	{
	case WwiseMetadataSwitchValueGroupType::Switch:
	{
		const WwiseRefSwitch* GlobalRef = SwitchGlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SwitchValue.GroupId, SwitchValue.Id, LanguageId));
		if (!GlobalRef)
		{
			return {};
		}
		return WwiseAnyRef::Create(*GlobalRef);
	}
	case WwiseMetadataSwitchValueGroupType::State:
	{
		const WwiseRefState* GlobalRef = StateGlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SwitchValue.GroupId, SwitchValue.Id, LanguageId));
		if (!GlobalRef)
		{
			return {};
		}
		return WwiseAnyRef::Create(*GlobalRef);
	}
	}
	return {};
}

WwiseMediaIdsMap WwiseRefSwitchContainer::GetSwitchContainerMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const
{
	const auto* SwitchContainer = GetSwitchContainer();
	const auto* SoundBank = GetSoundBank();
	if (!SwitchContainer || !SoundBank) [[unlikely]]
	{
		return {};
	}
	const auto& MediaRefs = SwitchContainer->MediaRefs;
	WwiseMediaIdsMap Result;
	Result.Empty(MediaRefs.Size());
	for (const auto& Elem : MediaRefs)
	{
		WwiseDatabaseMediaIdKey SoundBankFileId(Elem.Id, SoundBank->Id);
		const auto* GlobalRef = GlobalMap.Find(SoundBankFileId);
		if (GlobalRef)
		{
			Result.Add(Elem.Id, *GlobalRef);
		}
	}
	return Result;
}

WwiseExternalSourceIdsMap WwiseRefSwitchContainer::GetSwitchContainerExternalSources(const WwiseExternalSourceGlobalIdsMap& GlobalMap) const
{
	const auto* SwitchContainer = GetSwitchContainer();
	if (!SwitchContainer) [[unlikely]]
	{
		return {};
	}
	const auto& ExternalSourceRefs = SwitchContainer->ExternalSourceRefs;
	WwiseExternalSourceIdsMap Result;
	Result.Empty(ExternalSourceRefs.Size());
	for (const auto& Elem : ExternalSourceRefs)
	{
		WwiseDatabaseLocalizableIdKey Id(Elem.Cookie, LanguageId);
		const auto* GlobalRef = GlobalMap.Find(Id);
		if (GlobalRef)
		{
			Result.Add(Elem.Cookie, *GlobalRef);
		}
	}
	return Result;
}

WwiseCustomPluginIdsMap WwiseRefSwitchContainer::GetSwitchContainerCustomPlugins(const WwiseCustomPluginGlobalIdsMap& GlobalMap) const
{
	const auto* SwitchContainer = GetSwitchContainer();
	if (!SwitchContainer || !SwitchContainer->PluginRefs) [[unlikely]]
	{
		return {};
	}
	const auto& Plugins = SwitchContainer->PluginRefs->Custom;
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

WwisePluginShareSetIdsMap WwiseRefSwitchContainer::GetSwitchContainerPluginShareSets(const WwisePluginShareSetGlobalIdsMap& GlobalMap) const
{
	const auto* SwitchContainer = GetSwitchContainer();
	if (!SwitchContainer || !SwitchContainer->PluginRefs) [[unlikely]]
	{
		return {};
	}
	const auto& Plugins = SwitchContainer->PluginRefs->ShareSets;
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

WwiseAudioDeviceIdsMap WwiseRefSwitchContainer::GetSwitchContainerAudioDevices(const WwiseAudioDeviceGlobalIdsMap& GlobalMap) const
{
	const auto* SwitchContainer = GetSwitchContainer();
	if (!SwitchContainer || !SwitchContainer->PluginRefs) [[unlikely]]
	{
		return {};
	}
	const auto& Plugins = SwitchContainer->PluginRefs->AudioDevices;
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

WwiseDBArray<WwiseAnyRef> WwiseRefSwitchContainer::GetSwitchValues(const WwiseSwitchGlobalIdsMap& SwitchGlobalMap, const WwiseStateGlobalIdsMap& StateGlobalMap) const
{
	const auto* Event = GetEvent();
	if (!Event) [[unlikely]]
	{
		return {};
	}

	const auto* SwitchContainers = &Event->SwitchContainers;
	WwiseDBArray<WwiseAnyRef> Result;
	for (auto Index : ChildrenIndices)
	{
		if (!SwitchContainers->IsValidIndex(Index))
		{
			return {};
		}
		const auto& SwitchContainer = (*SwitchContainers)[Index];
		const auto& SwitchValue = SwitchContainer.SwitchValue;

		// Skipping Default Switches, but keep different ones
		if (!SwitchValue.bDefault)
		{
			switch (SwitchValue.GroupType)
			{
			case WwiseMetadataSwitchValueGroupType::Switch:
			{
				const WwiseRefSwitch* GlobalRef = SwitchGlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SwitchValue.GroupId, SwitchValue.Id, LanguageId));
				if (!GlobalRef)
				{
					return {};
				}
				Result.Add(WwiseAnyRef::Create(*GlobalRef));
				break;
			}
			case WwiseMetadataSwitchValueGroupType::State:
			{
				const WwiseRefState* GlobalRef = StateGlobalMap.Find(WwiseDatabaseLocalizableGroupValueKey(SwitchValue.GroupId, SwitchValue.Id, LanguageId));
				if (!GlobalRef)
				{
					return {};
				}
				Result.Add(WwiseAnyRef::Create(*GlobalRef));
				break;
			}
			default:
				return {};
			}
		}

		SwitchContainers = &SwitchContainer.Children;
	}
	return Result;
}

WwiseDBShortId WwiseRefSwitchContainer::Hash() const
{
	auto Result = WwiseRefEvent::Hash();
	if (ChildrenIndices.Size() > 0)
	{
		Result = WwiseDBHashCombine(Result, WwiseDBShortId(WwiseDBHashCombine(GetTypeHash(*ChildrenIndices.GetData()), (unsigned int)(ChildrenIndices.GetTypeSize() * ChildrenIndices.Size()))));
	}
	return Result;
}
