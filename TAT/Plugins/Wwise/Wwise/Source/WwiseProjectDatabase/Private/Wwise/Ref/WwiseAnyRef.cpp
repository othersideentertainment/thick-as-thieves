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

#include "Wwise/Ref/WwiseAnyRef.h"

#include "Wwise/Metadata/WwiseMetadataLanguage.h"
#include "Wwise/Metadata/WwiseMetadataPlatform.h"
#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Metadata/WwiseMetadataSoundBank.h"
#include "Wwise/Ref/WwiseRefAcousticTexture.h"
#include "Wwise/Ref/WwiseRefAudioDevice.h"
#include "Wwise/Ref/WwiseRefAuxBus.h"
#include "Wwise/Ref/WwiseRefBus.h"
#include "Wwise/Ref/WwiseRefCustomPlugin.h"
#include "Wwise/Ref/WwiseRefDialogueArgument.h"
#include "Wwise/Ref/WwiseRefExternalSource.h"
#include "Wwise/Ref/WwiseRefGameParameter.h"
#include "Wwise/Ref/WwiseRefLanguage.h"
#include "Wwise/Ref/WwiseRefMedia.h"
#include "Wwise/Ref/WwiseRefPlatform.h"
#include "Wwise/Ref/WwiseRefPluginLib.h"
#include "Wwise/Ref/WwiseRefPluginShareSet.h"
#include "Wwise/Ref/WwiseRefSoundBank.h"
#include "Wwise/Ref/WwiseRefState.h"
#include "Wwise/Ref/WwiseRefSwitch.h"
#include "Wwise/Ref/WwiseRefSwitchContainer.h"
#include "Wwise/Ref/WwiseRefTrigger.h"

const WwiseRefLanguage* WwiseAnyRef::GetLanguageRef() const
{
	if (GetType() != WwiseRefType::Language) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefLanguage*>(Ref.Get());
}

const WwiseRefPlatform* WwiseAnyRef::GetPlatformRef() const
{
	if (GetType() != WwiseRefType::Platform) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefPlatform*>(Ref.Get());
}

const WwiseRefPluginLib* WwiseAnyRef::GetPluginLibRef() const
{
	if (GetType() != WwiseRefType::PluginLib) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefPluginLib*>(Ref.Get());
}

const WwiseRefSoundBank* WwiseAnyRef::GetSoundBankRef() const
{
	if (GetType() < WwiseRefType::SoundBank || GetType() > WwiseRefType::AcousticTexture) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefSoundBank*>(Ref.Get());
}

const WwiseRefMedia* WwiseAnyRef::GetMediaRef() const
{
	if (GetType() != WwiseRefType::Media) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefMedia*>(Ref.Get());
}

const WwiseRefCustomPlugin* WwiseAnyRef::GetCustomPluginRef() const
{
	if (GetType() != WwiseRefType::CustomPlugin) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefCustomPlugin*>(Ref.Get());
}

const WwiseRefPluginShareSet* WwiseAnyRef::GetPluginShareSetRef() const
{
	if (GetType() != WwiseRefType::PluginShareSet) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefPluginShareSet*>(Ref.Get());
}

const WwiseRefAudioDevice* WwiseAnyRef::GetAudioDeviceRef() const
{
	if (GetType() != WwiseRefType::AudioDevice) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefAudioDevice*>(Ref.Get());
}

const WwiseRefEvent* WwiseAnyRef::GetEventRef() const
{
	if (GetType() < WwiseRefType::Event || GetType() > WwiseRefType::SwitchContainer) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefEvent*>(Ref.Get());
}

const WwiseRefSwitchContainer* WwiseAnyRef::GetSwitchContainerRef() const
{
	if (GetType() != WwiseRefType::SwitchContainer) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefSwitchContainer*>(Ref.Get());
}

const WwiseRefDialogueEvent* WwiseAnyRef::GetDialogueEventRef() const
{
	if (GetType() < WwiseRefType::DialogueEvent || GetType() > WwiseRefType::DialogueArgument) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefDialogueEvent*>(Ref.Get());
}

const WwiseRefDialogueArgument* WwiseAnyRef::GetDialogueArgumentRef() const
{
	if (GetType() != WwiseRefType::DialogueArgument) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefDialogueArgument*>(Ref.Get());
}

