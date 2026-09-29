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

#include "Wwise/Metadata/WwiseMetadataActionEntries.h"
#include "Wwise/Metadata/WwiseMetadataLoader.h"

WwiseMetadataActionPostEventEntry::WwiseMetadataActionPostEventEntry(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader)
{
	Loader.LogParsed("ActionPostEventEntry"_wwise_db, Id, Name);
}

WwiseMetadataActionSetFXEntry::WwiseMetadataActionSetFXEntry(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader)
{
	Loader.LogParsed("ActionSetFXEntry"_wwise_db, Id, Name);
}

WwiseMetadataActionSetStateEntry::WwiseMetadataActionSetStateEntry(WwiseMetadataLoader& Loader) :
	WwiseMetadataGroupValueReference(Loader)
{
	Loader.LogParsed("ActionSetStateEntry"_wwise_db, Id, Name);
}

WwiseMetadataActionSetSwitchEntry::WwiseMetadataActionSetSwitchEntry(WwiseMetadataLoader& Loader) :
	WwiseMetadataGroupValueReference(Loader)
{
	Loader.LogParsed("ActionSetSwitchEntry"_wwise_db, Id, Name);
}

WwiseMetadataActionTriggerEntry::WwiseMetadataActionTriggerEntry(WwiseMetadataLoader& Loader) :
	WwiseMetadataBasicReference(Loader)
{
	Loader.LogParsed("ActionTriggerEntry"_wwise_db, Id, Name);
}
