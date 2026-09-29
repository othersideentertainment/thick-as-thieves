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

#include "AkUnrealAssetDataHelper.h"
#include "Wwise/WwiseReconcile.h"
#include "AkAudioEvent.h"
#include "AkAuxBus.h"
#include "AkEffectShareSet.h"
#include "AkGroupValue.h"
#include "AkSettings.h"
#include "AkStateValue.h"
#include "AkSwitchValue.h"
#include "AkTrigger.h"
#include "PackageTools.h"
#include "Wwise/Metadata/WwiseMetadataStateGroup.h"
#include "Wwise/Metadata/WwiseMetadataSwitchGroup.h"
#include "Wwise/Ref/WwiseAnyRef.h"
#include "Wwise/Ref/WwiseRefType.h"
#include "Wwise/Stats/Reconcile.h"

namespace AkUnrealAssetDataHelper
{
	bool IsSameType(const FAssetData& AssetData, EWwiseItemType::Type ItemType)
	{
		return GetUClassName(ItemType) == GetAssetClassName(AssetData);
	}

	FName GetUClassName(EWwiseItemType::Type ItemType)
	{
		UClass* Class = nullptr;
		switch (ItemType)
		{
		case EWwiseItemType::Event:
			Class = UAkAudioEvent::StaticClass();
			break;
		case EWwiseItemType::AuxBus:
			Class = UAkAuxBus::StaticClass();
			break;
		case EWwiseItemType::AcousticTexture:
			Class = UAkAcousticTexture::StaticClass();
			break;
		case EWwiseItemType::AudioDeviceShareSet:
			Class = UAkAudioDeviceShareSet::StaticClass();
			break;
		case EWwiseItemType::State:
			Class = UAkStateValue::StaticClass();
			break;
		case EWwiseItemType::Switch:
			Class = UAkSwitchValue::StaticClass();
			break;
		case EWwiseItemType::GameParameter:
			Class = UAkRtpc::StaticClass();
			break;
		case EWwiseItemType::Trigger:
			Class = UAkTrigger::StaticClass();
			break;
		case EWwiseItemType::EffectShareSet:
			Class = UAkEffectShareSet::StaticClass();
			break;
		}
		if (Class)
		{
			return Class->GetClassPathName().GetAssetName();
		}
		return FName();
	}

	FName GetAssetClassName(const FAssetData& AssetData)
	{
		return AssetData.AssetClassPath.GetAssetName();
	}

	bool IsAssetAkAudioType(const FAssetData& AssetData)
	{
		auto GuidValue = AssetData.TagsAndValues.FindTag(GET_MEMBER_NAME_CHECKED(FWwiseObjectInfo, WwiseGuid));
		return GuidValue.IsSet();
	}

	bool IsAssetTransient(const FAssetData& AssetData)
	{
		return AssetData.PackagePath.ToString() == UPackageTools::SanitizePackageName(GetTransientPackage()->GetPathName());
	}

	void SetAssetClassName(FAssetData& AssetData, UClass* Class)
	{
		AssetData.AssetClassPath = Class->GetClassPathName();
	}

	FString GetAssetDefaultPackagePath(const FAssetData& AssetData)
	{
		if (auto AkAudioAsset = Cast<UAkAudioType>(AssetData.GetAsset()))
		{
			return AkAudioAsset->GetAssetDefaultPackagePath().ToString();
		}

		return {};
	}

	FString GetAssetDefaultPackagePath(const WwiseAnyRef* WwiseRef)
	{
		auto AkSettings = GetMutableDefault<UAkSettings>();
		if (!AkSettings)
		{
			UE_LOG(LogWwiseReconcile, Error, TEXT("Could not fetch AkSettings"));
			return {};
		}

		FString DefaultPath = AkSettings->DefaultAssetCreationPath + FWwiseStringConverter::ToFString(WwiseRef->GetObjectPath());
		DefaultPath.ReplaceCharInline('\\', '/');
		int32 Index;
		if(DefaultPath.FindLastChar('/', Index))
		{
			DefaultPath = DefaultPath.Left(Index);
		}
		return UPackageTools::SanitizePackageName(DefaultPath);;
	}

	FName GetAssetDefaultName(const FAssetData& AssetData)
	{
		if (auto AkAudioAsset = Cast<UAkAudioType>(AssetData.GetAsset()))
		{
			return AkAudioAsset->GetAssetDefaultName();
		}

		return {};
	}

	FName GetAssetDefaultName(const WwiseAnyRef* WwiseRef)
	{
		WwiseRefType WwiseRefType = WwiseRef->GetType();
		FName WwiseName = FName(*WwiseRef->GetName());
		FNameBuilder DefaultName;

		switch (WwiseRefType)
		{
		case WwiseRefType::AcousticTexture:
		case WwiseRefType::AuxBus:
		case WwiseRefType::AudioDevice:
		case WwiseRefType::Event:
		case WwiseRefType::GameParameter:
		case WwiseRefType::PluginShareSet:
		case WwiseRefType::Trigger:
			return WwiseName;

		case WwiseRefType::Switch:
			{
			FString GroupName = FWwiseStringConverter::ToFString(WwiseRef->GetSwitchGroup()->Name);
			DefaultName << GroupName << TEXT("-") << WwiseName;
			return FName(DefaultName.ToView());
			}

		case WwiseRefType::State:
			{
			FString GroupName = FWwiseStringConverter::ToFString(WwiseRef->GetStateGroup()->Name);
			DefaultName << GroupName << TEXT("-") << WwiseName;
			return FName(DefaultName.ToView());
			}

		default:
			return {};
		}
	}
}