const WwiseRefBus* WwiseAnyRef::GetBusRef() const
{
	if (GetType() != WwiseRefType::Bus) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefBus*>(Ref.Get());
}

const WwiseRefAuxBus* WwiseAnyRef::GetAuxBusRef() const
{
	if (GetType() != WwiseRefType::AuxBus) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefAuxBus*>(Ref.Get());
}

const WwiseRefGameParameter* WwiseAnyRef::GetGameParameterRef() const
{
	if (GetType() != WwiseRefType::GameParameter) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefGameParameter*>(Ref.Get());
}

const WwiseRefStateGroup* WwiseAnyRef::GetStateGroupRef() const
{
	if (GetType() < WwiseRefType::StateGroup || GetType() > WwiseRefType::State) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefStateGroup*>(Ref.Get());
}

const WwiseRefState* WwiseAnyRef::GetStateRef() const
{
	if (GetType() != WwiseRefType::State) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefState*>(Ref.Get());
}

const WwiseRefSwitchGroup* WwiseAnyRef::GetSwitchGroupRef() const
{
	if (GetType() < WwiseRefType::SwitchGroup || GetType() > WwiseRefType::Switch) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefSwitchGroup*>(Ref.Get());
}

const WwiseRefSwitch* WwiseAnyRef::GetSwitchRef() const
{
	if (GetType() != WwiseRefType::Switch) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefSwitch*>(Ref.Get());
}

const WwiseRefTrigger* WwiseAnyRef::GetTriggerRef() const
{
	if (GetType() != WwiseRefType::Trigger) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefTrigger*>(Ref.Get());
}

const WwiseRefExternalSource* WwiseAnyRef::GetExternalSourceRef() const
{
	if (GetType() != WwiseRefType::ExternalSource) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefExternalSource*>(Ref.Get());
}

const WwiseRefAcousticTexture* WwiseAnyRef::GetAcousticTextureRef() const
{
	if (GetType() != WwiseRefType::AcousticTexture) [[unlikely]]
	{
		return nullptr;
	}
	return static_cast<const WwiseRefAcousticTexture*>(Ref.Get());
}

const WwiseMetadataLanguage* WwiseAnyRef::GetLanguage() const
{
	const auto* LanguageRef = GetLanguageRef();
	if (!LanguageRef) [[unlikely]]
	{
		return nullptr;
	}
	return LanguageRef->GetLanguage();
}

const WwiseMetadataPlatform* WwiseAnyRef::GetPlatform() const
{
	const auto* PlatformRef = GetPlatformRef();
	if (!PlatformRef) [[unlikely]]
	{
		return nullptr;
	}
	return PlatformRef->GetPlatform();
}

const WwiseMetadataPlatformReference* WwiseAnyRef::GetPlatformReference() const
{
	const auto* PlatformRef = GetPlatformRef();
	if (!PlatformRef) [[unlikely]]
	{
		return nullptr;
	}
	return PlatformRef->GetPlatformReference();
}

const WwiseMetadataPluginLib* WwiseAnyRef::GetPluginLib() const
{
	const auto* PluginLibRef = GetPluginLibRef();
	if (!PluginLibRef) [[unlikely]]
	{
		return nullptr;
	}
	return PluginLibRef->GetPluginLib();
}

const WwiseMetadataSoundBank* WwiseAnyRef::GetSoundBank() const
{
	const auto* SoundBankRef = GetSoundBankRef();
	if (!SoundBankRef) [[unlikely]]
	{
		return nullptr;
	}
	return SoundBankRef->GetSoundBank();
}

const WwiseMetadataMedia* WwiseAnyRef::GetMedia() const
{
	const auto* MediaRef = GetMediaRef();
	if (!MediaRef) [[unlikely]]
	{
		return nullptr;
	}
	return MediaRef->GetMedia();
}

const WwiseMetadataPlugin* WwiseAnyRef::GetCustomPlugin() const
{
	const auto* CustomPluginRef = GetCustomPluginRef();
	if (!CustomPluginRef) [[unlikely]]
	{
		return nullptr;
	}
	return CustomPluginRef->GetPlugin();
}

const WwiseMetadataPlugin* WwiseAnyRef::GetPluginShareSet() const
{
	const auto* PluginShareSetRef = GetPluginShareSetRef();
	if (!PluginShareSetRef) [[unlikely]]
	{
		return nullptr;
	}
	return PluginShareSetRef->GetPlugin();
}

