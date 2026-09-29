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

#include "Wwise/Metadata/WwiseMetadataEvent.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"
#include "Wwise/Metadata/WwiseMetadataPluginGroup.h"

WwiseMetadataEventReference::WwiseMetadataEventReference(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader),
	MaxAttenuation(Loader.GetFloat(this, "MaxAttenuation"_wwise_db, WwiseRequiredMetadata::Optional)),
	DurationType(DurationTypeFromString(Loader.GetString(this, "DurationType"_wwise_db))),
	DurationMin(Loader.GetFloat(this, "DurationMin"_wwise_db, WwiseRequiredMetadata::Optional)),
	DurationMax(Loader.GetFloat(this, "DurationMax"_wwise_db, WwiseRequiredMetadata::Optional))
{
	//IncLoadedSize(sizeof(WwiseMetadataEventDurationType));
	Loader.LogParsed("EventReference"_wwise_db, Id, Name);
}

WwiseMetadataEventDurationType WwiseMetadataEventReference::DurationTypeFromString(const WwiseDBString& TypeString)
{
	if (TypeString == "OneShot"_wwise_db)
	{
		return WwiseMetadataEventDurationType::OneShot;
	}
	else if (TypeString == "Infinite"_wwise_db)
	{
		return WwiseMetadataEventDurationType::Infinite;
	}
	else if (TypeString == "Mixed"_wwise_db)
	{
		return WwiseMetadataEventDurationType::Mixed;
	}
	else if (!(TypeString == "Unknown"_wwise_db))
	{
		WWISE_DB_LOG(Warning, "Wwise/Metadata/WwiseMetadataEventReference: Unknown DurationType: %s", *TypeString);
	}
	return WwiseMetadataEventDurationType::Unknown;
}

WwiseMetadataEvent::WwiseMetadataEvent(WwiseMetadataLoader& Loader) :
	WwiseMetadataEventReference(Loader),
	MediaRefs(Loader.GetArray<WwiseMetadataMediaReference>(this, "MediaRefs"_wwise_db)),
	ExternalSourceRefs(Loader.GetArray<WwiseMetadataExternalSourceReference>(this, "ExternalSourceRefs"_wwise_db)),
	PluginRefs(Loader.GetObjectPtr<WwiseMetadataPluginReferenceGroup>(this, "PluginRefs"_wwise_db)),
	ActionSetFX(Loader.GetArray<WwiseMetadataActionSetFXEntry>(this, "ActionSetFX"_wwise_db)),
	AuxBusRefs(Loader.GetArray<WwiseMetadataBusReference>(this, "AuxBusRefs"_wwise_db)),
	SwitchContainers(Loader.GetArray<WwiseMetadataSwitchContainer>(this, "SwitchContainers"_wwise_db)),
	ActionPostEvent(Loader.GetArray<WwiseMetadataActionPostEventEntry>(this, "ActionPostEvent"_wwise_db)),
	ActionSetState(Loader.GetArray<WwiseMetadataActionSetStateEntry>(this, "ActionSetState"_wwise_db)),
	ActionSetSwitch(Loader.GetArray<WwiseMetadataActionSetSwitchEntry>(this, "ActionSetSwitch"_wwise_db)),
	ActionTrigger(Loader.GetArray<WwiseMetadataActionTriggerEntry>(this, "ActionTrigger"_wwise_db))
{
	Loader.LogParsed("Event"_wwise_db, Id, Name);
}

bool WwiseMetadataEvent::IsMandatory() const
{
	return
		(ActionPostEvent.Size() > 0)
		|| (ActionSetState.Size() > 0)
		|| (ActionSetSwitch.Size() > 0)
		|| (ActionTrigger.Size() > 0)
		|| (ActionSetFX.Size() > 0)
		|| (AuxBusRefs.Size() > 0)
		|| (ExternalSourceRefs.Size() > 0)
		|| (MediaRefs.Size() > 0)
		|| (PluginRefs && (
			(PluginRefs->Custom.Size() > 0)
			|| (PluginRefs->ShareSets.Size() > 0)
			|| (PluginRefs->AudioDevices.Size() > 0)))
		|| (SwitchContainers.Size() == 0);
}