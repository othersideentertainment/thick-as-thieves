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

#include "Wwise/Ref/WwiseRefRootFile.h"

const WwiseDBString WwiseRefRootFile::NAME = "RootFile"_wwise_db;

WwiseDBShortId WwiseRefRootFile::Hash() const
{
	auto Result = WwiseDBHashCombine(GetTypeHash(JsonFilePath), GetTypeHash(static_cast<std::underlying_type<WwiseRefType>::type>(Type())));
	return Result;
}

bool WwiseRefRootFile::IsValid() const
{
	return static_cast<bool>(RootFileRef);
}

const WwiseMetadataRootFile* WwiseRefRootFile::GetRootFile() const
{
	const auto* Result = RootFileRef.Get();
	WWISE_DB_CLOG(!Result, Error, "Could not get Root File Ref");
	return Result;
}