const WwiseMetadataPlugin* WwiseAnyRef::GetAudioDevice() const
{
	const auto* AudioDeviceRef = GetAudioDeviceRef();
	if (!AudioDeviceRef) [[unlikely]]
	{
		return nullptr;
	}
	return AudioDeviceRef->GetPlugin();
}

const WwiseMetadataEvent* WwiseAnyRef::GetEvent() const
{
	const auto* EventRef = GetEventRef();
	if (!EventRef) [[unlikely]]
	{
		return nullptr;
	}
	return EventRef->GetEvent();
}

const WwiseMetadataSwitchContainer* WwiseAnyRef::GetSwitchContainer() const
{
	const auto* SwitchContainerRef = GetSwitchContainerRef();
	if (!SwitchContainerRef) [[unlikely]]
	{
		return nullptr;
	}
	return SwitchContainerRef->GetSwitchContainer();
}

const WwiseMetadataDialogueEvent* WwiseAnyRef::GetDialogueEvent() const
{
	const auto* DialogueEventRef = GetDialogueEventRef();
	if (!DialogueEventRef) [[unlikely]]
	{
		return nullptr;
	}
	return DialogueEventRef->GetDialogueEvent();
}

const WwiseMetadataDialogueArgument* WwiseAnyRef::GetDialogueArgument() const
{
	const auto* DialogueArgumentRef = GetDialogueArgumentRef();
	if (!DialogueArgumentRef) [[unlikely]]
	{
		return nullptr;
	}
	return DialogueArgumentRef->GetDialogueArgument();
}

const WwiseMetadataBus* WwiseAnyRef::GetBus() const
{
	const auto* BusRef = GetBusRef();
	if (!BusRef) [[unlikely]]
	{
		return nullptr;
	}
	return BusRef->GetBus();
}

const WwiseMetadataBus* WwiseAnyRef::GetAuxBus() const
{
	const auto* AuxBusRef = GetAuxBusRef();
	if (!AuxBusRef) [[unlikely]]
	{
		return nullptr;
	}
	return AuxBusRef->GetAuxBus();
}

const WwiseMetadataGameParameter* WwiseAnyRef::GetGameParameter() const
{
	const auto* GameParameterRef = GetGameParameterRef();
	if (!GameParameterRef) [[unlikely]]
	{
		return nullptr;
	}
	return GameParameterRef->GetGameParameter();
}

const WwiseMetadataStateGroup* WwiseAnyRef::GetStateGroup() const
{
	const auto* StateGroupRef = GetStateGroupRef();
	if (!StateGroupRef) [[unlikely]]
	{
		return nullptr;
	}
	return StateGroupRef->GetStateGroup();
}

const WwiseMetadataState* WwiseAnyRef::GetState() const
{
	const auto* StateRef = GetStateRef();
	if (!StateRef) [[unlikely]]
	{
		return nullptr;
	}
	return StateRef->GetState();
}

const WwiseMetadataSwitchGroup* WwiseAnyRef::GetSwitchGroup() const
{
	const auto* SwitchGroupRef = GetSwitchGroupRef();
	if (!SwitchGroupRef) [[unlikely]]
	{
		return nullptr;
	}
	return SwitchGroupRef->GetSwitchGroup();
}

const WwiseMetadataSwitch* WwiseAnyRef::GetSwitch() const
{
	const auto* SwitchRef = GetSwitchRef();
	if (!SwitchRef) [[unlikely]]
	{
		return nullptr;
	}
	return SwitchRef->GetSwitch();
}

const WwiseMetadataTrigger* WwiseAnyRef::GetTrigger() const
{
	const auto* TriggerRef = GetTriggerRef();
	if (!TriggerRef) [[unlikely]]
	{
		return nullptr;
	}
	return TriggerRef->GetTrigger();
}

const WwiseMetadataExternalSource* WwiseAnyRef::GetExternalSource() const
{
	const auto* ExternalSourceRef = GetExternalSourceRef();
	if (!ExternalSourceRef) [[unlikely]]
	{
		return nullptr;
	}
	return ExternalSourceRef->GetExternalSource();
}

