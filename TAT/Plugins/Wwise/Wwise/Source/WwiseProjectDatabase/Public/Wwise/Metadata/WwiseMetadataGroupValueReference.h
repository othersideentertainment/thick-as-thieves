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

#include "Wwise/Metadata/WwiseMetadataBasicReference.h"


struct WwiseMetadataGroupValueReference : public WwiseMetadataBasicReference
{
	WwiseDBShortId GroupId;

	WwiseMetadataGroupValueReference();
	WwiseMetadataGroupValueReference(WwiseMetadataLoader& Loader);
};

inline WwiseDBShortId GetTypeHash(const WwiseMetadataGroupValueReference& Ref)
{
	return HashCombine(GetTypeHash((const WwiseMetadataBasicReference&)Ref), GetTypeHash(Ref.Id));
}
inline bool operator==(const WwiseMetadataGroupValueReference& Lhs, const WwiseMetadataGroupValueReference& Rhs)
{
	return (const WwiseMetadataBasicReference&)Lhs == Rhs && Lhs.GroupId == Rhs.GroupId;
}
inline bool operator<(const WwiseMetadataGroupValueReference& Lhs, const WwiseMetadataGroupValueReference& Rhs)
{
	return (const WwiseMetadataBasicReference&)Lhs < Rhs && Lhs.GroupId < Rhs.GroupId;
}
