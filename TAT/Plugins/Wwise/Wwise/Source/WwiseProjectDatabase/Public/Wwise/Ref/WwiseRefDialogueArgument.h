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

#include "Wwise/Ref/WwiseRefDialogueEvent.h"

class WwiseRefDialogueArgument : public WwiseRefDialogueEvent
{
public:
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::DialogueArgument;
	struct FGlobalIdsMap;

	WwiseRefIndexType DialogueArgumentIndex;

	WwiseRefDialogueArgument() {}
	WwiseRefDialogueArgument(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath,
		WwiseRefIndexType InSoundBankIndex, WwiseDBShortId InLanguageId,
		WwiseRefIndexType InDialogueEventIndex,
		WwiseRefIndexType InDialogueArgumentIndex) :
		WwiseRefDialogueEvent(InRootMediaRef, InJsonFilePath, InSoundBankIndex, InLanguageId, InDialogueEventIndex),
		DialogueArgumentIndex(InDialogueArgumentIndex)
	{}
	const WwiseMetadataDialogueArgument* GetDialogueArgument() const;

	WwiseDBShortId DialogueArgumentId() const;
	WwiseDBGuid DialogueArgumentGuid() const;
	WwiseDBString DialogueArgumentName() const;
	WwiseDBString DialogueArgumentObjectPath() const;

	WwiseDBShortId Hash() const override;
	WwiseRefType Type() const override { return TYPE; }
	bool operator==(const WwiseRefDialogueArgument& Rhs) const
	{
		return WwiseRefDialogueEvent::operator ==(Rhs)
			&& DialogueArgumentIndex == Rhs.DialogueArgumentIndex;
	}
	bool operator!=(const WwiseRefDialogueArgument& Rhs) const { return !operator==(Rhs); }
};

struct WwiseRefDialogueArgument::FGlobalIdsMap
{
	WwiseDialogueArgumentGlobalIdsMap GlobalIdsMap;
};