const WwiseMetadataAcousticTexture* WwiseAnyRef::GetAcousticTexture() const
{
	const auto* AcousticTextureRef = GetAcousticTextureRef();
	if (!AcousticTextureRef) [[unlikely]]
	{
		return nullptr;
	}
	return AcousticTextureRef->GetAcousticTexture();
}

bool WwiseAnyRef::GetRef(WwiseRefLanguage& OutRef) const
{
	const auto* Result = GetLanguageRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefPlatform& OutRef) const
{
	const auto* Result = GetPlatformRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefPluginLib& OutRef) const
{
	const auto* Result = GetPluginLibRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefSoundBank& OutRef) const
{
	const auto* Result = GetSoundBankRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefMedia& OutRef) const
{
	const auto* Result = GetMediaRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefCustomPlugin& OutRef) const
{
	const auto* Result = GetCustomPluginRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefPluginShareSet& OutRef) const
{
	const auto* Result = GetPluginShareSetRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefAudioDevice& OutRef) const
{
	const auto* Result = GetAudioDeviceRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefEvent& OutRef) const
{
	const auto* Result = GetEventRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefSwitchContainer& OutRef) const
{
	const auto* Result = GetSwitchContainerRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefDialogueEvent& OutRef) const
{
	const auto* Result = GetDialogueEventRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefDialogueArgument& OutRef) const
{
	const auto* Result = GetDialogueArgumentRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefBus& OutRef) const
{
	const auto* Result = GetBusRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefAuxBus& OutRef) const
{
	const auto* Result = GetAuxBusRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefGameParameter& OutRef) const
{
	const auto* Result = GetGameParameterRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefStateGroup& OutRef) const
{
	const auto* Result = GetStateGroupRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefState& OutRef) const
{
	const auto* Result = GetStateRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefSwitchGroup& OutRef) const
{
	const auto* Result = GetSwitchGroupRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefSwitch& OutRef) const
{
	const auto* Result = GetSwitchRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefTrigger& OutRef) const
{
	const auto* Result = GetTriggerRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefExternalSource& OutRef) const
{
	const auto* Result = GetExternalSourceRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

bool WwiseAnyRef::GetRef(WwiseRefAcousticTexture& OutRef) const
{
	const auto* Result = GetAcousticTextureRef();
	if (!Result) [[unlikely]]
	{
		return false;
	}
	OutRef = *Result;
	return true;
}

WwiseDBGuid WwiseAnyRef::GetGuid(const WwiseRefType* TypeOverride) const
{
	const auto Type = TypeOverride ? *TypeOverride : GetType();
	switch (Type)
	{
	case WwiseRefType::RootFile: return {};
	case WwiseRefType::ProjectInfo: return {};
	case WwiseRefType::Language: return GetLanguage()->GUID;
	case WwiseRefType::PlatformInfo: return {};
	case WwiseRefType::Platform: return GetPlatformReference()->GUID;
	case WwiseRefType::PluginInfo: return {};
	case WwiseRefType::PluginLib: return {};
	case WwiseRefType::SoundBanksInfo: return {};
	case WwiseRefType::SoundBank: return GetSoundBank()->GUID;
	case WwiseRefType::Media: return {};
	case WwiseRefType::CustomPlugin: return GetCustomPlugin()->GUID;
	case WwiseRefType::PluginShareSet: return GetPluginShareSet()->GUID;
	case WwiseRefType::AudioDevice: return GetAudioDevice()->GUID;
	case WwiseRefType::Event: return GetEvent()->GUID;
	case WwiseRefType::SwitchContainer: return {};
	case WwiseRefType::DialogueEvent: return GetDialogueEvent()->GUID;
	case WwiseRefType::DialogueArgument: return GetDialogueArgument()->GUID;
	case WwiseRefType::Bus: return GetBus()->GUID;
	case WwiseRefType::AuxBus: return GetAuxBus()->GUID;
	case WwiseRefType::GameParameter: return GetGameParameter()->GUID;
	case WwiseRefType::StateGroup: return GetStateGroup()->GUID;
	case WwiseRefType::State: return GetState()->GUID;
	case WwiseRefType::SwitchGroup: return GetSwitchGroup()->GUID;
	case WwiseRefType::Switch: return GetSwitch()->GUID;
	case WwiseRefType::Trigger: return GetTrigger()->GUID;
	case WwiseRefType::ExternalSource: return GetExternalSource()->GUID;
	case WwiseRefType::AcousticTexture: return GetAcousticTexture()->GUID;
	case WwiseRefType::None:
	default:
		return {};
	}
}

WwiseDBShortId WwiseAnyRef::GetGroupId(const WwiseRefType* TypeOverride) const
{
	const auto Type = TypeOverride ? *TypeOverride : GetType();
	switch (Type)
	{
	case WwiseRefType::RootFile: return 0;
	case WwiseRefType::ProjectInfo: return 0;
	case WwiseRefType::Language: return 0;
	case WwiseRefType::PlatformInfo: return 0;
	case WwiseRefType::Platform: return 0;
	case WwiseRefType::PluginInfo: return 0;
	case WwiseRefType::PluginLib: return 0;
	case WwiseRefType::SoundBanksInfo: return 0;
	case WwiseRefType::SoundBank: return 0;
	case WwiseRefType::Media: return 0;
	case WwiseRefType::CustomPlugin: return 0;
	case WwiseRefType::PluginShareSet: return 0;
	case WwiseRefType::AudioDevice: return 0;
	case WwiseRefType::Event: return 0;
	case WwiseRefType::SwitchContainer: return 0;
	case WwiseRefType::DialogueEvent: return 0;
	case WwiseRefType::DialogueArgument: return 0;
	case WwiseRefType::Bus: return 0;
	case WwiseRefType::AuxBus: return 0;
	case WwiseRefType::GameParameter: return 0;
	case WwiseRefType::StateGroup: return 0;
	case WwiseRefType::State: return GetStateGroup()->Id;
	case WwiseRefType::SwitchGroup: return 0;
	case WwiseRefType::Switch: return GetSwitchGroup()->Id;
	case WwiseRefType::Trigger: return 0;
	case WwiseRefType::ExternalSource: return 0;
	case WwiseRefType::AcousticTexture: return 0;
	case WwiseRefType::None:
	default:
		return 0;
	}
}

WwiseDBShortId WwiseAnyRef::GetId(const WwiseRefType* TypeOverride) const
{
	const auto Type = TypeOverride ? *TypeOverride : GetType();
	switch (Type)
	{
	case WwiseRefType::RootFile: return 0;
	case WwiseRefType::ProjectInfo: return 0;
	case WwiseRefType::Language: return GetLanguage()->Id;
	case WwiseRefType::PlatformInfo: return 0;
	case WwiseRefType::Platform: return 0;
	case WwiseRefType::PluginInfo: return 0;
	case WwiseRefType::PluginLib: return 0;
	case WwiseRefType::SoundBanksInfo: return 0;
	case WwiseRefType::SoundBank: return GetSoundBank()->Id;
	case WwiseRefType::Media: return GetMedia()->Id;
	case WwiseRefType::CustomPlugin: return GetCustomPlugin()->Id;
	case WwiseRefType::PluginShareSet: return GetPluginShareSet()->Id;
	case WwiseRefType::AudioDevice: return GetAudioDevice()->Id;
	case WwiseRefType::Event: return GetEvent()->Id;
	case WwiseRefType::SwitchContainer: return 0;
	case WwiseRefType::DialogueEvent: return GetDialogueEvent()->Id;
	case WwiseRefType::DialogueArgument: return GetDialogueArgument()->Id;
	case WwiseRefType::Bus: return GetBus()->Id;
	case WwiseRefType::AuxBus: return GetAuxBus()->Id;
	case WwiseRefType::GameParameter: return GetGameParameter()->Id;
	case WwiseRefType::StateGroup: return GetStateGroup()->Id;
	case WwiseRefType::State: return GetState()->Id;
	case WwiseRefType::SwitchGroup: return GetSwitchGroup()->Id;
	case WwiseRefType::Switch: return GetSwitch()->Id;
	case WwiseRefType::Trigger: return GetTrigger()->Id;
	case WwiseRefType::ExternalSource: return GetExternalSource()->Cookie;
	case WwiseRefType::AcousticTexture: return GetAcousticTexture()->Id;
	case WwiseRefType::None:
	default:
		return 0;
	}
}

WwiseDBString WwiseAnyRef::GetName(const WwiseRefType* TypeOverride) const
{
	const auto Type = TypeOverride ? *TypeOverride : GetType();
	switch (Type)
	{
	case WwiseRefType::RootFile: return {};
	case WwiseRefType::ProjectInfo: return {};
	case WwiseRefType::Language: return GetLanguage()->Name;
	case WwiseRefType::PlatformInfo: return {};
	case WwiseRefType::Platform: return GetPlatformReference()->Name;
	case WwiseRefType::PluginInfo: return {};
	case WwiseRefType::PluginLib: return {};
	case WwiseRefType::SoundBanksInfo: return {};
	case WwiseRefType::SoundBank: return GetSoundBank()->ShortName;
	case WwiseRefType::Media: return GetMedia()->ShortName;
	case WwiseRefType::CustomPlugin: return GetCustomPlugin()->Name;
	case WwiseRefType::PluginShareSet: return GetPluginShareSet()->Name;
	case WwiseRefType::AudioDevice: return GetAudioDevice()->Name;
	case WwiseRefType::Event: return GetEvent()->Name;
	case WwiseRefType::SwitchContainer: return {};
	case WwiseRefType::DialogueEvent: return GetDialogueEvent()->Name;
	case WwiseRefType::DialogueArgument: return GetDialogueArgument()->Name;
	case WwiseRefType::Bus: return GetBus()->Name;
	case WwiseRefType::AuxBus: return GetAuxBus()->Name;
	case WwiseRefType::GameParameter: return GetGameParameter()->Name;
	case WwiseRefType::StateGroup: return GetStateGroup()->Name;
	case WwiseRefType::State: return GetState()->Name;
	case WwiseRefType::SwitchGroup: return GetSwitchGroup()->Name;
	case WwiseRefType::Switch: return GetSwitch()->Name;
	case WwiseRefType::Trigger: return GetTrigger()->Name;
	case WwiseRefType::ExternalSource: return GetExternalSource()->Name;
	case WwiseRefType::AcousticTexture: return GetAcousticTexture()->Name;
	case WwiseRefType::None:
	default:
		return {};
	}
}

WwiseDBString WwiseAnyRef::GetObjectPath(const WwiseRefType* TypeOverride) const
{
	const auto Type = TypeOverride ? *TypeOverride : GetType();
	switch (Type)
	{
	case WwiseRefType::RootFile: return {};
	case WwiseRefType::ProjectInfo: return {};
	case WwiseRefType::Language: return {};
	case WwiseRefType::PlatformInfo: return {};
	case WwiseRefType::Platform: return {};
	case WwiseRefType::PluginInfo: return {};
	case WwiseRefType::PluginLib: return {};
	case WwiseRefType::SoundBanksInfo: return {};
	case WwiseRefType::SoundBank: return GetSoundBank()->ObjectPath;
	case WwiseRefType::Media: return {};
	case WwiseRefType::CustomPlugin: return GetCustomPlugin()->ObjectPath;
	case WwiseRefType::PluginShareSet: return GetPluginShareSet()->ObjectPath;
	case WwiseRefType::AudioDevice: return GetAudioDevice()->ObjectPath;
	case WwiseRefType::Event: return GetEvent()->ObjectPath;
	case WwiseRefType::SwitchContainer: return {};
	case WwiseRefType::DialogueEvent: return GetDialogueEvent()->ObjectPath;
	case WwiseRefType::DialogueArgument: return GetDialogueArgument()->ObjectPath;
	case WwiseRefType::Bus: return GetBus()->ObjectPath;
	case WwiseRefType::AuxBus: return GetAuxBus()->ObjectPath;
	case WwiseRefType::GameParameter: return GetGameParameter()->ObjectPath;
	case WwiseRefType::StateGroup: return GetStateGroup()->ObjectPath;
	case WwiseRefType::State: return GetState()->ObjectPath;
	case WwiseRefType::SwitchGroup: return GetSwitchGroup()->ObjectPath;
	case WwiseRefType::Switch: return GetSwitch()->ObjectPath;
	case WwiseRefType::Trigger: return GetTrigger()->ObjectPath;
	case WwiseRefType::ExternalSource: return GetExternalSource()->ObjectPath;
	case WwiseRefType::AcousticTexture: return GetAcousticTexture()->ObjectPath;
	case WwiseRefType::None:
	default:
		return {};
	}
}
