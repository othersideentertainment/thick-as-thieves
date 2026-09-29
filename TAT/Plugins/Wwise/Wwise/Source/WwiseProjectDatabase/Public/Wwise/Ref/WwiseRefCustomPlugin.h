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

#include "Wwise/Ref/WwiseRefSoundBank.h"

class WwiseRefCustomPlugin : public WwiseRefSoundBank
{
public:
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::CustomPlugin;
	struct FGlobalIdsMap;

	WwiseRefIndexType CustomPluginIndex;

	WwiseRefCustomPlugin() {}
	WwiseRefCustomPlugin(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath,
		WwiseRefIndexType InSoundBankIndex, WwiseDBShortId InLanguageId,
		WwiseRefIndexType InCustomPluginIndex) :
		WwiseRefSoundBank(InRootMediaRef, InJsonFilePath, InSoundBankIndex, InLanguageId),
		CustomPluginIndex(InCustomPluginIndex)
	{}
	const WwiseMetadataPlugin* GetPlugin() const;
	WwiseMediaIdsMap GetPluginMedia(const WwiseMediaGlobalIdsMap& GlobalMap) const;

	WwiseDBShortId CustomPluginId() const;
	WwiseDBGuid CustomPluginGuid() const;
	const WwiseDBString* CustomPluginName() const;
	const WwiseDBString* CustomPluginObjectPath() const;

	WwiseDBShortId Hash() const override;
	WwiseRefType Type() const override { return TYPE; }
	bool operator==(const WwiseRefCustomPlugin& Rhs) const
	{
		return WwiseRefSoundBank::operator==(Rhs)
			&& CustomPluginIndex == Rhs.CustomPluginIndex;
	}
	bool operator!=(const WwiseRefCustomPlugin& Rhs) const { return !operator==(Rhs); }
};

struct WwiseRefCustomPlugin::FGlobalIdsMap
{
	WwiseCustomPluginGlobalIdsMap GlobalIdsMap;
};
