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

#include "Wwise/Metadata/WwiseMetadataLoadable.h"
#include "Wwise/Metadata/WwiseMetadataGameParameter.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"

enum class WwiseRequiredMetadata
{
	Optional,
	Mandatory
};

struct IWwiseMetadataLoader
{
protected:
	~IWwiseMetadataLoader() = default;

public:
	
	virtual void Fail(const WwiseDBString& FieldName) = 0;
	virtual void LogParsed(const WwiseDBString& FieldName, const WwiseDBShortId Id = 0, const WwiseDBString& Name = WwiseDBString()) = 0;

	virtual bool GetBool(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) = 0;
	virtual float GetFloat(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) = 0;
	virtual WwiseDBGuid GetGuid(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) = 0;
	virtual int GetInt(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata	Required = WwiseRequiredMetadata::Mandatory) = 0;
	virtual WwiseDBString GetString(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) = 0;
	virtual WwiseDBShortId GetWwiseShortId(WwiseMetadataLoadable* Object, const WwiseDBString& FieldName, WwiseRequiredMetadata Required = WwiseRequiredMetadata::Mandatory) = 0;
};