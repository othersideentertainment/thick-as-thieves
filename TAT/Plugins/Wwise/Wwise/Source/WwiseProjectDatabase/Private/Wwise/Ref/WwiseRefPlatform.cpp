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

#include "Wwise/Ref/WwiseRefPlatform.h"
#include "Wwise/Metadata/WwiseMetadataPlatform.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Metadata/WwiseMetadataProjectInfo.h"

const WwiseDBString WwiseRefPlatform::NAME = "Platform"_wwise_db;

void WwiseRefPlatform::Merge(WwiseRefPlatform&& InOtherPlatform)
{
	if (InOtherPlatform.ProjectInfo.IsValid() && InOtherPlatform.IsValid())
	{
		WWISE_DB_LOG(Error, "WwiseRefPlatform::Merge: Merging with a complete OtherPlatform.");
	}
	if (ProjectInfo.IsValid() && IsValid())
	{
		WWISE_DB_LOG(Error, "WwiseRefPlatform::Merge: Merging with a complete self.");
	}

	if (InOtherPlatform.IsValid())
	{
		if (IsValid())
		{
			WWISE_DB_LOG(Error, "WwiseRefPlatform::Merge: Already have a PlatformInfo. Overriding.");
		}
		RootFileRef = std::move(InOtherPlatform.RootFileRef);
		JsonFilePath = std::move(InOtherPlatform.JsonFilePath);
	}
	if (InOtherPlatform.ProjectInfo.IsValid())
	{
		if (ProjectInfo.IsValid())
		{
			WWISE_DB_LOG(Error, "WwiseRefPlatform::Merge: Already have a ProjectInfo. Overriding.");
		}
		ProjectInfo = std::move(InOtherPlatform.ProjectInfo);
		ProjectInfoPlatformReferenceIndex = std::move(InOtherPlatform.ProjectInfoPlatformReferenceIndex);
	}
}

const WwiseMetadataPlatform* WwiseRefPlatform::GetPlatform() const
{
	const auto* PlatformInfo = GetPlatformInfo();
	if (!PlatformInfo) [[unlikely]]
	{
		return nullptr;
	}
	return &PlatformInfo->Platform;
}

const WwiseMetadataPlatformReference* WwiseRefPlatform::GetPlatformReference() const
{
	const auto* GetProjectInfo = ProjectInfo.GetProjectInfo();
	if (!GetProjectInfo) [[unlikely]]
	{
		return nullptr;
	}
	const auto& Platforms = GetProjectInfo->Platforms;
	if (Platforms.IsValidIndex(ProjectInfoPlatformReferenceIndex))
	{
		return &Platforms[ProjectInfoPlatformReferenceIndex];
	}
	else
	{
		WWISE_DB_LOG(Error, "Could not get Platform Reference index #%zu", ProjectInfoPlatformReferenceIndex);
		return nullptr;
	}
}

const WwiseDBGuid* WwiseRefPlatform::PlatformGuid() const
{
	const auto* PlatformReference = GetPlatformReference();
	if (!PlatformReference) [[unlikely]]
	{
		return {};
	}
	return &PlatformReference->GUID;
}

const WwiseDBString* WwiseRefPlatform::PlatformName() const
{
	const auto* PlatformReference = GetPlatformReference();
	if (!PlatformReference) [[unlikely]]
	{
		return {};
	}
	return &PlatformReference->Name;
}

WwiseDBGuid WwiseRefPlatform::BasePlatformGuid() const
{
	const auto* PlatformReference = GetPlatformReference();
	if (!PlatformReference) [[unlikely]]
	{
		return {};
	}
	return PlatformReference->BasePlatformGUID;
}

WwiseDBString WwiseRefPlatform::BasePlatformName() const
{
	const auto* PlatformReference = GetPlatformReference();
	if (!PlatformReference) [[unlikely]]
	{
		return {};
	}
	return PlatformReference->BasePlatform;
}
WwiseDBShortId WwiseRefPlatform::Hash() const
{
	auto Result = WwiseRefPlatformInfo::Hash();
	Result = WwiseDBHashCombine(Result, ProjectInfo.Hash());
	Result = WwiseDBHashCombine(Result, GetTypeHash(ProjectInfoPlatformReferenceIndex));
	return Result;
}
