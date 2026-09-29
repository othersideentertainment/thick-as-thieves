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

#include "Wwise/Metadata/WwiseMetadataActionEntries.h"
#include "Wwise/Metadata/WwiseMetadataBus.h"
#include "Wwise/Metadata/WwiseMetadataSwitchContainer.h"

enum class WwiseMetadataEventDurationType : WwiseDBShortId
{
	OneShot = 0,
	Unknown = 1,
	Infinite = 2,
	Mixed = 3
};

struct WwiseMetadataEventReference : public WwiseMetadataBasicReference
{
	float MaxAttenuation;
	WwiseMetadataEventDurationType DurationType;
	float DurationMin;
	float DurationMax;

	WwiseMetadataEventReference(WwiseMetadataLoader& Loader);

private:
	static WwiseMetadataEventDurationType DurationTypeFromString(const WwiseDBString& TypeString);
};

struct WWISEPROJECTDATABASE_API WwiseMetadataEvent : public WwiseMetadataEventReference
{
	WwiseDBArray<WwiseMetadataMediaReference> MediaRefs;
	WwiseDBArray<WwiseMetadataExternalSourceReference> ExternalSourceRefs;
	WwiseMetadataPluginReferenceGroup* PluginRefs;
	WwiseDBArray<WwiseMetadataActionSetFXEntry> ActionSetFX; 
	WwiseMetadataEvent(WwiseMetadataLoader& Loader);
	WwiseDBArray<WwiseMetadataBusReference> AuxBusRefs;
	WwiseDBArray<WwiseMetadataSwitchContainer> SwitchContainers;
	WwiseDBArray<WwiseMetadataActionPostEventEntry> ActionPostEvent;
	WwiseDBArray<WwiseMetadataActionSetStateEntry> ActionSetState;
	WwiseDBArray<WwiseMetadataActionSetSwitchEntry> ActionSetSwitch;
	WwiseDBArray<WwiseMetadataActionTriggerEntry> ActionTrigger;


	bool IsMandatory() const;
};
