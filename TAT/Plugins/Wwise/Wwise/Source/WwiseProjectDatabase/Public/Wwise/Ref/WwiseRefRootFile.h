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

#include "Wwise/Metadata/WwiseMetadataCollections.h"
#include "Wwise/Ref/WwiseRefCollections.h"
#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"
#include "Wwise/Ref/WwiseRefType.h"

class WwiseRefRootFile
{
public:
	inline static const WwiseDBString emptyString = TEXT("");
	static const WwiseDBString NAME;
	static constexpr WwiseRefType TYPE = WwiseRefType::RootFile;

	WwiseMetadataSharedRootFileConstPtr RootFileRef;
	WwiseDBString JsonFilePath;

	WwiseRefRootFile() {}
	WwiseRefRootFile(const WwiseMetadataSharedRootFileConstPtr& InRootMediaRef, const WwiseDBString& InJsonFilePath) :
		RootFileRef(InRootMediaRef),
		JsonFilePath(InJsonFilePath)
	{}
	virtual ~WwiseRefRootFile() {}
	virtual WwiseDBShortId Hash() const;
	virtual WwiseRefType Type() const { return TYPE; }
	bool operator==(const WwiseRefRootFile& Rhs) const
	{
		return JsonFilePath == Rhs.JsonFilePath;
	}
	bool operator!=(const WwiseRefRootFile& Rhs) const { return !operator==(Rhs); }

	bool operator<(const WwiseRefRootFile& Rhs) const
	{
		return JsonFilePath < Rhs.JsonFilePath;
	}
	
	bool IsValid() const;
	const WwiseMetadataRootFile* GetRootFile() const;
};

inline WwiseDBShortId GetTypeHash(const WwiseRefRootFile& Type)
{
	return Type.Hash();
}
