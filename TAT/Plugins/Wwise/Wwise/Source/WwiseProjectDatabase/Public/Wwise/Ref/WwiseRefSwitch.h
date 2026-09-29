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

#include "Wwise/Ref/WwiseRefSwitchGroup.h"

class WwiseRefSwitch : public WwiseRefSwitchGroup
{
public:
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::Switch;
	struct FGlobalIdsMap;

	WwiseRefIndexType SwitchIndex;

	WwiseRefSwitch() {}
	WwiseRefSwitch(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath,
		WwiseRefIndexType InSoundBankIndex, WwiseDBShortId InLanguageId,
		WwiseRefIndexType InSwitchGroupIndex,
		WwiseRefIndexType InSwitchIndex) :
		WwiseRefSwitchGroup(InRootMediaRef, InJsonFilePath, InSoundBankIndex, InLanguageId, InSwitchGroupIndex),
		SwitchIndex(InSwitchIndex)
	{}
	const WwiseMetadataSwitch* GetSwitch() const;

	WwiseDBShortId SwitchId() const;
	WwiseDBGuid SwitchGuid() const;
	WwiseDBString SwitchName() const;
	WwiseDBString SwitchObjectPath() const;

	WwiseDBShortId Hash() const override;
	WwiseRefType Type() const override { return TYPE; }
	bool operator==(const WwiseRefSwitch& Rhs) const
	{
		return WwiseRefSwitchGroup::operator ==(Rhs)
			&& SwitchIndex == Rhs.SwitchIndex;
	}
	bool operator!=(const WwiseRefSwitch& Rhs) const { return !operator==(Rhs); }
};

struct WwiseRefSwitch::FGlobalIdsMap
{
	WwiseSwitchGlobalIdsMap GlobalIdsMap;
};
