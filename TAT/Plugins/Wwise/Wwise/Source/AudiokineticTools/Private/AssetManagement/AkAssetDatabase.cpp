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

#include "AssetManagement/AkAssetDatabase.h"
#include "AkAudioType.h"
#include "AkAcousticTexture.h"
#include "AkAudioEvent.h"
#include "AkAuxBus.h"
#include "AkRtpc.h"
#include "AkStateValue.h"
#include "AkSwitchValue.h"
#include "AkTrigger.h"
#include "WwiseUnrealHelper.h"
#include "AkAudioDevice.h"
#include "AkSettingsPerUser.h"
#include "IAudiokineticTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Async/Async.h"
#include "Misc/FeedbackContext.h"
#include "ObjectTools.h"
#include "Logging/LogMacros.h"

#define LOCTEXT_NAMESPACE "AkAudio"

AkAssetDatabase& AkAssetDatabase::Get()
{
	static AkAssetDatabase instance;
	return instance;
}

AkAssetDatabase::AkAssetDatabase()
{
	AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetToolsModule = &FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
}

bool AkAssetDatabase::FindAllAssets(TArray<FAssetData>& OutData)
{
	AssetRegistryModule->Get().GetAssetsByClass(UAkAudioType::StaticClass()->GetClassPathName(), OutData, true);
	return OutData.Num() > 0;
}

bool AkAssetDatabase::FindAssets(const FGuid& AkGuid, TArray<FAssetData>& OutData)
{
	TMultiMap<FName, FString> Search;
	Search.Add(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseGuid), AkGuid.ToString(EGuidFormats::Digits));
	AssetRegistryModule->Get().GetAssetsByTagValues(Search, OutData);

	return OutData.Num() > 0;
}


bool AkAssetDatabase::FindAssets(const FString& AssetName, TArray<FAssetData>& OutData)
{
	TMultiMap<FName, FString> Search;
	Search.Add(GET_MEMBER_NAME_CHECKED(FAssetData, AssetName), AssetName);
	AssetRegistryModule->Get().GetAssetsByTagValues(Search, OutData);

	return OutData.Num() > 0;
}

FAssetData AkAssetDatabase::FindAssetByObjectPath(const FSoftObjectPath& AssetPath)
{
	return AssetRegistryModule->Get().GetAssetByObjectPath(AssetPath);
}

bool AkAssetDatabase::FindFirstAsset(const FGuid& AkGuid, FAssetData& OutAsset)
{
	TArray<FAssetData> Assets;
	if (FindAssets(AkGuid, Assets))
	{
		OutAsset = Assets[0];
		return true;
	}
	return false;
}

bool AkAssetDatabase::FindFirstAsset(const FString& AssetName, FAssetData& OutAsset)
{
	TArray<FAssetData> Assets;
	if (FindAssets(AssetName, Assets))
	{
		OutAsset = Assets[0];
		return true;
	}
	return false;
}

bool AkAssetDatabase::FindAssetsByGuidAndClass(const FGuid& AkGuid, const UClass* StaticClass, TArray<FAssetData>& OutWwiseAssets)
{
	TMultiMap<FName, FString> Search;
	FARFilter Filter;
	Filter.ClassPaths.Add(StaticClass->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.TagsAndValues.AddUnique(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseGuid), AkGuid.ToString(EGuidFormats::Digits));
	AssetRegistryModule->Get().GetAssets(Filter, OutWwiseAssets);

	return OutWwiseAssets.Num() > 0;
}

bool AkAssetDatabase::RenameAsset(const FGuid& Id, const FString& AssetName,
	const FString& RelativePath)
{
	check(IsInGameThread());

	auto parentPath = RelativePath;

	TArray<FAssetData> AssetData;
	if (!FindAssets(Id, AssetData))
	{
		UE_LOG(LogAudiokineticTools, Verbose, TEXT("Can't find Wwise asset to rename with ID %s"), *Id.ToString());
		return false;
	}

	TArray<FAssetRenameData> AssetsToRename;

	for (FAssetData Asset : AssetData)
	{
		if (Asset.AssetName.ToString() != AssetName || parentPath != Asset.PackagePath.ToString())
		{
			if (parentPath.IsEmpty())
			{
				parentPath = Asset.PackagePath.ToString();
			}

			FAssetRenameData NewAssetRenameData(Asset.GetAsset(), parentPath, AssetName);
			AssetsToRename.Add(NewAssetRenameData);
			UE_LOG(LogAudiokineticTools, Verbose, TEXT("Renaming Wwise asset %s"), *AssetName);
		}
	}

	if (!AssetToolsModule->Get().RenameAssets(AssetsToRename))
	{
		UE_LOG(LogAudiokineticTools, Error, TEXT("Failed to rename Wwise Assets"));
		return false;
	}

	return true;
}

void AkAssetDatabase::DeleteAsset(const FGuid& Id)
{
	check(IsInGameThread());

	TArray<FAssetData> AssetsToDelete;

	if (FindAssets(Id, AssetsToDelete))
	{
		ObjectTools::DeleteAssets(AssetsToDelete, true);
	}
}

void AkAssetDatabase::DeleteAssets(const TSet<FGuid>& AssetsId)
{

	for (auto& ID : AssetsId)
	{
		DeleteAsset(ID);
	}
}


void AkAssetDatabase::FixUpRedirectors(const FString& AssetPackagePath)
{
	TArray<UObjectRedirector*> redirectorsToFix;

	TArray<FAssetData> foundRedirectorsData;
	AssetRegistryModule->Get().GetAssetsByClass(UObjectRedirector::StaticClass()->GetClassPathName(), foundRedirectorsData);

	if (foundRedirectorsData.Num() > 0)
	{
		for (auto& entry : foundRedirectorsData)
		{
			if (auto redirector = Cast<UObjectRedirector>(entry.GetAsset()))
			{
				if (redirector->DestinationObject)
				{
					auto pathName = redirector->DestinationObject->GetPathName();
					if (pathName.StartsWith(AssetPackagePath))
					{
						redirectorsToFix.Add(redirector);
					}
				}
			}
		}
	}

	if (redirectorsToFix.Num() > 0)
	{
		AssetToolsModule->Get().FixupReferencers(redirectorsToFix);
	}
}

bool AkAssetDatabase::IsAkAudioType(const FAssetData& AssetData)
{
	static const TArray<FTopLevelAssetPath> AkAudioClassPaths = {
		UAkAcousticTexture::StaticClass()->GetClassPathName(),
		UAkAudioEvent::StaticClass()->GetClassPathName(),
		UAkAuxBus::StaticClass()->GetClassPathName(),
		UAkRtpc::StaticClass()->GetClassPathName(),
		UAkStateValue::StaticClass()->GetClassPathName(),
		UAkSwitchValue::StaticClass()->GetClassPathName(),
		UAkTrigger::StaticClass()->GetClassPathName()
	};

	if (AkAudioClassPaths.Contains(AssetData.AssetClassPath))
		return true;
	return false;
}

bool AkAssetDatabase::CheckIfLoadingAssets()
{
	return AssetRegistryModule->Get().IsLoadingAssets();
}

#undef LOCTEXT_NAMESPACE
