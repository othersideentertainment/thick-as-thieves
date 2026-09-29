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

#include "Wwise/Ref/WwiseRefStateGroup.h"

class WwiseRefState : public WwiseRefStateGroup
{
public:
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::State;
	struct FGlobalIdsMap;

	WwiseRefIndexType StateIndex;

	WwiseRefState() {}
	WwiseRefState(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath,
		WwiseRefIndexType InSoundBankIndex, WwiseDBShortId InLanguageId,
		WwiseRefIndexType InStateGroupIndex,
		WwiseRefIndexType InStateIndex) :
		WwiseRefStateGroup(InRootMediaRef, InJsonFilePath, InSoundBankIndex, InLanguageId, InStateGroupIndex),
		StateIndex(InStateIndex)
	{}
	const WwiseMetadataState* GetState() const;

	WwiseDBShortId StateId() const;
	WwiseDBGuid StateGuid() const;
	WwiseDBString StateName() const;
	WwiseDBString StateObjectPath() const;

	WwiseDBShortId Hash() const override;
	WwiseRefType Type() const override { return TYPE; }
	bool operator==(const WwiseRefState& Rhs) const
	{
		return WwiseRefStateGroup::operator ==(Rhs)
			&& StateIndex == Rhs.StateIndex;
	}
	bool operator!=(const WwiseRefState& Rhs) const { return !operator==(Rhs); }
};

struct WwiseRefState::FGlobalIdsMap
{
	WwiseStateGlobalIdsMap GlobalIdsMap;
};
