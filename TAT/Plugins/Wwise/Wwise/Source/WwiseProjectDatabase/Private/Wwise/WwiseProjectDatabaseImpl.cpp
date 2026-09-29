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

#include "Wwise/WwiseProjectDatabaseImpl.h"

#include "WwiseUnrealHelper.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseProjectDatabaseDelegates.h"

#include "Async/Async.h"
#include "Misc/ScopedSlowTask.h"
#include "Wwise/WwiseStringConverter.h"
#include "Wwise/Stats/ProjectDatabase.h"

#define LOCTEXT_NAMESPACE "WwiseProjectDatabase"

FWwiseProjectDatabaseImpl::FWwiseProjectDatabaseImpl() :
	ResourceLoaderOverride(nullptr),
	LockedDataStructure(new WwiseDataStructure())
{
}

FWwiseProjectDatabaseImpl::~FWwiseProjectDatabaseImpl()
{
}

void FWwiseProjectDatabaseImpl::UpdateDataStructure(const WwiseDBGuid* InBasePlatformGuid)
{
	SCOPED_WWISEPROJECTDATABASE_EVENT_2(TEXT("FWwiseProjectDatabaseImpl::UpdateDataStructure"));
	WwiseDBSharedPlatformId Platform;
	FDirectoryPath SourcePath;
	{
		FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
		if (UNLIKELY(!ResourceLoader))
		{
			return;
		}
		auto SharedPlatform = ResourceLoader->GetCurrentPlatform();
		Platform = WwiseDBSharedPlatformId(WwiseDBGuid(
			SharedPlatform.Platform->PlatformGuid.A, SharedPlatform.Platform->PlatformGuid.B,
			SharedPlatform.Platform->PlatformGuid.C, SharedPlatform.Platform->PlatformGuid.D),
			FWwiseStringConverter::ToWwiseDBString(*SharedPlatform.Platform->PlatformName.ToString()),
			FWwiseStringConverter::ToWwiseDBString(*SharedPlatform.Platform->PathRelativeToGeneratedSoundBanks.ToString()));
		Platform.Platform->ExternalSourceRootPath = FWwiseStringConverter::ToWwiseDBString(SharedPlatform.Platform->ExternalSourceRootPath.ToString());
		SourcePath = GetGeneratedSoundBanksPath();
	}

	{
		std::scoped_lock WLock(LockedDataStructure->Lock);
		auto& DataStructure = LockedDataStructure.Get();

		if (DisableDefaultPlatforms())
		{
			UE_LOG(LogWwiseProjectDatabase, Log, TEXT("UpdateDataStructure: Retrieving root data structure in (%s)"), *SourcePath.Path);
			FScopedSlowTask SlowTask(0, LOCTEXT("WwiseProjectDatabaseUpdate", "Retrieving Wwise data structure root..."));

			{
				WwiseDataStructure UpdatedDataStructure(FWwiseStringConverter::ToWwiseDBString(SourcePath.Path), nullptr);
				DataStructure = std::move(UpdatedDataStructure);
			}
		}
		else
		{
			UE_LOG(LogWwiseProjectDatabase, Log, TEXT("UpdateDataStructure: Retrieving data structure for %s (Base: %s) in (%s)"),
				*FWwiseStringConverter::ToFString(Platform.GetPlatformName()), InBasePlatformGuid ? *FWwiseStringConverter::ToFString(InBasePlatformGuid->ToString()) : TEXT("null"), *SourcePath.Path);
			FScopedSlowTask SlowTask(0, FText::Format(
				LOCTEXT("WwiseProjectDatabaseUpdate", "Retrieving Wwise data structure for platform {0}..."),
				FText::FromName(*Platform.GetPlatformName())));

			{
				WwiseDBString PlatformName = WwiseDBString(*Platform.GetPlatformName());
				WwiseDataStructure UpdatedDataStructure(FWwiseStringConverter::ToWwiseDBString(SourcePath.Path), &PlatformName);
				DataStructure = std::move(UpdatedDataStructure);

				// Update platform according to data found if different
				WwiseDBSharedPlatformId FoundSimilarPlatform = Platform;
				for (const auto& LoadedPlatform : DataStructure.Platforms)
				{
					WwiseDBPair<const WwiseDBSharedPlatformId, WwisePlatformDataStructure> LoadedPlatformPair(LoadedPlatform);
					FoundSimilarPlatform = LoadedPlatformPair.GetFirst();
					if (FoundSimilarPlatform == Platform)
					{
						break;
					}
				}

				//Update SharedPlatformId with parsed root paths
				if (DataStructure.Platforms.Contains(FoundSimilarPlatform))
				{
					const WwisePlatformDataStructure& PlatformEntry = DataStructure.Platforms.FindRef(FoundSimilarPlatform);
					if (auto PlatformInfo = PlatformEntry.PlatformRef.GetPlatformInfo())
					{
						if (UNLIKELY(!PlatformInfo))
						{
							UE_LOG(LogWwiseProjectDatabase, Warning,
								TEXT("Cannot set ExternalSourceRootPath for platform  %s, no PlatformInfo found."),
								*FoundSimilarPlatform.GetPlatformName());
						}
						else
						{
							FoundSimilarPlatform.Platform->ExternalSourceRootPath =
								PlatformInfo->RootPaths.ExternalSourcesOutputRoot;
						}
					}
				}
				//Update the resource loader current platform as internal data may have changed
				FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
				if (UNLIKELY(!ResourceLoader))
				{
					return;
				}

				int A, B, C, D;
				FoundSimilarPlatform.Platform->PlatformGuid.GetGuidValues(A, B, C, D);
				FWwiseSharedPlatformId SharedPlatform = FWwiseSharedPlatformId(FGuid(A, B, C, D),
					*FoundSimilarPlatform.Platform->PlatformName, *FoundSimilarPlatform.Platform->PathRelativeToGeneratedSoundBanks);
				SharedPlatform.Platform->ExternalSourceRootPath = FName(*FoundSimilarPlatform.Platform->ExternalSourceRootPath);
				ResourceLoader->SetPlatform(SharedPlatform);
			}

			if (UNLIKELY(DataStructure.Platforms.Size() == 0))
			{
				if(!InBasePlatformGuid)
				{
					WWISE_DB_LOG(Error, "JSON metadata files are not generated. Make sure the SoundBanks are generated and that the \"Generate JSON Metadata\" setting is enabled in your Wwise Project Settings, under the SoundBanks tab.");
					return;
				}
				UE_LOG(LogWwiseProjectDatabase, Error, TEXT("UpdateDataStructure: Could not find suitable platform for %s (Base: %s) in (%s)"),
					*FWwiseStringConverter::ToFString(Platform.GetPlatformName()), InBasePlatformGuid ? *FWwiseStringConverter::ToFString(InBasePlatformGuid->ToString()) : TEXT("null"), *SourcePath.Path);
				return;
			}
		}
		bIsDatabaseParsed = true;
		WWISE_DB_LOG(Log, "UpdateDataStructure: Done.");
	}
	if (Get() == this && bShouldBroadcast)		// Only broadcast database updates on main project.
	{
		//Stop multiple threads from Broadcasting this delegate at the same time.
		bShouldBroadcast = false;
		FWwiseProjectDatabaseDelegates::Get()->GetOnDatabaseUpdateCompletedDelegate().Broadcast();
		bShouldBroadcast = true;
	}
}

void FWwiseProjectDatabaseImpl::PrepareProjectDatabaseForPlatform(FWwiseResourceLoaderPtr&& InResourceLoader)
{
	ResourceLoaderOverride = InResourceLoader;
}

FWwiseResourceLoaderPtr FWwiseProjectDatabaseImpl::GetResourceLoader()
{
	if (ResourceLoaderOverride.IsValid())
	{
		return ResourceLoaderOverride;
	}
	else
	{
		return FWwiseResourceLoader::Get();
	}
}

const FWwiseResourceLoaderPtr FWwiseProjectDatabaseImpl::GetResourceLoader() const
{
	if (ResourceLoaderOverride.IsValid())
	{
		return ResourceLoaderOverride;
	}
	else
	{
		return FWwiseResourceLoader::Get();
	}
}

#undef LOCTEXT_NAMESPACE
