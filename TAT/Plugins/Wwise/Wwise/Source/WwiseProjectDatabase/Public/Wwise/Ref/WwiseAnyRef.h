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

#include "Wwise/Metadata/WwiseMetadataForwardDeclarations.h"
#include "Wwise/Ref/WwiseRefRootFile.h"
#include "Wwise/Ref/WwiseRefType.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/WwiseSharedPtr.h"

class WWISEPROJECTDATABASE_API WwiseAnyRef
{
public:
	WwiseDBSharedPtr<WwiseRefRootFile> Ref;

	template<typename WwiseRefType>
	static WwiseAnyRef Create(const WwiseRefType& inRef)
	{
		return WwiseAnyRef(new WwiseRefType(inRef));
	}

	WwiseAnyRef() :
		Ref()
	{}

	WwiseAnyRef(const WwiseAnyRef& InRef) :
		Ref(InRef.Ref)
	{}

private:
	WwiseAnyRef(WwiseRefRootFile*&& InRef) :
		Ref(InRef)
	{
	}

public:
	~WwiseAnyRef()
	{
	}

	WwiseRefType GetType() const
	{
		if (!Ref)
		{
			return WwiseRefType::None;
		}
		return (WwiseRefType)Ref->Type();
	}
	operator bool() const { return Ref != nullptr; }
	bool IsValid() const { return Ref != nullptr; }

	const WwiseRefLanguage* GetLanguageRef() const;
	const WwiseRefPlatform* GetPlatformRef() const;
	const WwiseRefPluginLib* GetPluginLibRef() const;
	const WwiseRefSoundBank* GetSoundBankRef() const;
	const WwiseRefMedia* GetMediaRef() const;
	const WwiseRefCustomPlugin* GetCustomPluginRef() const;
	const WwiseRefPluginShareSet* GetPluginShareSetRef() const;
	const WwiseRefAudioDevice* GetAudioDeviceRef() const;
	const WwiseRefEvent* GetEventRef() const;
	const WwiseRefSwitchContainer* GetSwitchContainerRef() const;
	const WwiseRefDialogueEvent* GetDialogueEventRef() const;
	const WwiseRefDialogueArgument* GetDialogueArgumentRef() const;
	const WwiseRefBus* GetBusRef() const;
	const WwiseRefAuxBus* GetAuxBusRef() const;
	const WwiseRefGameParameter* GetGameParameterRef() const;
	const WwiseRefStateGroup* GetStateGroupRef() const;
	const WwiseRefState* GetStateRef() const;
	const WwiseRefSwitchGroup* GetSwitchGroupRef() const;
	const WwiseRefSwitch* GetSwitchRef() const;
	const WwiseRefTrigger* GetTriggerRef() const;
	const WwiseRefExternalSource* GetExternalSourceRef() const;
	const WwiseRefAcousticTexture* GetAcousticTextureRef() const;

	const WwiseMetadataLanguage* GetLanguage() const;
	const WwiseMetadataPlatform* GetPlatform() const;
	const WwiseMetadataPlatformReference* GetPlatformReference() const;
	const WwiseMetadataPluginLib* GetPluginLib() const;
	const WwiseMetadataSoundBank* GetSoundBank() const;
	const WwiseMetadataMedia* GetMedia() const;
	const WwiseMetadataPlugin* GetCustomPlugin() const;
	const WwiseMetadataPlugin* GetPluginShareSet() const;
	const WwiseMetadataPlugin* GetAudioDevice() const;
	const WwiseMetadataEvent* GetEvent() const;
	const WwiseMetadataSwitchContainer* GetSwitchContainer() const;
	const WwiseMetadataDialogueEvent* GetDialogueEvent() const;
	const WwiseMetadataDialogueArgument* GetDialogueArgument() const;
	const WwiseMetadataBus* GetBus() const;
	const WwiseMetadataBus* GetAuxBus() const;
	const WwiseMetadataGameParameter* GetGameParameter() const;
	const WwiseMetadataStateGroup* GetStateGroup() const;
	const WwiseMetadataState* GetState() const;
	const WwiseMetadataSwitchGroup* GetSwitchGroup() const;
	const WwiseMetadataSwitch* GetSwitch() const;
	const WwiseMetadataTrigger* GetTrigger() const;
	const WwiseMetadataExternalSource* GetExternalSource() const;
	const WwiseMetadataAcousticTexture* GetAcousticTexture() const;

	bool GetRef(WwiseRefLanguage& OutRef) const;
	bool GetRef(WwiseRefPlatform& OutRef) const;
	bool GetRef(WwiseRefPluginLib& OutRef) const;
	bool GetRef(WwiseRefSoundBank& OutRef) const;
	bool GetRef(WwiseRefMedia& OutRef) const;
	bool GetRef(WwiseRefCustomPlugin& OutRef) const;
	bool GetRef(WwiseRefPluginShareSet& OutRef) const;
	bool GetRef(WwiseRefAudioDevice& OutRef) const;
	bool GetRef(WwiseRefEvent& OutRef) const;
	bool GetRef(WwiseRefSwitchContainer& OutRef) const;
	bool GetRef(WwiseRefDialogueEvent& OutRef) const;
	bool GetRef(WwiseRefDialogueArgument& OutRef) const;
	bool GetRef(WwiseRefBus& OutRef) const;
	bool GetRef(WwiseRefAuxBus& OutRef) const;
	bool GetRef(WwiseRefGameParameter& OutRef) const;
	bool GetRef(WwiseRefStateGroup& OutRef) const;
	bool GetRef(WwiseRefState& OutRef) const;
	bool GetRef(WwiseRefSwitchGroup& OutRef) const;
	bool GetRef(WwiseRefSwitch& OutRef) const;
	bool GetRef(WwiseRefTrigger& OutRef) const;
	bool GetRef(WwiseRefExternalSource& OutRef) const;
	bool GetRef(WwiseRefAcousticTexture& OutRef) const;

	WwiseDBGuid GetGuid(const WwiseRefType* TypeOverride = nullptr) const;
	WwiseDBShortId GetGroupId(const WwiseRefType* TypeOverride = nullptr) const;
	WwiseDBShortId GetId(const WwiseRefType* TypeOverride = nullptr) const;
	WwiseDBString GetName(const WwiseRefType* TypeOverride = nullptr) const;
	WwiseDBString GetObjectPath(const WwiseRefType* TypeOverride = nullptr) const;

	bool operator ==(const WwiseAnyRef& Rhs) const
	{
		return GetType() == Rhs.GetType()
			&& operator bool() == Rhs.operator bool()
			&& (!operator bool()
				|| Ref->Hash() == Rhs.Ref->Hash());
	}

	bool operator !=(const WwiseAnyRef& Rhs) const
	{
		return !(operator == (Rhs));
	}

};

inline WwiseDBShortId GetTypeHash(const WwiseAnyRef& InValue)
{
	if ((bool)InValue)
	{
		return InValue.Ref->Hash();
	}
	return 0;
}
