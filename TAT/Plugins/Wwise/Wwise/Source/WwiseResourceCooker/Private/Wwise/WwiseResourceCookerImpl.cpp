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

#include "Wwise/WwiseResourceCookerImpl.h"

#include "Wwise/WwiseCookingCache.h"
#include "Wwise/WwiseExternalSourceManager.h"
#include "Wwise/WwiseFileCache.h"
#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseResourceLoader.h"
#include "Wwise/WwiseStringConverter.h"
#include "Wwise/Packaging/WwiseAssetLibraryInfo.h"
#include "Wwise/CookedData/WwiseAssetLibraryCookedData.h"
#include "Wwise/CookedData/WwiseSoundBankCookedData.h"
#include "Wwise/Metadata/WwiseMetadataPlatformInfo.h"
#include "Wwise/Metadata/WwiseMetadataPlugin.h"
#include "Wwise/Stats/ResourceCooker.h"
#include "Wwise/WwiseAllowShrinking.h"

#include "Async/Async.h"
#include "Async/MappedFileHandle.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

FWwiseResourceCookerImpl::FWwiseResourceCookerImpl() :
	ExportDebugNameRule(EWwiseExportDebugNameRule::ObjectPath),
	TargetPackagingStrategy(EWwisePackagingStrategy::Source),
	CookingCache(nullptr)
{
}

FWwiseResourceCookerImpl::~FWwiseResourceCookerImpl()
{
}


void FWwiseResourceCookerImpl::CookAuxBus(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookAuxBus: No CookingCache."));
		return;
	}

	if (const auto* CachedCookedData = CookingCache->AuxBusCache.Find(InInfo))
	{
		CookLocalizedAuxBusToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseLocalizedAuxBusCookedData CookedData;
		if (UNLIKELY(!GetAuxBusCookedData(CookedData,*GetOwnerContentFolder(Owner), InInfo)))
		{
			return;
		}

		CookingCache->AuxBusCache.Add(InInfo, CookedData);
		CookLocalizedAuxBusToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookEvent(const FWwiseEventInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookEvent: No CookingCache."));
		return;
	}

	if (const auto* CachedCookedData = CookingCache->EventCache.Find(InInfo))
	{
		CookLocalizedEventToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseLocalizedEventCookedData CookedData;
		if (UNLIKELY(!GetEventCookedData(CookedData, *GetOwnerContentFolder(Owner), InInfo)))
		{
			return;
		}

		CookingCache->EventCache.Add(InInfo, CookedData);
		CookLocalizedEventToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookExternalSource(uint32 InCookie, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookExternalSource: No CookingCache."));
		return;
	}

	if (const auto* CachedCookedData = CookingCache->ExternalSourceCache.Find(InCookie))
	{
		CookExternalSourceToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseExternalSourceCookedData CookedData;
		if (UNLIKELY(!GetExternalSourceCookedData(CookedData, InCookie)))
		{
			return;
		}

		CookingCache->ExternalSourceCache.Add(InCookie, CookedData);
		CookExternalSourceToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookInitBank(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookInitBank: No CookingCache."));
		return;
	}

	if (const auto* CachedCookedData = CookingCache->InitBankCache.Find(InInfo))
	{
		CookInitBankToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseInitBankCookedData CookedData;
		if (UNLIKELY(!GetInitBankCookedData(CookedData, *GetOwnerContentFolder(Owner), InInfo)))
		{
			return;
		}

		CookingCache->InitBankCache.Add(InInfo, CookedData);
		CookInitBankToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookMedia(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookMedia: No CookingCache."));
		return;
	}

	WwiseDatabaseMediaIdKey MediaKey;
	MediaKey.MediaId = InInfo.WwiseShortId;
	MediaKey.SoundBankId = InInfo.HardCodedSoundBankShortId;

	if (const auto* CachedCookedData = CookingCache->MediaCache.Find(MediaKey))
	{
		CookMediaToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseMediaCookedData CookedData;
		if (UNLIKELY(!GetMediaCookedData(CookedData, *GetOwnerContentFolder(Owner), InInfo)))
		{
			return;
		}

		CookingCache->MediaCache.Add(MediaKey, CookedData);
		CookMediaToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookShareSet(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookShareSet: No CookingCache."));
		return;
	}

	if (const auto* CachedCookedData = CookingCache->ShareSetCache.Find(InInfo))
	{
		CookLocalizedShareSetToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseLocalizedShareSetCookedData CookedData;
		if (UNLIKELY(!GetShareSetCookedData(CookedData, *GetOwnerContentFolder(Owner), InInfo)))
		{
			return;
		}

		CookingCache->ShareSetCache.Add(InInfo, CookedData);
		CookLocalizedShareSetToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookSoundBank(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("CookSoundBank: No CookingCache."));
		return;
	}

	if (const auto* CachedCookedData = CookingCache->SoundBankCache.Find(InInfo))
	{
		CookLocalizedSoundBankToSandbox(*CachedCookedData, PackageFilename, WriteAdditionalFile);
	}
	else
	{
		FWwiseLocalizedSoundBankCookedData CookedData;
		if (UNLIKELY(!GetSoundBankCookedData(CookedData, *GetOwnerContentFolder(Owner), InInfo)))
		{
			return;
		}

		CookingCache->SoundBankCache.Add(InInfo, CookedData);
		CookLocalizedSoundBankToSandbox(CookedData, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::PreCacheAssetLibraries(const IWwiseResourceCookerModule::FAssetLibraryInfoMap& AssetLibraryInfoMap)
{
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries: No CookingCache."));
		return;
	}

	for (const auto& AssetLibraryInfoTuple : AssetLibraryInfoMap)
	{
		auto* Owner = AssetLibraryInfoTuple.Key;
		FCookedAssetLibraryFiles Files;

		if (UNLIKELY(!AssetLibraryInfoTuple.Value.IsValid()))
		{
			UE_LOG(LogWwiseResourceCooker, Error, TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries(%s): No AssetLibraryInfo."), *Owner->GetName());
			return;
		}
		const auto& AssetLibraryInfo { *AssetLibraryInfoTuple.Value };
		PreCacheAssetLibraryInfo(Files, Owner, AssetLibraryInfo);

		CookingCache->CookedAssetLibraryMap.Add(Owner, Files.Array());
	}
}

void FWwiseResourceCookerImpl::PreCacheAssetLibraryInfo(FCookedAssetLibraryFiles& OutFiles, UObject* Owner, const FWwiseAssetLibraryInfo& AssetLibraryInfo)
{
	for (const auto& AssetLibraryRef : AssetLibraryInfo.FilteredAssets)
	{
		PreCacheAssetLibraryRef(OutFiles, Owner, AssetLibraryRef);
	}
}

void FWwiseResourceCookerImpl::PreCacheAssetLibraryRef(FCookedAssetLibraryFiles& OutFiles, UObject* Owner, const FWwiseAssetLibraryRef& AssetLibraryRef)
{
	const auto ObjectInfo = FWwiseObjectInfo(
		AssetLibraryRef.Guid,
		AssetLibraryRef.Id,
		AssetLibraryRef.Name,
		AssetLibraryRef.Type == EWwiseAssetLibraryRefType::Media ? AssetLibraryRef.SoundBankId : 0);
	
	switch (AssetLibraryRef.Type)
	{
	case EWwiseAssetLibraryRefType::Unknown:
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries(%s): Received an unknown library ref"), *Owner->GetName());
		break;
	case EWwiseAssetLibraryRefType::InitBank:
	case EWwiseAssetLibraryRefType::SoundBank:
		{
			FWwiseLocalizedSoundBankCookedData CookedData;
			if (UNLIKELY(!PrepareCookedData(CookedData, Owner, ObjectInfo)))
			{
				UE_LOG(LogWwiseResourceCooker, Warning,
					TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries(%s): Could not prepare SoundBank CookedData for Id %d"),
					*Owner->GetName(), AssetLibraryRef.Id);
				break;
			}
			if (AssetLibraryRef.LanguageId == 0)
			{
				for (auto& Language : CookedData.SoundBankLanguageMap)
				{
					for (auto& SoundBankTuple : CookedData.SoundBankLanguageMap)
					{
						auto& PackagedFile { SoundBankTuple.Value.PackagedFile };
						auto AssetLibraryPackagedFile { PackagedFile };					
						PackagedFile.PackagingStrategy = EWwisePackagingStrategy::External;
						UpdateAssetLibraryPackagedFile(AssetLibraryPackagedFile);

						OutFiles.Add(AssetLibraryPackagedFile);
					}
				}
			}
			else
			{
				FWwiseSoundBankCookedData* FoundCookedData { nullptr };
				for (auto& SoundBankTuple : CookedData.SoundBankLanguageMap)
				{
					if (SoundBankTuple.Key.GetLanguageId() == AssetLibraryRef.LanguageId)
					{
						FoundCookedData = &SoundBankTuple.Value;
						break;
					}
				}
				if (!FoundCookedData)
				{
					UE_LOG(LogWwiseResourceCooker, Warning,
						TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries(%s): Could not find language %d in SoundBank CookedData for Id %d"),
						*Owner->GetName(), AssetLibraryRef.LanguageId, AssetLibraryRef.Id);
					break;
				}
				auto& PackagedFile { FoundCookedData->PackagedFile };
				auto AssetLibraryPackagedFile { PackagedFile };					
				PackagedFile.PackagingStrategy = EWwisePackagingStrategy::External;
				UpdateAssetLibraryPackagedFile(AssetLibraryPackagedFile);

				OutFiles.Add(AssetLibraryPackagedFile);
			}

			// Update cache so the assets will get the External packaging strategy 
			CookingCache->SoundBankCache[ObjectInfo] = CookedData;
		}
		break;
	case EWwiseAssetLibraryRefType::Media:
		{
			FWwiseMediaCookedData CookedData;
			if (UNLIKELY(!PrepareCookedData(CookedData, Owner, ObjectInfo)))
			{
				UE_LOG(LogWwiseResourceCooker, Warning,
					TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries(%s): Could not prepare Media CookedData for Id %d"),
					*Owner->GetName(), AssetLibraryRef.Id);
				break;
			}
			auto& PackagedFile { CookedData.PackagedFile };
			auto AssetLibraryPackagedFile { PackagedFile };					
			PackagedFile.PackagingStrategy = EWwisePackagingStrategy::External;
			UpdateAssetLibraryPackagedFile(AssetLibraryPackagedFile);

			OutFiles.Add(AssetLibraryPackagedFile);
			CookingCache->MediaCache[WwiseDatabaseMediaIdKey(ObjectInfo.WwiseShortId, ObjectInfo.HardCodedSoundBankShortId)] = CookedData;
		}
		break;
	case EWwiseAssetLibraryRefType::ExternalSource:
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("FWwiseResourceCookerImpl::PreCacheAssetLibraries(%s): Received an unsupported ExtSrc"), *Owner->GetName());
		break;
	}
}

void FWwiseResourceCookerImpl::UpdateAssetLibraryPackagedFile(FWwisePackagedFile& File)
{
	File.PackagingStrategy = EWwisePackagingStrategy::BulkData;
}

void FWwiseResourceCookerImpl::PrepareAssetLibrary(UObject* Owner, FWwiseAssetLibraryCookedData& OutCookedData, bool bPackageAssets)
{
	if (!bPackageAssets)
	{
		UE_LOG(LogWwiseResourceCooker, Display, TEXT("FWwiseResourceCookerImpl::PrepareAssetLibrary(%s): Skipping. \"Package Assets\" option is disabled for library."), *Owner->GetName());
		OutCookedData.PackagedFiles.Empty();
		return;
	}
	
	if (UNLIKELY(!CookingCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("FWwiseResourceCookerImpl::PrepareAssetLibrary(%s): No CookingCache."), *Owner->GetName());
		return;
	}

	const auto* CachedCookedData = CookingCache->CookedAssetLibraryMap.Find(Owner);

	if (UNLIKELY(!CachedCookedData))
	{
		UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("FWwiseResourceCookerImpl::PrepareAssetLibrary(%s): Cannot find AssetLibrary in Map. Is Package as Bulk Data disabled?"), *Owner->GetName());
		return;
	}

	OutCookedData.PackagedFiles = *CachedCookedData;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseAcousticTextureCookedData& OutCookedData,
	const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetAcousticTextureCookedData(OutCookedData, InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->AcousticTextureCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetAcousticTextureCookedData(OutCookedData, InInfo)))
	{
		CookingCache->AcousticTextureCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseAudioDeviceShareSetCookedData& OutCookedData, const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetAudioDeviceShareSetCookedData(OutCookedData, InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->AudioDeviceShareSetCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	else if (LIKELY(GetAudioDeviceShareSetCookedData(OutCookedData, InInfo)))
	{
		CookingCache->AudioDeviceShareSetCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseLocalizedAuxBusCookedData& OutCookedData, const UObject* Owner,
                                                 const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetAuxBusCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->AuxBusCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetAuxBusCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo)))
	{
		CookingCache->AuxBusCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseLocalizedEventCookedData& OutCookedData, const UObject* Owner, const FWwiseEventInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetEventCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->EventCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetEventCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo)))
	{
		CookingCache->EventCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseExternalSourceCookedData& OutCookedData, uint32 InCookie)
{
	if (!CookingCache)
	{
		return GetExternalSourceCookedData(OutCookedData, InCookie);
	}

	if (const auto* CachedCookedData = CookingCache->ExternalSourceCache.Find(InCookie))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetExternalSourceCookedData(OutCookedData, InCookie)))
	{
		CookingCache->ExternalSourceCache.Add(InCookie, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseGameParameterCookedData& OutCookedData,
	const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetGameParameterCookedData(OutCookedData, InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->GameParameterCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetGameParameterCookedData(OutCookedData, InInfo)))
	{
		CookingCache->GameParameterCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseGroupValueCookedData& OutCookedData,
	const FWwiseGroupValueInfo& InInfo, EWwiseGroupType InGroupType)
{
	if (InGroupType == EWwiseGroupType::State)
	{
		if (!CookingCache)
		{
			return GetStateCookedData(OutCookedData, InInfo);
		}

		if (const auto* CachedCookedData = CookingCache->StateCache.Find(InInfo))
		{
			OutCookedData = *CachedCookedData;
			return true;
		}
		if (LIKELY(GetStateCookedData(OutCookedData, InInfo)))
		{
			CookingCache->StateCache.Add(InInfo, OutCookedData);
			return true;
		}
	}
	else if (InGroupType == EWwiseGroupType::Switch)
	{
		if (!CookingCache)
		{
			return GetSwitchCookedData(OutCookedData, InInfo);
		}

		if (const auto* CachedCookedData = CookingCache->SwitchCache.Find(InInfo))
		{
			OutCookedData = *CachedCookedData;
			return true;
		}
		if (LIKELY(GetSwitchCookedData(OutCookedData, InInfo)))
		{
			CookingCache->SwitchCache.Add(InInfo, OutCookedData);
			return true;
		}
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseInitBankCookedData& OutCookedData, const UObject* Owner,
	const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetInitBankCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->InitBankCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetInitBankCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo)))
	{
		CookingCache->InitBankCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseMediaCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetMediaCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo);
	}

	WwiseDatabaseMediaIdKey MediaIdKey;
	MediaIdKey.MediaId = InInfo.WwiseShortId;
	MediaIdKey.SoundBankId = InInfo.HardCodedSoundBankShortId;

	if (const auto* CachedCookedData = CookingCache->MediaCache.Find(MediaIdKey))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetMediaCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo)))
	{
		CookingCache->MediaCache.Add(MediaIdKey, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseLocalizedShareSetCookedData& OutCookedData, const UObject* Owner,
	const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetShareSetCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->ShareSetCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetShareSetCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo)))
	{
		CookingCache->ShareSetCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseLocalizedSoundBankCookedData& OutCookedData, const UObject* Owner,
	const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetSoundBankCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->SoundBankCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetSoundBankCookedData(OutCookedData, *GetOwnerContentFolder(Owner), InInfo)))
	{
		CookingCache->SoundBankCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

bool FWwiseResourceCookerImpl::PrepareCookedData(FWwiseTriggerCookedData& OutCookedData, const FWwiseObjectInfo& InInfo)
{
	if (!CookingCache)
	{
		return GetTriggerCookedData(OutCookedData, InInfo);
	}

	if (const auto* CachedCookedData = CookingCache->TriggerCache.Find(InInfo))
	{
		OutCookedData = *CachedCookedData;
		return true;
	}
	if (LIKELY(GetTriggerCookedData(OutCookedData, InInfo)))
	{
		CookingCache->TriggerCache.Add(InInfo, OutCookedData);
		return true;
	}
	return false;
}

FWwiseProjectDatabase* FWwiseResourceCookerImpl::GetProjectDatabase()
{
	if (ProjectDatabaseOverride.IsValid())
	{
		return ProjectDatabaseOverride.Get();
	}
	return FWwiseProjectDatabase::Get();
}

const FWwiseProjectDatabase* FWwiseResourceCookerImpl::GetProjectDatabase() const
{
	if (ProjectDatabaseOverride.IsValid())
	{
		return ProjectDatabaseOverride.Get();
	}
	return FWwiseProjectDatabase::Get();
}

FWwiseResourceLoaderPtr FWwiseResourceCookerImpl::GetResourceLoader()
{
	if (auto* ProjectDatabase = GetProjectDatabase())
	{
		return ProjectDatabase->GetResourceLoader();
	}
	return FWwiseResourceLoader::Get();
}

const FWwiseResourceLoaderPtr FWwiseResourceCookerImpl::GetResourceLoader() const
{
	if (const auto* ProjectDatabase = GetProjectDatabase())
	{
		return ProjectDatabase->GetResourceLoader();
	}
	return FWwiseResourceLoader::Get();
}

FWwiseSharedLanguageId FWwiseResourceCookerImpl::GetCurrentLanguage() const
{
	if (const auto* ProjectDatabase = GetProjectDatabase())
	{
		return ProjectDatabase->GetCurrentLanguage();
	}
	return {};
}

FWwiseSharedPlatformId FWwiseResourceCookerImpl::GetCurrentPlatform() const
{
	if (const auto* ProjectDatabase = GetProjectDatabase())
	{
		return ProjectDatabase->GetCurrentPlatform();
	}
	return {};
}

FString FWwiseResourceCookerImpl::GetOwnerContentFolder(const UObject* Owner) const
{
	FString ContentFolder = "";
	FString CleanedPath = Owner->GetPathName();
	if (CleanedPath.StartsWith("/"))
	{
		CleanedPath = CleanedPath.RightChop(1);
	}
	TArray<FString> PathParts;
	CleanedPath.ParseIntoArray(PathParts, TEXT("/"));

	if (PathParts.Num() > 0)
	{
		ContentFolder = PathParts[0];
	}
	else
	{
		ContentFolder.Empty();
	}

	//Extract the full plugin folder using the name.
	if(!ContentFolder.IsEmpty() && ContentFolder != "Game" && ContentFolder != "Engine")
	{
		const auto PluginInfo = IPluginManager::Get().FindPlugin(ContentFolder);
		const FString FullPluginPath = PluginInfo->GetDescriptorFileName();
		const FString PluginsFolderName = "Plugins";
		const int32 StartIndex = FullPluginPath.Find(PluginsFolderName) + PluginsFolderName.Len() + 1;
		int32 EndIndex = FullPluginPath.Find(ContentFolder);
		EndIndex += ContentFolder.Len();
		if (StartIndex != -1 && EndIndex != -1)
		{
			ContentFolder = FullPluginPath.Mid(StartIndex, EndIndex - StartIndex);
		}
	}
	
	
	return ContentFolder;
}


void FWwiseResourceCookerImpl::CookLocalizedAuxBusToSandbox(const FWwiseLocalizedAuxBusCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	for (const auto& AuxBus : InCookedData.AuxBusLanguageMap)
	{
		UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking AuxBus %s in %s %" PRIu32),
			*InCookedData.DebugName.ToString(), *AuxBus.Key.GetLanguageName().ToString(),
			(uint32)AuxBus.Key.GetLanguageId());
		CookAuxBusToSandbox(AuxBus.Value, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookLocalizedEventToSandbox(const FWwiseLocalizedEventCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	for (const auto& Event : InCookedData.EventLanguageMap)
	{
		UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking Event %s in %s %" PRIu32),
			*InCookedData.DebugName.ToString(), *Event.Key.GetLanguageName().ToString(),
			(uint32)Event.Key.GetLanguageId());
		CookEventToSandbox(Event.Value, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookLocalizedShareSetToSandbox(const FWwiseLocalizedShareSetCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	for (const auto& ShareSet : InCookedData.ShareSetLanguageMap)
	{
		UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking ShareSet %s in %s %" PRIu32),
			*InCookedData.DebugName.ToString(), *ShareSet.Key.GetLanguageName().ToString(),
			(uint32)ShareSet.Key.GetLanguageId());
		CookShareSetToSandbox(ShareSet.Value, PackageFilename, WriteAdditionalFile);
	}
}

void FWwiseResourceCookerImpl::CookLocalizedSoundBankToSandbox(const FWwiseLocalizedSoundBankCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	for (const auto& SoundBank : InCookedData.SoundBankLanguageMap)
	{
		UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking SoundBank %s in %s %" PRIu32),
			*InCookedData.DebugName.ToString(), *SoundBank.Key.GetLanguageName().ToString(),
			(uint32)SoundBank.Key.GetLanguageId());
		CookSoundBankToSandbox(SoundBank.Value, PackageFilename, WriteAdditionalFile);
	}
}

FString FWwiseResourceCookerImpl::GetCookRootPath(const TCHAR* PackageFilename)
{
	auto Path = FPaths::GetPath(PackageFilename); // Remove Filename.uasset
	while (!Path.IsEmpty() && FPaths::GetCleanFilename(Path) != TEXT("Content"))
	{
		Path = FPaths::GetPath(Path);
	}

	return Path;
}

void FWwiseResourceCookerImpl::SetWwiseStagePath(const FString& InPath)
{
	UE_CLOG(InPath != WwiseStagePath, LogWwiseResourceCooker, Display,
		TEXT("FWwiseResourceCookerImpl::SetWwiseStagePath: Updating path to \"%s\""), *InPath);
	WwiseStagePath = InPath;
}

FString FWwiseResourceCookerImpl::GetWwiseStagePath() const
{
	return WwiseStagePath;
}

void FWwiseResourceCookerImpl::UpdatePackagedFile(FWwisePackagedFile& OutPackagedFile,
	const WwiseMetadataMedia& InMedia) const
{
	auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("UpdatePackagedFile: Could not get Project Database"));
		return;
	}
	FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
	if (UNLIKELY(!ResourceLoader))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("UpdatePackagedFile: Could not get Resource Loader"));
		return;
	}

	FString Path;
	if (InMedia.Path.IsEmpty())
	{
		if (UNLIKELY(InMedia.CachePath.IsEmpty()))
		{
			UE_LOG(LogWwiseResourceCooker, Warning, TEXT("UpdatePackagedFile: Empty path for Media %" PRIu32 "."),
				InMedia.Id);
			return;
		}
		Path = *InMedia.CachePath;
	}
	else
	{
		Path = *InMedia.Path;
	}

	auto SourcePathName = ResourceLoader->GetCurrentPlatform().Platform->PathRelativeToGeneratedSoundBanks.ToString() /
		Path;
	if (FPaths::IsRelative(SourcePathName))
	{
		SourcePathName = ProjectDatabase->GetGeneratedSoundBanksPathFor(SourcePathName);
	}
	OutPackagedFile.SourcePathName = SourcePathName;

	if (bCookingForPackaging && TargetPackagingStrategy != EWwisePackagingStrategy::AdditionalFile)
	{
		bool bUnique;
		{
			WwiseDataStructureScopeLock Lock(*ProjectDatabase);
			OutPackagedFile.WwiseProjectUsageCount = Lock.GetMediaUsageCount(InMedia.Id);
			bUnique = (OutPackagedFile.WwiseProjectUsageCount == 1);
		}
		if (bUnique)
		{
			OutPackagedFile.PackagingStrategy = TargetPackagingStrategy;
		}
		else
		{
			OutPackagedFile.PackagingStrategy = EWwisePackagingStrategy::BulkData;
		}
	}

	switch (OutPackagedFile.PackagingStrategy)
	{
	case EWwisePackagingStrategy::AdditionalFile:
	case EWwisePackagingStrategy::HybridAdditionalFile:
		OutPackagedFile.PathName = FName{GetWwiseStagePath() / Path};
		break;
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::BulkData:
	case EWwisePackagingStrategy::External:
		OutPackagedFile.PathName = FName{};
		break;
	}

	// Check if the Media file's size is equal to the prefetch
	UpdatePackagedFileStreaming(OutPackagedFile, InMedia.Id);
	
	OutPackagedFile.Hash = GetTypeHash(OutPackagedFile);

	UE_LOG(LogWwiseResourceCooker, VeryVerbose,
		TEXT("FWwiseResourceCookerImpl::UpdatePackagedFile (Media %s %" PRIu32 "): Source: %s. Packaged: %s."),
		*FString(*InMedia.ShortName), InMedia.Id, *OutPackagedFile.SourcePathName,
		*OutPackagedFile.PathName.ToString());
}

void FWwiseResourceCookerImpl::UpdatePackagedFile(FWwisePackagedFile& OutPackagedFile,
	const WwiseMetadataSoundBank& InSoundBank) const
{
	auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("UpdatePackagedFile: Could not get Project Database"));
		return;
	}
	FWwiseResourceLoaderPtr ResourceLoader = GetResourceLoader();
	if (UNLIKELY(!ResourceLoader))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("UpdatePackagedFile: Could not get Resource Loader"));
		return;
	}

	const FString Path = *InSoundBank.Path;

	auto SourcePathName = ResourceLoader->GetCurrentPlatform().Platform->PathRelativeToGeneratedSoundBanks.ToString() / Path;
	if (FPaths::IsRelative(SourcePathName))
	{
		SourcePathName = ProjectDatabase->GetGeneratedSoundBanksPathFor(SourcePathName);
	}
	OutPackagedFile.SourcePathName = SourcePathName;

	if (bCookingForPackaging && TargetPackagingStrategy != EWwisePackagingStrategy::AdditionalFile)
	{
		bool bUnique;
		{
			WwiseDataStructureScopeLock Lock(*ProjectDatabase);
			OutPackagedFile.WwiseProjectUsageCount = Lock.GetSoundBankUsageCount(InSoundBank.Id, InSoundBank.Language);
			bUnique = (OutPackagedFile.WwiseProjectUsageCount == 1);
		}
		if (bUnique)
		{
			OutPackagedFile.PackagingStrategy = TargetPackagingStrategy;
		}
		else
		{
			OutPackagedFile.PackagingStrategy = EWwisePackagingStrategy::BulkData;
		}
	}
	
	switch (OutPackagedFile.PackagingStrategy)
	{
	case EWwisePackagingStrategy::AdditionalFile:
	case EWwisePackagingStrategy::HybridAdditionalFile:
		OutPackagedFile.PathName = FName(GetWwiseStagePath() / Path);
		break;
	case EWwisePackagingStrategy::Source:
	case EWwisePackagingStrategy::BulkData:
	case EWwisePackagingStrategy::External:
		OutPackagedFile.PathName = FName{};
		break;
	}
	
	OutPackagedFile.Hash = GetTypeHash(OutPackagedFile);

	UE_LOG(LogWwiseResourceCooker, VeryVerbose,
		TEXT("FWwiseResourceCookerImpl::UpdatePackagedFile (SoundBank %s %" PRIu32 "): Source: %s. Packaged: %s."),
		*FString(*InSoundBank.ShortName), InSoundBank.Id, *OutPackagedFile.SourcePathName,
		*OutPackagedFile.PathName.ToString());
}

void FWwiseResourceCookerImpl::UpdatePackagedFileStreaming(FWwisePackagedFile& OutPackagedFile, uint32 Id) const
{
	if (!OutPackagedFile.bStreaming || OutPackagedFile.PrefetchSize == 0)
	{
		UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": No streaming / PrefetchSize for %s."), Id, *OutPackagedFile.SourcePathName);
		return;
	}
	
	FWwiseFileCache* FileCache = FWwiseFileCache::Get();
	if (UNLIKELY(!FileCache))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": WwiseFileCache not available."), Id);
		return;
	}
	IWwiseFileCacheHandle* StreamedFile{ nullptr };
	if (UNLIKELY(OutPackagedFile.SourcePathName.IsEmpty()))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": Trying to stream a nameless file."), Id);
		return;
	}

	FEventRef Wait;
	FileCache->CreateFileCacheHandle(StreamedFile, OutPackagedFile.SourcePathName, [&Wait, &StreamedFile](bool bResult) mutable
	{
		if (!bResult)
		{
			StreamedFile->CloseAndDelete();
			StreamedFile = nullptr;
		}
		return Wait->Trigger();
	});
	Wait->Wait();

	if (UNLIKELY(!StreamedFile))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": Could not open file %s."), Id, *OutPackagedFile.SourcePathName);
		return;
	}

	const auto FileSize = static_cast<int32>(StreamedFile->GetFileSize());
	if (UNLIKELY(OutPackagedFile.PrefetchSize > FileSize))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": PrefetchSize %" PRIi32 " > FileSize %" PRIi32 " for %s. Removing streaming."),
			Id, OutPackagedFile.PrefetchSize, FileSize, *OutPackagedFile.SourcePathName);
		OutPackagedFile.PrefetchSize = 0;
		OutPackagedFile.bStreaming = false;
	}
	else if (UNLIKELY(OutPackagedFile.PrefetchSize == FileSize))
	{
		UE_LOG(LogWwiseResourceCooker, Display,
			TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": PrefetchSize %" PRIi32 " == FileSize %" PRIi32 " for %s. Removing streaming."),
			Id, OutPackagedFile.PrefetchSize, FileSize, *OutPackagedFile.SourcePathName);
		OutPackagedFile.PrefetchSize = 0;
		OutPackagedFile.bStreaming = false;
	}
	else
	{
		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("FWwiseResourceCookerImpl::UpdatePackagedFileStreaming Media %" PRIu32 ": PrefetchSize %" PRIi32 " < FileSize %" PRIi32 " for %s."),
			Id, OutPackagedFile.PrefetchSize, FileSize, *OutPackagedFile.SourcePathName);
	}
	StreamedFile->CloseAndDelete();
}

void FWwiseResourceCookerImpl::InitForStaging(const IWwiseResourceCooker& DefaultResourceCooker)
{
	SetWwiseStagePath(DefaultResourceCooker.GetWwiseStagePath());
}

void FWwiseResourceCookerImpl::PrepareResourceCookerForPlatform(FWwiseProjectDatabase*&& InProjectDatabaseOverride,
	EWwisePackagingStrategy InTargetPackagingStrategy, EWwiseExportDebugNameRule InExportDebugNameRule)
{
	ProjectDatabaseOverride.Reset(InProjectDatabaseOverride);
	ExportDebugNameRule = InExportDebugNameRule;
	CookingCache.Reset(new FWwiseCookingCache);
	CookingCache->ExternalSourceManager = IWwiseExternalSourceManager::Get();
	bCookingForPackaging = true;
	TargetPackagingStrategy = InTargetPackagingStrategy;

	{
		int32 CookProcessCount=-1;
		FParse::Value(FCommandLine::Get(), TEXT("-CookProcessCount="), CookProcessCount);
		if (CookProcessCount < 0)
		{
			GConfig->GetInt(TEXT("CookSettings"), TEXT("CookProcessCount"), CookProcessCount, GEditorIni);
		}
		CookProcessCount = FMath::Max(1, CookProcessCount);
		bUnrealMultiprocessCooking = CookProcessCount > 1;
	}
}

void FWwiseResourceCookerImpl::CookAuxBusToSandbox(const FWwiseAuxBusCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking AuxBus %s %" PRIu32), *InCookedData.DebugName.ToString(),
		(uint32)InCookedData.AuxBusId);
	for (const auto& SoundBank : InCookedData.SoundBanks)
	{
		CookSoundBankToSandbox(SoundBank, PackageFilename, WriteAdditionalFile);
	}
	for (const auto& Media : InCookedData.Media)
	{
		CookMediaToSandbox(Media, PackageFilename, WriteAdditionalFile);
	}
	UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("Done cooking AuxBus %s %" PRIu32),
		*InCookedData.DebugName.ToString(), (uint32)InCookedData.AuxBusId);
}

void FWwiseResourceCookerImpl::CookEventToSandbox(const FWwiseEventCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking Event %s %" PRIu32), *InCookedData.DebugName.ToString(),
		(uint32)InCookedData.EventId);
	for (const auto& SoundBank : InCookedData.SoundBanks)
	{
		CookSoundBankToSandbox(SoundBank, PackageFilename, WriteAdditionalFile);
	}
	for (const auto& Media : InCookedData.Media)
	{
		CookMediaToSandbox(Media, PackageFilename, WriteAdditionalFile);
	}
	for (const auto& ExternalSource : InCookedData.ExternalSources)
	{
		CookExternalSourceToSandbox(ExternalSource, PackageFilename, WriteAdditionalFile);
	}
	for (const auto& SwitchContainerLeaf : InCookedData.SwitchContainerLeaves)
	{
		UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking Event %s %" PRIu32 " Switched Media"),
			*InCookedData.DebugName.ToString(), (uint32)InCookedData.EventId);
		for (const auto& SoundBank : SwitchContainerLeaf.SoundBanks)
		{
			CookSoundBankToSandbox(SoundBank, PackageFilename, WriteAdditionalFile);
		}
		for (const auto& Media : SwitchContainerLeaf.Media)
		{
			CookMediaToSandbox(Media, PackageFilename, WriteAdditionalFile);
		}
		for (const auto& ExternalSource : SwitchContainerLeaf.ExternalSources)
		{
			CookExternalSourceToSandbox(ExternalSource, PackageFilename, WriteAdditionalFile);
		}
	}
	UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("Done cooking Event %s %" PRIu32),
		*InCookedData.DebugName.ToString(), (uint32)InCookedData.EventId);
}

void FWwiseResourceCookerImpl::CookExternalSourceToSandbox(const FWwiseExternalSourceCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (LIKELY(CookingCache && CookingCache->ExternalSourceManager))
	{
		CookingCache->ExternalSourceManager->Cook(*this, InCookedData, PackageFilename, WriteAdditionalFile,
			GetCurrentPlatform(), GetCurrentLanguage());
	}
	else
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("No External Source Manager while cooking External Source %s %" PRIu32),
			*InCookedData.DebugName.ToString(), (uint32)InCookedData.Cookie);
	}
}

void FWwiseResourceCookerImpl::CookInitBankToSandbox(const FWwiseInitBankCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking Init SoundBank %s %" PRIu32),
		*InCookedData.DebugName.ToString(), (uint32)InCookedData.SoundBankId);
	CookSoundBankToSandbox(InCookedData, PackageFilename, WriteAdditionalFile);

	for (const auto& SoundBank : InCookedData.SoundBanks)
	{
		CookSoundBankToSandbox(SoundBank, PackageFilename, WriteAdditionalFile);
	}

	for (const auto& Media : InCookedData.Media)
	{
		CookMediaToSandbox(Media, PackageFilename, WriteAdditionalFile);
	}
	UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("Done cooking Init SoundBank %s %" PRIu32),
		*InCookedData.DebugName.ToString(), (uint32)InCookedData.SoundBankId);
}

void FWwiseResourceCookerImpl::CookMediaToSandbox(const FWwiseMediaCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (InCookedData.PackagedFile.PackagingStrategy != EWwisePackagingStrategy::AdditionalFile &&
			InCookedData.PackagedFile.PackagingStrategy != EWwisePackagingStrategy::HybridAdditionalFile)
	{
		return;
	}

	if (UNLIKELY(InCookedData.PackagedFile.PathName.IsNone()))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("Empty pathname for Media %s %" PRIu32),
			*InCookedData.DebugName.ToString(), (uint32)InCookedData.MediaId);
		return;
	}

	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking Media %s %" PRIu32), *InCookedData.DebugName.ToString(),
		(uint32)InCookedData.MediaId);

	UE_CLOG(UNLIKELY(bUnrealMultiprocessCooking), LogWwiseResourceCooker, Warning,
		TEXT("FWwiseResourceCookerImpl::CookMediaToSandbox %s: "
		"Trying to package an additional file with Multiprocess cooking enabled. Enable Bulk Data asset cooking and create Wwise Asset Libraries for extraneous files."),
		*InCookedData.GetDebugString());

	CookFileToSandbox(InCookedData.PackagedFile, PackageFilename, WriteAdditionalFile);
}

void FWwiseResourceCookerImpl::CookShareSetToSandbox(const FWwiseShareSetCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking ShareSet %s %" PRIu32), *InCookedData.DebugName.ToString(),
		(uint32)InCookedData.ShareSetId);
	for (const auto& SoundBank : InCookedData.SoundBanks)
	{
		CookSoundBankToSandbox(SoundBank, PackageFilename, WriteAdditionalFile);
	}
	for (const auto& Media : InCookedData.Media)
	{
		CookMediaToSandbox(Media, PackageFilename, WriteAdditionalFile);
	}
	UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("Done cooking ShareSet %s %" PRIu32),
		*InCookedData.DebugName.ToString(), (uint32)InCookedData.ShareSetId);
}

void FWwiseResourceCookerImpl::CookSoundBankToSandbox(const FWwiseSoundBankCookedData& InCookedData,
	const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	if (InCookedData.PackagedFile.PackagingStrategy != EWwisePackagingStrategy::AdditionalFile &&
			InCookedData.PackagedFile.PackagingStrategy != EWwisePackagingStrategy::HybridAdditionalFile)
	{
		return;
	}

	if (UNLIKELY(InCookedData.PackagedFile.PathName.IsNone()))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("Empty pathname for SoundBank %s %" PRIu32),
			*InCookedData.DebugName.ToString(), (uint32)InCookedData.SoundBankId);
		return;
	}

	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("Cooking SoundBank %s %" PRIu32), *InCookedData.DebugName.ToString(),
		(uint32)InCookedData.SoundBankId);
	
	UE_CLOG(UNLIKELY(bUnrealMultiprocessCooking), LogWwiseResourceCooker, Warning,
		TEXT("FWwiseResourceCookerImpl::CookSoundBankToSandbox %s: "
		"Trying to package an additional file with Multiprocess cooking enabled. Enable Bulk Data asset cooking and create Wwise Asset Libraries for extraneous files."),
		*InCookedData.GetDebugString());

	CookFileToSandbox(InCookedData.PackagedFile, PackageFilename, WriteAdditionalFile);
}

void FWwiseResourceCookerImpl::CookFileToSandbox(const FWwisePackagedFile& PackagedFile, const TCHAR* PackageFilename,
	const WriteAdditionalFileFunction& WriteAdditionalFile)
{
	const auto& InputPathName{PackagedFile.SourcePathName};
	const auto& OutputPathFName{PackagedFile.PathName};
	const auto OutputPathName{OutputPathFName.ToString()};

	auto& StageFiles = CookingCache->StagedFiles;

	if (const auto* AlreadyStaged = StageFiles.Find(OutputPathFName))
	{
		if (*AlreadyStaged == InputPathName)
		{
			UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("Cook: Skipping already present file %s -> %s"),
				*InputPathName, *OutputPathName);
		}
		else
		{
			UE_LOG(LogWwiseResourceCooker, Error,
				TEXT("Cook: Trying to stage two different files to the same path: [%s and %s] -> %s"),
				*InputPathName, **AlreadyStaged, *OutputPathName);
		}

		return;
	}
	StageFiles.Add(OutputPathFName, InputPathName);

	const auto CookRootPath = GetCookRootPath(PackageFilename);
	PackagedFile.CookToSandbox(CookRootPath, WriteAdditionalFile);
}

bool FWwiseResourceCookerImpl::GetAcousticTextureCookedData(FWwiseAcousticTextureCookedData& OutCookedData,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.ShortId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetAcousticTextureCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetAcousticTextureCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefAcousticTexture AcousticTextureRef;

	if (UNLIKELY(!PlatformData->GetRef(AcousticTextureRef, WwiseDBSharedLanguageId(), ConvertWwiseObjectInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetAcousticTextureCookedData (%s %" PRIu32 " %s): No acoustic texture data found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const auto* AcousticTexture = AcousticTextureRef.GetAcousticTexture();

	OutCookedData.AbsorptionLow = AcousticTexture->AbsorptionLow;
	OutCookedData.AbsorptionMidLow = AcousticTexture->AbsorptionMidLow;
	OutCookedData.AbsorptionMidHigh = AcousticTexture->AbsorptionMidHigh;
	OutCookedData.AbsorptionHigh = AcousticTexture->AbsorptionHigh;
	OutCookedData.ShortId = AcousticTexture->Id;
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutCookedData.DebugName = FName();
	}
	else
	{
		OutCookedData.DebugName = FName(*((ExportDebugNameRule == EWwiseExportDebugNameRule::Name)
			? AcousticTexture->Name
			: AcousticTexture->ObjectPath));
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetAudioDeviceShareSetCookedData(FWwiseAudioDeviceShareSetCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const
{
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetAudioDeviceShareSetCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetAudioDeviceShareSetCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefAudioDevice AudioDeviceRef;

	if (UNLIKELY(!PlatformData->GetRef(AudioDeviceRef, WwiseDBSharedLanguageId(), ConvertWwiseObjectInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetAudioDeviceShareSetCookedData (%s %" PRIu32 " %s): No Audio Device ShareSet data found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.ShortId = AudioDeviceRef.AudioDeviceId();
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutCookedData.DebugName = FName();
	}
	else
	{
		OutCookedData.DebugName = FName((ExportDebugNameRule == EWwiseExportDebugNameRule::Name) ? **AudioDeviceRef.AudioDeviceName() : **AudioDeviceRef.AudioDeviceObjectPath());
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetAuxBusCookedData(FWwiseLocalizedAuxBusCookedData& OutCookedData, const TCHAR* ContentFolderName,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.AuxBusId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const auto* PlatformInfo = PlatformData->PlatformRef.GetPlatformInfo();
	if (UNLIKELY(!PlatformInfo))
	{
		return false;
	}

	const WwiseDBSet<WwiseDBSharedLanguageId>& Languages = DataStructure.GetLanguages();

	WwiseDBMap<WwiseDBSharedLanguageId, WwiseDBSet<WwiseRefAuxBus>> RefLanguageMap;
	PlatformData->GetRefMap(RefLanguageMap, Languages, ConvertWwiseObjectInfo(InInfo));
	if (UNLIKELY(RefLanguageMap.Size() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): No ref found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.AuxBusLanguageMap.Empty(RefLanguageMap.Size());

	for (auto& Ref : RefLanguageMap)
	{
		WwiseDBPair<const WwiseDBSharedLanguageId, WwiseDBSet<WwiseRefAuxBus>> RefPair(Ref);
		FWwiseAuxBusCookedData CookedData;

		WwiseDBSet<FWwiseSoundBankCookedData> SoundBankSet;
		WwiseDBSet<FWwiseMediaCookedData> MediaSet;
		WwiseDBSet<WwiseRefAuxBus> AuxBusses = RefPair.GetSecond();

		if (UNLIKELY(AuxBusses.Size() == 0))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): Empty ref for language"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}

		// Set up basic global Aux Bus information
		{
			auto FirstAuxBus(AuxBusses.GetFirst());
			CookedData.AuxBusId = FirstAuxBus.AuxBusId();
			if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
			{
				OutCookedData.DebugName = FName();
			}
			else
			{
				CookedData.DebugName = FName(*((ExportDebugNameRule == EWwiseExportDebugNameRule::Name)
					? *FirstAuxBus.AuxBusName()
					: *FirstAuxBus.AuxBusObjectPath()));
				OutCookedData.DebugName = CookedData.DebugName;
			}
			OutCookedData.AuxBusId = CookedData.AuxBusId;
		}

		for (auto& AuxBusRef : AuxBusses)
		{
			const auto* AuxBus = AuxBusRef.GetAuxBus();
			if (UNLIKELY(!AuxBus))
			{
				UE_LOG(LogWwiseResourceCooker, Warning,
					TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): Could not get AuxBus from Ref"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				continue;
			}

			WwiseDBSet<const WwiseRefAuxBus*> SubAuxBusRefs;
			AuxBusRef.GetAllAuxBusRefs(SubAuxBusRefs, PlatformData->AuxBusses);
			for (const auto* SubAuxBusRef : SubAuxBusRefs)
			{
				const auto* SoundBank = SubAuxBusRef->GetSoundBank();
				if (UNLIKELY(!SoundBank))
				{
					UE_LOG(LogWwiseResourceCooker, Warning,
						TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): Could not get SoundBank from Ref"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					continue;
				}

				if (!SoundBank->IsInitBank())
				{
					FWwiseSoundBankCookedData SoundBankCookedData;
					if (UNLIKELY(!FillSoundBankBaseInfo(SoundBankCookedData, *PlatformInfo, *SoundBank, ContentFolderName)))
					{
						UE_LOG(LogWwiseResourceCooker, Warning,
							TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): Could not fill SoundBank from Data"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						continue;
					}
					SoundBankSet.Add(SoundBankCookedData);
				}

				{
					WwiseCustomPluginIdsMap CustomPluginsRefs = SubAuxBusRef->GetAuxBusCustomPlugins(
						PlatformData->CustomPlugins);
					for (const auto& Plugin : CustomPluginsRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
						const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(
								!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(),
									WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				{
					WwisePluginShareSetIdsMap ShareSetRefs = SubAuxBusRef->GetAuxBusPluginShareSets(
						PlatformData->PluginShareSets);
					for (const auto& ShareSet : ShareSetRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
						const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(
								!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(),
								WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				{
					WwiseAudioDeviceIdsMap AudioDevicesRefs = SubAuxBusRef->GetAuxBusAudioDevices(
						PlatformData->AudioDevices);
					for (const auto& AudioDevice : AudioDevicesRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
						const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(
								!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(),
								WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}
			}
		}
		SoundBankSet.AppendTo(CookedData.SoundBanks);
		MediaSet.AppendTo(CookedData.Media);
		OutCookedData.AuxBusLanguageMap.Add(
			FWwiseLanguageCookedData((int32)RefPair.GetFirst().GetLanguageId(), FName(*RefPair.GetFirst().GetLanguageName()), (EWwiseLanguageRequirement)RefPair.GetFirst().LanguageRequirement),
			MoveTemp(CookedData));
	}

	if (UNLIKELY(OutCookedData.AuxBusLanguageMap.Num() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): No AuxBus"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	// Make this a SFX if all CookedData are identical
	{
		auto& Map = OutCookedData.AuxBusLanguageMap;
		TArray<FWwiseLanguageCookedData> Keys;
		Map.GetKeys(Keys);

		auto LhsKey = Keys.Pop(EWwiseAllowShrinking::No);
		const auto* Lhs = Map.Find(LhsKey);
		while (Keys.Num() > 0)
		{
			auto RhsKey = Keys.Pop(EWwiseAllowShrinking::No);
			const auto* Rhs = Map.Find(RhsKey);

			if (Lhs->AuxBusId != Rhs->AuxBusId
				|| Lhs->DebugName != Rhs->DebugName
				|| Lhs->SoundBanks.Num() != Rhs->SoundBanks.Num()
				|| Lhs->Media.Num() != Rhs->Media.Num())
			{
				UE_LOG(LogWwiseResourceCooker, VeryVerbose,
					TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): AuxBus has languages"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return true;
			}
			for (const auto& Elem : Lhs->SoundBanks)
			{
				if (!Rhs->SoundBanks.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): AuxBus has languages due to banks"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
			for (const auto& Elem : Lhs->Media)
			{
				if (!Rhs->Media.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): AuxBus has languages due to media"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
		}

		UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): AuxBus is a SFX"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		std::remove_reference_t<decltype(Map)> SfxMap;
		SfxMap.Add(FWwiseLanguageCookedData::Sfx, *Lhs);

		Map = SfxMap;
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetEventCookedData(FWwiseLocalizedEventCookedData& OutCookedData, const TCHAR* ContentFolderName,
	const FWwiseEventInfo& InInfo) const
{
	OutCookedData.EventId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetEventCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetEventCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDBSet<WwiseDBSharedLanguageId>& Languages = DataStructure.GetLanguages();

	const auto* PlatformInfo = PlatformData->PlatformRef.GetPlatformInfo();
	if (UNLIKELY(!PlatformInfo))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetEventCookedData (%s %" PRIu32 " %s): No Platform Info"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseDBMap<WwiseDBSharedLanguageId, WwiseDBSet<WwiseRefEvent>> RefLanguageMap;
	PlatformData->GetRefMap(RefLanguageMap, Languages, ConvertWwiseObjectInfo(InInfo));
	if (UNLIKELY(RefLanguageMap.Size() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetEventCookedData (%s %" PRIu32 " %s): No ref found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.EventLanguageMap.Empty(RefLanguageMap.Size());
	UE_LOG(LogWwiseResourceCooker, Verbose, TEXT("GetEventCookedData (%s %" PRIu32 " %s): Adding %llu languages to map"),
		*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString(), RefLanguageMap.Size());

	for (auto& Ref : RefLanguageMap)
	{
		WwiseDBPair<const WwiseDBSharedLanguageId, WwiseDBSet<WwiseRefEvent>> RefPair(Ref);
		FWwiseEventCookedData CookedData;

		WwiseDBSet<FWwiseSoundBankCookedData> SoundBankSet;
		WwiseDBSet<FWwiseMediaCookedData> MediaSet;

		const WwiseDBSharedLanguageId& LanguageId = RefPair.GetFirst();
		WwiseDBSet<WwiseRefEvent> Events = RefPair.GetSecond();
		WwiseSwitchContainerArray SwitchContainerRefs;

		if (UNLIKELY(Events.Size() == 0))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetEventCookedData (%s %" PRIu32 " %s): Empty ref for language"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}

		// Set up basic global Event information
		{
			auto FirstEvent(Events.GetFirst());
			CookedData.EventId = FirstEvent.EventId();
			if (ExportDebugNameRule != EWwiseExportDebugNameRule::Release)
			{
				CookedData.DebugName = FName(*((ExportDebugNameRule == EWwiseExportDebugNameRule::Name)
					? *FirstEvent.EventName()
					: *FirstEvent.EventObjectPath()));
				OutCookedData.DebugName = CookedData.DebugName;
			}

			OutCookedData.EventId = CookedData.EventId;
			SwitchContainerRefs = FirstEvent.GetSwitchContainers(PlatformData->SwitchContainersByEvent);
		}

		// Add extra events recursively
		{
			WwiseDBSet<WwiseRefEvent> DiffEvents = Events;
			while (true)
			{
				bool bHaveMore = false;
				WwiseDBSet<WwiseRefEvent> OldEvents(Events);
				for (auto& EventRef : OldEvents)
				{
					const WwiseMetadataEvent* Event = EventRef.GetEvent();
					if (UNLIKELY(!Event))
					{
						UE_LOG(LogWwiseResourceCooker, Warning,
							TEXT("GetEventCookedData (%s %" PRIu32 " %s): Could not get Event from Ref"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return false;
					}

					for (const auto& ActionPostEvent : Event->ActionPostEvent)
					{
						bool bHaveMoreInThisEvent = PlatformData->GetRef(
							Events, LanguageId,
							ConvertWwiseObjectInfo(FWwiseEventInfo(ActionPostEvent.Id, FName(*ActionPostEvent.Name))));
						bHaveMore = bHaveMore || bHaveMoreInThisEvent;
					}
				}
				if (bHaveMore)
				{
					DiffEvents = Events.Difference(OldEvents);
					if (DiffEvents.Size() == 0)
					{
						UE_LOG(LogWwiseResourceCooker, Warning,
							TEXT("GetEventCookedData (%s %" PRIu32
								" %s): GetRef should return false when no more additional Refs"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						break;
					}
					for (auto& EventRef : DiffEvents)
					{
						SwitchContainerRefs.Append(EventRef.GetSwitchContainers(PlatformData->SwitchContainersByEvent));
					}
				}
				else
				{
					break;
				}
			}
		}

		// Add mandatory SoundBank information
		WwiseDBSet<FWwiseExternalSourceCookedData> ExternalSourceSet;
		WwiseDBSet<WwiseAnyRef> RequiredGroupValueSet;
		for (auto& EventRef : Events)
		{
			const WwiseMetadataEvent* Event = EventRef.GetEvent();
			if (UNLIKELY(!Event))
			{
				UE_LOG(LogWwiseResourceCooker, Warning,
					TEXT("GetEventCookedData (%s %" PRIu32 " %s): Could not get Event from Ref"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return false;
			}

			if (LIKELY(Event->IsMandatory()) || LIKELY(Events.Size() == 1))
			{
				// Add main SoundBank
				{
					const auto* SoundBank = EventRef.GetSoundBank();
					if (UNLIKELY(!SoundBank))
					{
						UE_LOG(LogWwiseResourceCooker, Warning,
							TEXT("GetEventCookedData (%s %" PRIu32 " %s): Could not get SoundBank from Ref"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return false;
					}
					if (!SoundBank->IsInitBank())
					{
						FWwiseSoundBankCookedData MainSoundBank;
						if (UNLIKELY(!FillSoundBankBaseInfo(MainSoundBank, *PlatformInfo, *SoundBank, ContentFolderName)))
						{
							UE_LOG(LogWwiseResourceCooker, Warning,
								TEXT("GetEventCookedData (%s %" PRIu32 " %s): Could not fill SoundBank from Data"),
								*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
							return false;
						}
						SoundBankSet.Add(MainSoundBank);
					}
				}

				// Get Aux Bus banks & media
				{
					WwiseAuxBusIdsMap AuxBusRefs = EventRef.GetEventAuxBusses(PlatformData->AuxBusses);
					WwiseDBSet<const WwiseRefAuxBus*> SubAuxBusRefs;
					for (const auto& AuxBusRef : AuxBusRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefAuxBus> AuxBusRefPair(AuxBusRef);
						AuxBusRefs.FindChecked(AuxBusRefPair.GetFirst()).GetAllAuxBusRefs(SubAuxBusRefs, PlatformData->AuxBusses);
					}
					for (const auto* SubAuxBusRef : SubAuxBusRefs)
					{
						const auto* SoundBank = SubAuxBusRef->GetSoundBank();
						if (UNLIKELY(!SoundBank))
						{
							UE_LOG(LogWwiseResourceCooker, Warning,
								TEXT("GetEventCookedData (%s %" PRIu32 " %s): Could not get SoundBank from Ref"),
								*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
							return false;
						}

						if (!SoundBank->IsInitBank())
						{
							FWwiseSoundBankCookedData SoundBankCookedData;
							if (UNLIKELY(!FillSoundBankBaseInfo(SoundBankCookedData, *PlatformInfo, *SoundBank, ContentFolderName)))
							{
								UE_LOG(LogWwiseResourceCooker, Warning,
									TEXT("GetEventCookedData (%s %" PRIu32 " %s): Could not fill SoundBank from Data"
									),
									*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
								return false;
							}
							SoundBankSet.Add(SoundBankCookedData);
						}

						{
							WwiseCustomPluginIdsMap CustomPluginsRefs = SubAuxBusRef->GetAuxBusCustomPlugins(
								PlatformData->CustomPlugins);
							for (const auto& Plugin : CustomPluginsRefs)
							{
								WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
								const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
								for (const auto& MediaRef : MediaRefs)
								{
									WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
									if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
									{
										return false;
									}
								}
							}
						}

						{
							WwisePluginShareSetIdsMap ShareSetRefs = SubAuxBusRef->GetAuxBusPluginShareSets(
								PlatformData->PluginShareSets);
							for (const auto& ShareSet : ShareSetRefs)
							{
								WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
								const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
								for (const auto& MediaRef : MediaRefs)
								{
									WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
									if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
									{
										return false;
									}
								}
							}
						}

						{
							WwiseAudioDeviceIdsMap AudioDevicesRefs = SubAuxBusRef->GetAuxBusAudioDevices(
								PlatformData->AudioDevices);
							for (const auto& AudioDevice : AudioDevicesRefs)
							{
								WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
								const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
								for (const auto& MediaRef : MediaRefs)
								{
									WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
									if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
									{
										return false;
									}
								}
							}
						}
					}
				}

				// Get media
				{
					WwiseMediaIdsMap MediaRefs = EventRef.GetEventMedia(PlatformData->MediaFiles);
					for (const auto& MediaRef : MediaRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
						if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
						{
							return false;
						}
					}
				}

				// Get Media from custom plugins
				{
					WwiseCustomPluginIdsMap CustomPluginsRefs = EventRef.GetEventCustomPlugins(
						PlatformData->CustomPlugins);
					for (const auto& Plugin : CustomPluginsRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
						const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				// Get Media from plugin ShareSets
				{
					WwisePluginShareSetIdsMap ShareSetRefs = EventRef.GetEventPluginShareSets(PlatformData->PluginShareSets);
					for (const auto& ShareSet : ShareSetRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
						const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				// Get Media from audio devices
				{
					WwiseAudioDeviceIdsMap AudioDevicesRefs = EventRef.GetEventAudioDevices(PlatformData->AudioDevices);
					for (const auto& AudioDevice : AudioDevicesRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
						const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				// Get External Sources
				{
					WwiseExternalSourceIdsMap ExternalSourceRefs = EventRef.GetEventExternalSources(
						PlatformData->ExternalSources);
					for (const auto& ExternalSourceRef : ExternalSourceRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefExternalSource> ExternalSourceRefPair(ExternalSourceRef);
						if (UNLIKELY(!AddRequirementsForExternalSource(ExternalSourceSet, ExternalSourceRefPair.GetSecond())))
						{
							return false;
						}
					}
				}

				// Get required GroupValues
				{
					for (const auto& Switch : EventRef.GetActionSetSwitch(PlatformData->Switches))
					{
						WwiseDBPair<const WwiseDatabaseGroupValueKey, WwiseRefSwitch> SwitchPair(Switch);
						if (LIKELY(SwitchPair.GetSecond().IsValid()))
						{
							RequiredGroupValueSet.Add(WwiseAnyRef::Create(SwitchPair.GetSecond()));
						}
					}

					for (const auto& State : EventRef.GetActionSetState(PlatformData->States))
					{
						WwiseDBPair<const WwiseDatabaseGroupValueKey, WwiseRefState> StatePair(State);
						if (LIKELY(StatePair.GetSecond().IsValid()))
						{
							RequiredGroupValueSet.Add(WwiseAnyRef::Create(StatePair.GetSecond()));
						}
					}
				}
			}
		}

		// Get Switched Media, negating required switches.
		{
			WwiseDBMap<WwiseRefSwitchContainer, WwiseDBSet<WwiseAnyRef>> SwitchValuesMap;

			for (const auto& SwitchContainerRef : SwitchContainerRefs)
			{
				const auto* SwitchContainer = SwitchContainerRef.GetSwitchContainer();
				if (UNLIKELY(!SwitchContainer))
				{
					return false;
				}

				auto SwitchValues = WwiseDBSet<WwiseAnyRef>(SwitchContainerRef.GetSwitchValues(PlatformData->Switches, PlatformData->States));

				WwiseDBSet<WwiseAnyRef> SwitchesToRemove;
				for (const auto& SwitchValue : SwitchValues)
				{
					// Remove all SwitchValues if we load them all by default
					if (InInfo.SwitchContainerLoading == EWwiseEventSwitchContainerLoading::AlwaysLoad)
					{
						UE_LOG(LogWwiseResourceCooker, VeryVerbose,
							TEXT("GetEventCookedData (%s %" PRIu32 " %s): Skip value %s (%" PRIu32 ":%" PRIu32
								"): Event Switch Container set to AlwaysLoad"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString(),
							*FWwiseStringConverter::ToFString(SwitchValue.GetName()), SwitchValue.GetGroupId(), SwitchValue.GetId());

						SwitchesToRemove.Add(SwitchValue);
						continue;
					}

					// Remove SwitchValues that are already present in RequiredGroupValueSet
					if (RequiredGroupValueSet.Contains(SwitchValue))
					{
						UE_LOG(LogWwiseResourceCooker, VeryVerbose,
							TEXT("GetEventCookedData (%s %" PRIu32 " %s): Skip value %s (%" PRIu32 ":%" PRIu32
								"): Already in the required set"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString(),
							*FWwiseStringConverter::ToFString(SwitchValue.GetName()), SwitchValue.GetGroupId(), SwitchValue.GetId());

						SwitchesToRemove.Add(SwitchValue);
						continue;
					}

					// Remove SwitchValues that have an ID of "0" (wildcard in music)
					if (SwitchValue.GetId() == 0)
					{
						UE_LOG(LogWwiseResourceCooker, VeryVerbose,
							TEXT("GetEventCookedData (%s %" PRIu32 " %s): Skip value %s (%" PRIu32 ":%" PRIu32
								"): Wildcard ID"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString(),
							*FWwiseStringConverter::ToFString(SwitchValue.GetName()), SwitchValue.GetGroupId(), SwitchValue.GetId());

						SwitchesToRemove.Add(SwitchValue);
						continue;
					}

					// Remove Switch groups that are controlled by a Game Parameter (RTPC)
					if (SwitchValue.GetType() == WwiseRefType::Switch)
					{
						const auto* SwitchRef = SwitchValue.GetSwitchRef();
						check(SwitchRef);

						if (SwitchRef->IsControlledByGameParameter())
						{
							UE_LOG(LogWwiseResourceCooker, VeryVerbose,
								TEXT("GetEventCookedData (%s %" PRIu32 " %s): Skip value %s (%" PRIu32 ":%" PRIu32
									"): Controlled by Game Parameter"),
								*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString(),
								*FWwiseStringConverter::ToFString(SwitchValue.GetName()), SwitchValue.GetGroupId(), SwitchValue.GetId());

							SwitchesToRemove.Add(SwitchValue);
							continue;
						}
					}
				}
				SwitchValues = SwitchValues.Difference(SwitchesToRemove);

				// Add default information if it was not already added, in case all the Switch Values are compulsory.
				if (SwitchValues.Size() == 0)
				{
					// Media and SoundBank are compulsory. Add them so they are always loaded.
					const auto* SoundBank = SwitchContainerRef.GetSoundBank();
					if (UNLIKELY(!SoundBank))
					{
						UE_LOG(LogWwiseResourceCooker, Warning,
							TEXT("GetEventCookedData (%s %" PRIu32
								" %s): Could not get SoundBank from Switch Container Ref"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return false;
					}
					if (!SoundBank->IsInitBank())
					{
						FWwiseSoundBankCookedData SwitchContainerSoundBank;
						if (UNLIKELY(!FillSoundBankBaseInfo(SwitchContainerSoundBank, *PlatformInfo, *SoundBank, ContentFolderName)))
						{
							UE_LOG(LogWwiseResourceCooker, Warning,
								TEXT("GetEventCookedData (%s %" PRIu32
									" %s): Could not fill SoundBank from Switch Container Data"),
								*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
							return false;
						}
						SoundBankSet.Add(SwitchContainerSoundBank);
					}

					{
						WwiseDBArray<WwiseRefMedia> MediaToAdd;
						SwitchContainerRef.GetSwitchContainerMedia(PlatformData->MediaFiles).GenerateValueArray(
							MediaToAdd);
						for (const auto& MediaRef : MediaToAdd)
						{
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRef, RefPair.GetFirst(), *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}

					{
						WwiseCustomPluginIdsMap CustomPluginsRefs = SwitchContainerRef.GetSwitchContainerCustomPlugins(
							PlatformData->CustomPlugins);
						for (const auto& Plugin : CustomPluginsRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
							const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
							for (const auto& MediaRef : MediaRefs)
							{
								WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
								if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
								{
									return false;
								}
							}
						}
					}

					{
						WwisePluginShareSetIdsMap ShareSetRefs = SwitchContainerRef.GetSwitchContainerPluginShareSets(
							PlatformData->PluginShareSets);
						for (const auto& ShareSet : ShareSetRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
							const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
							for (const auto& MediaRef : MediaRefs)
							{
								WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
								if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
								{
									return false;
								}
							}
						}
					}

					{
						WwiseAudioDeviceIdsMap AudioDevicesRefs = SwitchContainerRef.GetSwitchContainerAudioDevices(
							PlatformData->AudioDevices);
						for (const auto& AudioDevice : AudioDevicesRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
							const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
							for (const auto& MediaRef : MediaRefs)
							{
								WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
								if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
								{
									return false;
								}
							}
						}
					}

					WwiseDBArray<WwiseRefExternalSource> ExternalSourcesToAdd;
					SwitchContainerRef.GetSwitchContainerExternalSources(PlatformData->ExternalSources).
						GenerateValueArray(ExternalSourcesToAdd);

					for (const auto& ExternalSourceRef : ExternalSourcesToAdd)
					{
						if (UNLIKELY(!AddRequirementsForExternalSource(ExternalSourceSet, ExternalSourceRef)))
						{
							return false;
						}
					}
				}
				else
				{
					// Media is optional. Will process later
					SwitchValuesMap.Add(SwitchContainerRef, SwitchValues);
				}
			}

			// Process Switch Containers that seemingly contain additional media and conditions
			for (const auto& SwitchContainerRef : SwitchContainerRefs)
			{
				const auto* SwitchValues = SwitchValuesMap.Find(SwitchContainerRef);
				if (!SwitchValues)
				{
					continue;
				}
				check(SwitchValues->Size() > 0);

				// Prepare media and main SoundBank to add
				WwiseDBSet<FWwiseSoundBankCookedData> SoundBankSetToAdd;
				WwiseDBSet<FWwiseMediaCookedData> MediaSetToAdd;
				WwiseDBSet<FWwiseExternalSourceCookedData> ExternalSourceSetToAdd;

				const auto* SoundBank = SwitchContainerRef.GetSoundBank();
				if (UNLIKELY(!SoundBank))
				{
					UE_LOG(LogWwiseResourceCooker, Warning,
						TEXT("GetEventCookedData (%s %" PRIu32
							" %s): Could not get SoundBank from Switch Container Ref"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return false;
				}
				if (!SoundBank->IsInitBank())
				{
					FWwiseSoundBankCookedData SwitchContainerSoundBank;
					if (UNLIKELY(!FillSoundBankBaseInfo(SwitchContainerSoundBank, *PlatformInfo, *SoundBank, ContentFolderName)))
					{
						UE_LOG(LogWwiseResourceCooker, Warning,
							TEXT("GetEventCookedData (%s %" PRIu32
								" %s): Could not fill SoundBank from Switch Container Data"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return false;
					}
					SoundBankSetToAdd.Add(SwitchContainerSoundBank);
				}

				{
					WwiseDBArray<WwiseRefMedia> MediaToAdd;
					SwitchContainerRef.GetSwitchContainerMedia(PlatformData->MediaFiles).GenerateValueArray(MediaToAdd);
					for (const auto& MediaRef : MediaToAdd)
					{
						if (UNLIKELY(!AddRequirementsForMedia(SoundBankSetToAdd, MediaSetToAdd, MediaRef, RefPair.GetFirst(), *PlatformData, ContentFolderName)))
						{
							return false;
						}
					}
				}

				{
					WwiseCustomPluginIdsMap CustomPluginsRefs = SwitchContainerRef.GetSwitchContainerCustomPlugins(
						PlatformData->CustomPlugins);
					for (const auto& Plugin : CustomPluginsRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
						const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSetToAdd, MediaSetToAdd, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				{
					WwisePluginShareSetIdsMap ShareSetRefs = SwitchContainerRef.GetSwitchContainerPluginShareSets(
						PlatformData->PluginShareSets);
					for (const auto& ShareSet : ShareSetRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
						const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSetToAdd, MediaSetToAdd, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}

				{
					WwiseAudioDeviceIdsMap AudioDevicesRefs = SwitchContainerRef.GetSwitchContainerAudioDevices(
						PlatformData->AudioDevices);
					for (const auto& AudioDevice : AudioDevicesRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
						const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
						for (const auto& MediaRef : MediaRefs)
						{
							WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
							if (UNLIKELY(!AddRequirementsForMedia(SoundBankSetToAdd, MediaSetToAdd, MediaRefPair.GetSecond(), LanguageId, *PlatformData, ContentFolderName)))
							{
								return false;
							}
						}
					}
				}


				WwiseDBArray<WwiseRefExternalSource> ExternalSourcesToAdd;
				SwitchContainerRef.GetSwitchContainerExternalSources(PlatformData->ExternalSources).GenerateValueArray(
					ExternalSourcesToAdd);

				for (const auto& ExternalSourceRef : ExternalSourcesToAdd)
				{
					if (UNLIKELY(!AddRequirementsForExternalSource(ExternalSourceSetToAdd, ExternalSourceRef)))
					{
						return false;
					}
				}

				SoundBankSetToAdd = SoundBankSetToAdd.Difference(SoundBankSet);
				MediaSetToAdd = MediaSetToAdd.Difference(MediaSet);
				ExternalSourceSetToAdd = ExternalSourceSetToAdd.Difference(ExternalSourceSet);

				// Have we already included all the external banks and media
				if (SoundBankSetToAdd.Size() == 0 && MediaSetToAdd.Size() == 0 && ExternalSourceSetToAdd.Size() == 0)
				{
					continue;
				}

				// Fill up SwitchContainerCookedData and add it to SwitchContainerLeaves
				FWwiseSwitchContainerLeafCookedData SwitchContainerCookedData;
				for (const auto& SwitchValue : *SwitchValues)
				{
					FWwiseGroupValueCookedData SwitchCookedData;
					switch (SwitchValue.GetType())
					{
					case WwiseRefType::Switch: SwitchCookedData.Type = EWwiseGroupType::Switch; break;
					case WwiseRefType::State: SwitchCookedData.Type = EWwiseGroupType::State; break;
					default: SwitchCookedData.Type = EWwiseGroupType::Unknown;
					}
					SwitchCookedData.GroupId = SwitchValue.GetGroupId();
					SwitchCookedData.Id = SwitchValue.GetId();
					if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
					{
						SwitchCookedData.DebugName = FName();
					}
					else
					{
						SwitchCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? SwitchValue.GetName() : SwitchValue.GetObjectPath()));
					}
					SwitchContainerCookedData.GroupValueSet.Add(MoveTemp(SwitchCookedData));
				}
				if (auto* ExistingSwitchedMedia = CookedData.SwitchContainerLeaves.FindByPredicate(
					[&SwitchContainerCookedData](const FWwiseSwitchContainerLeafCookedData& RhsValue)
					{
						return RhsValue.GroupValueSet.Difference(SwitchContainerCookedData.GroupValueSet).Num() == 0
							&& SwitchContainerCookedData.GroupValueSet.Difference(RhsValue.GroupValueSet).Num() == 0;
					}))
				{
					SoundBankSetToAdd.AppendTo(ExistingSwitchedMedia->SoundBanks);
					MediaSetToAdd.AppendTo(ExistingSwitchedMedia->Media);
					ExternalSourceSetToAdd.AppendTo(ExistingSwitchedMedia->ExternalSources);
				}
				else
				{
					SoundBankSetToAdd.AppendTo(SwitchContainerCookedData.SoundBanks);
					MediaSetToAdd.AppendTo(SwitchContainerCookedData.Media);
					ExternalSourceSetToAdd.AppendTo(SwitchContainerCookedData.ExternalSources);
					CookedData.SwitchContainerLeaves.Add(MoveTemp(SwitchContainerCookedData));
				}
			}
		}

		// Finalize banks and media
		SoundBankSet.AppendTo(CookedData.SoundBanks);
		if (CookedData.SoundBanks.Num() == 0)
		{
			UE_LOG(LogWwiseResourceCooker, Log,
				TEXT("GetEventCookedData (%s %" PRIu32
					" %s): No SoundBank set for Event. Unless Switch values are properly set, no SoundBank will be loaded."
				),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		}
		MediaSet.AppendTo(CookedData.Media);
		ExternalSourceSet.AppendTo(CookedData.ExternalSources);

		for (const auto& SwitchRef : RequiredGroupValueSet)
		{
			FWwiseGroupValueCookedData SwitchCookedData;
			switch (SwitchRef.GetType())
			{
			case WwiseRefType::Switch: SwitchCookedData.Type = EWwiseGroupType::Switch; break;
			case WwiseRefType::State: SwitchCookedData.Type = EWwiseGroupType::State; break;
			default: SwitchCookedData.Type = EWwiseGroupType::Unknown;
			}
			SwitchCookedData.GroupId = SwitchRef.GetGroupId();
			SwitchCookedData.Id = SwitchRef.GetId();
			if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
			{
				SwitchCookedData.DebugName = FName();
			}
			else
			{
				SwitchCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? SwitchRef.GetName() : SwitchRef.GetObjectPath()));
			}

			CookedData.RequiredGroupValueSet.Add(MoveTemp(SwitchCookedData));
		}

		CookedData.DestroyOptions = InInfo.DestroyOptions;

		OutCookedData.EventLanguageMap.Add(FWwiseLanguageCookedData(LanguageId.GetLanguageId(), FName(*LanguageId.GetLanguageName()), (EWwiseLanguageRequirement)LanguageId.LanguageRequirement), MoveTemp(CookedData));
	}

	if (UNLIKELY(OutCookedData.EventLanguageMap.Num() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetEventCookedData (%s %" PRIu32 " %s): No Event"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	// Make this a SFX if all CookedData are identical
	{
		auto& Map = OutCookedData.EventLanguageMap;
		TArray<FWwiseLanguageCookedData> Keys;
		Map.GetKeys(Keys);

		auto LhsKey = Keys.Pop(EWwiseAllowShrinking::No);
		const auto* Lhs = Map.Find(LhsKey);
		while (Keys.Num() > 0)
		{
			auto RhsKey = Keys.Pop(EWwiseAllowShrinking::No);
			const auto* Rhs = Map.Find(RhsKey);

			if (Lhs->EventId != Rhs->EventId
				|| Lhs->DebugName != Rhs->DebugName
				|| Lhs->SoundBanks.Num() != Rhs->SoundBanks.Num()
				|| Lhs->ExternalSources.Num() != Rhs->ExternalSources.Num()
				|| Lhs->Media.Num() != Rhs->Media.Num()
				|| Lhs->RequiredGroupValueSet.Num() != Rhs->RequiredGroupValueSet.Num()
				|| Lhs->SwitchContainerLeaves.Num() != Rhs->SwitchContainerLeaves.Num())
			{
				UE_LOG(LogWwiseResourceCooker, VeryVerbose,
					TEXT("GetEventCookedData (%s %" PRIu32 " %s): Event has languages"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return true;
			}
			for (const auto& Elem : Lhs->SoundBanks)
			{
				if (!Rhs->SoundBanks.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetEventCookedData (%s %" PRIu32 " %s): Event has languages due to banks"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
			for (const auto& Elem : Lhs->ExternalSources)
			{
				if (!Rhs->ExternalSources.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetEventCookedData (%s %" PRIu32 " %s): Event has languages due to external sources"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
			for (const auto& Elem : Lhs->Media)
			{
				if (!Rhs->Media.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetEventCookedData (%s %" PRIu32 " %s): Event has languages due to media"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
			for (const auto& Elem : Lhs->RequiredGroupValueSet)
			{
				if (!Rhs->RequiredGroupValueSet.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetEventCookedData (%s %" PRIu32
							" %s): Event has languages due to required group values"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
			for (const auto& LhsLeaf : Lhs->SwitchContainerLeaves)
			{
				const auto RhsLeafIndex = Rhs->SwitchContainerLeaves.Find(LhsLeaf);
				if (RhsLeafIndex == INDEX_NONE)
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetEventCookedData (%s %" PRIu32 " %s): Event has languages due to switch container"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
				const auto& RhsLeaf = Rhs->SwitchContainerLeaves[RhsLeafIndex];

				for (const auto& Elem : LhsLeaf.SoundBanks)
				{
					if (!RhsLeaf.SoundBanks.Contains(Elem))
					{
						UE_LOG(LogWwiseResourceCooker, VeryVerbose,
							TEXT("GetEventCookedData (%s %" PRIu32
								" %s): Event has languages due to banks in switch container"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return true;
					}
				}
				for (const auto& Elem : LhsLeaf.ExternalSources)
				{
					if (!RhsLeaf.ExternalSources.Contains(Elem))
					{
						UE_LOG(LogWwiseResourceCooker, VeryVerbose,
							TEXT("GetEventCookedData (%s %" PRIu32
								" %s): Event has languages due to external sources in switch container"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return true;
					}
				}
				for (const auto& Elem : LhsLeaf.Media)
				{
					if (!RhsLeaf.Media.Contains(Elem))
					{
						UE_LOG(LogWwiseResourceCooker, VeryVerbose,
							TEXT("GetEventCookedData (%s %" PRIu32
								" %s): Event has languages due to media in switch container"),
							*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
						return true;
					}
				}
			}
		}

		UE_LOG(LogWwiseResourceCooker, VeryVerbose, TEXT("GetEventCookedData (%s %" PRIu32 " %s): Event is a SFX"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		std::remove_reference_t<decltype(Map)> SfxMap;
		SfxMap.Add(FWwiseLanguageCookedData::Sfx, *Lhs);

		Map = SfxMap;
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetExternalSourceCookedData(FWwiseExternalSourceCookedData& OutCookedData,
	uint32 InCookie) const
{
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetExternalSourceCookedData (%" PRIu32 "): ProjectDatabase not initialized"),
			InCookie);
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetExternalSourceCookedData (%" PRIu32 "): No data for platform"),
			InCookie);
		return false;
	}

	const auto LocalizableId = WwiseDatabaseLocalizableIdKey(InCookie, WwiseDatabaseLocalizableIdKey::GENERIC_LANGUAGE);
	const auto* ExternalSourceRef = PlatformData->ExternalSources.Find(LocalizableId);
	if (UNLIKELY(!ExternalSourceRef))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetExternalSourceCookedData (%" PRIu32 "): Could not find External Source"),
			InCookie);
		return false;
	}
	const auto* ExternalSource = ExternalSourceRef->GetExternalSource();
	if (UNLIKELY(!ExternalSource))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetExternalSourceCookedData (%" PRIu32 "): Could not get External Source"),
			InCookie);
		return false;
	}

	if (UNLIKELY(!FillExternalSourceBaseInfo(OutCookedData, *ExternalSource)))
	{
		return false;
	}
	return true;
}

bool FWwiseResourceCookerImpl::GetGameParameterCookedData(FWwiseGameParameterCookedData& OutCookedData,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.ShortId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetGameParameterCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetGameParameterCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefGameParameter GameParameterRef;
	
	if (UNLIKELY(!PlatformData->GetRef(GameParameterRef, WwiseDBSharedLanguageId(), ConvertWwiseObjectInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetGameParameterCookedData (%s %" PRIu32 " %s): No game parameter found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const auto* GameParameter = GameParameterRef.GetGameParameter();

	OutCookedData.ShortId = GameParameter->Id;
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutCookedData.DebugName = FName();
	}
	else
	{
		OutCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? GameParameter->Name : GameParameter->ObjectPath));
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetInitBankCookedData(FWwiseInitBankCookedData& OutCookedData, const TCHAR* ContentFolderName,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.SoundBankId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const auto* PlatformInfo = PlatformData->PlatformRef.GetPlatformInfo();
	if (UNLIKELY(!PlatformInfo))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): No Platform Info"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefSoundBank SoundBankRef;
	if (UNLIKELY(!PlatformData->GetRef(SoundBankRef, WwiseDBSharedLanguageId(), ConvertWwiseObjectInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): No ref found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	if (UNLIKELY(!SoundBankRef.IsInitBank()))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): Not an init SoundBank"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	{
		const auto* SoundBank = SoundBankRef.GetSoundBank();
		if (UNLIKELY(!SoundBank))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): Could not get SoundBank from Ref"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}
		if (UNLIKELY(!FillSoundBankBaseInfo(OutCookedData, *PlatformInfo, *SoundBank, ContentFolderName)))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetInitBankCookedData (%s %" PRIu32 " %s): Could not fill SoundBank from Data"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}

		// Add all Init SoundBank media
		WwiseDBSet<FWwiseSoundBankCookedData> SoundBankSet;
		WwiseDBSet<FWwiseMediaCookedData> MediaSet;
		{
			for (const auto& MediaRef : SoundBankRef.GetSoundBankMedia(PlatformData->MediaFiles))
			{
				WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
				if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
				{
					return false;
				}
			}
		}

		{
			WwiseCustomPluginIdsMap CustomPluginsRefs = SoundBankRef.GetSoundBankCustomPlugins(
				PlatformData->CustomPlugins);
			for (const auto& Plugin : CustomPluginsRefs)
			{
				WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
				const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
				for (const auto& MediaRef : MediaRefs)
				{
					WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
					if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
					{
						return false;
					}
				}
			}
		}

		{
			WwisePluginShareSetIdsMap ShareSetRefs = SoundBankRef.GetSoundBankPluginShareSets(
				PlatformData->PluginShareSets);
			for (const auto& ShareSet : ShareSetRefs)
			{
				WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
				const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
				for (const auto& MediaRef : MediaRefs)
				{
					WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
					if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
					{
						return false;
					}
				}
			}
		}

		{
			WwiseAudioDeviceIdsMap AudioDevicesRefs = SoundBankRef.GetSoundBankAudioDevices(PlatformData->AudioDevices);
			for (const auto& AudioDevice : AudioDevicesRefs)
			{
				WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
				const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
				for (const auto& MediaRef : MediaRefs)
				{
					WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
					if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
					{
						return false;
					}
				}
			}
		}

		for(const auto& SoundBankToAdd : SoundBankSet)
		{
			OutCookedData.SoundBanks.Add(SoundBankToAdd);
		}
		for(const auto& MediaToAdd : MediaSet)
		{
			OutCookedData.Media.Add(MediaToAdd);
		}
		const WwiseDBSet<WwiseDBSharedLanguageId>& Languages = DataStructure.GetLanguages();
		OutCookedData.Language.Empty(Languages.Size());
		for (const WwiseDBSharedLanguageId& Language : Languages)
		{
			OutCookedData.Language.Add({ (int32)Language.GetLanguageId(), FName(*Language.GetLanguageName()), (EWwiseLanguageRequirement)Language.LanguageRequirement });
		}

		if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
		{
			OutCookedData.DebugName = FName();
		}
		else
		{
			OutCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? SoundBank->ShortName : SoundBank->ObjectPath));
		}
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetMediaCookedData(FWwiseMediaCookedData& OutCookedData, const TCHAR* ContentFolderName,
	const FWwiseObjectInfo& InInfo) const
{
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetMediaCookedData (%" PRIu32 "): ProjectDatabase not initialized"),
			InInfo.WwiseShortId);
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetMediaCookedData (%" PRIu32 "): No data for platform"),
			InInfo.WwiseShortId);
		return false;
	}

	auto MediaId = WwiseDatabaseMediaIdKey(InInfo.WwiseShortId, InInfo.HardCodedSoundBankShortId);
	const auto* MediaRef = PlatformData->MediaFiles.Find(MediaId);
	if (UNLIKELY(!MediaRef))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetMediaCookedData (%" PRIu32 "): Could not find Media in SoundBank %" PRIu32),
			InInfo.WwiseShortId, InInfo.HardCodedSoundBankShortId);
		return false;
	}

	const WwiseDBSharedLanguageId* LanguageRefPtr = nullptr;
	if (MediaRef->LanguageId)
	{
		const auto& Languages = DataStructure.GetLanguages();
		for (const auto& Language : Languages)
		{
			if (Language.GetLanguageId() == MediaRef->LanguageId)
			{
				LanguageRefPtr = &Language;
				break;
			}
		}
		if (UNLIKELY(!LanguageRefPtr))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetMediaCookedData (%" PRIu32 "): Could not find language %" PRIu32),
				InInfo.WwiseShortId, MediaRef->LanguageId);
			return false;
		}
	}
	const auto& LanguageRef = LanguageRefPtr ? *LanguageRefPtr : WwiseDBSharedLanguageId();

	WwiseDBSet<FWwiseSoundBankCookedData> SoundBankSet;
	WwiseDBSet<FWwiseMediaCookedData> MediaSet;
	if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, *MediaRef, LanguageRef, *PlatformData, ContentFolderName)))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetMediaCookedData (%" PRIu32 "): Could not get requirements for media."),
			InInfo.WwiseShortId);
		return false;
	}

	if (UNLIKELY(SoundBankSet.Size() > 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetMediaCookedData (%" PRIu32 "): Asking for a media in a particular SoundBank (%" PRIu32
				") must have it fully defined in this SoundBank."),
			InInfo.WwiseShortId, InInfo.HardCodedSoundBankShortId);
		return false;
	}

	if (MediaSet.Size() == 0)
	{
		// Not directly an error: Media is in this SoundBank, without streaming. Can be a logical error, but it's not our error.
		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("GetMediaCookedData (%" PRIu32 "): Media is fully in SoundBank. Returning no media."),
			InInfo.WwiseShortId);
		return false;
	}

	auto Media = MediaSet.AsArray()[0];

	OutCookedData = MoveTemp(Media);
	return true;
}

bool FWwiseResourceCookerImpl::GetShareSetCookedData(FWwiseLocalizedShareSetCookedData& OutCookedData, const TCHAR* ContentFolderName,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.ShareSetId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDBSet<WwiseDBSharedLanguageId>& Languages = DataStructure.GetLanguages();

	const auto* PlatformInfo = PlatformData->PlatformRef.GetPlatformInfo();
	if (UNLIKELY(!PlatformInfo))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): No Platform Info"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseDBMap<WwiseDBSharedLanguageId, WwiseDBSet<WwiseRefPluginShareSet>> RefLanguageMap;
	PlatformData->GetRefMap(RefLanguageMap, Languages, ConvertWwiseObjectInfo(InInfo));
	if (UNLIKELY(RefLanguageMap.Size() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): No ref found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.ShareSetLanguageMap.Empty(RefLanguageMap.Size());

	for (auto& Ref : RefLanguageMap)
	{
		WwiseDBPair<const WwiseDBSharedLanguageId, WwiseDBSet<WwiseRefPluginShareSet>> RefPair(Ref);
		FWwiseShareSetCookedData CookedData;
		WwiseDBSet<FWwiseSoundBankCookedData> SoundBankSet;
		WwiseDBSet<FWwiseMediaCookedData> MediaSet;
		WwiseDBSet<WwiseRefPluginShareSet> ShareSets = RefPair.GetSecond();

		if (UNLIKELY(ShareSets.Size() == 0))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetAuxBusCookedData (%s %" PRIu32 " %s): Empty ref for language"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}

		// Set up basic global Aux bus information
		{
			auto FirstShareSet(ShareSets.GetFirst());
			CookedData.ShareSetId = FirstShareSet.PluginShareSetId();
			if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
			{
				OutCookedData.DebugName = FName();
			}
			else
			{
				CookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? *FirstShareSet.PluginShareSetName() : *FirstShareSet.PluginShareSetObjectPath()));
				OutCookedData.DebugName = CookedData.DebugName;
			}
			OutCookedData.ShareSetId = CookedData.ShareSetId;
		}
		for (auto& ShareSetRef : ShareSets)
		{
			const auto* PluginShareSet = ShareSetRef.GetPlugin();
			if (UNLIKELY(!PluginShareSet))
			{
				UE_LOG(LogWwiseResourceCooker, Warning,
					TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): Could not get ShareSet from Ref"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return false;
			}
			CookedData.ShareSetId = PluginShareSet->Id;

			const auto* SoundBank = ShareSetRef.GetSoundBank();
			if (UNLIKELY(!SoundBank))
			{
				UE_LOG(LogWwiseResourceCooker, Warning,
					TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): Could not get SoundBank from Ref"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return false;
			}
			if (!SoundBank->IsInitBank())
			{
				FWwiseSoundBankCookedData MainSoundBank;
				if (UNLIKELY(!FillSoundBankBaseInfo(MainSoundBank, *PlatformInfo, *SoundBank, ContentFolderName)))
				{
					UE_LOG(LogWwiseResourceCooker, Warning,
						TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): Could not fill SoundBank from Data"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return false;
				}
				SoundBankSet.Add(MainSoundBank);
			}

			{
				WwiseCustomPluginIdsMap CustomPluginsRefs = ShareSetRef.GetPluginCustomPlugins(
					PlatformData->CustomPlugins);
				for (const auto& Plugin : CustomPluginsRefs)
				{
					WwiseDBPair<const unsigned, WwiseRefCustomPlugin> PluginPair(Plugin);
					const WwiseMediaIdsMap MediaRefs = PluginPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
					for (const auto& MediaRef : MediaRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
						if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
						{
							return false;
						}
					}
				}
			}

			{
				WwisePluginShareSetIdsMap ShareSetRefs = ShareSetRef.GetPluginPluginShareSets(
					PlatformData->PluginShareSets);
				for (const auto& ShareSet : ShareSetRefs)
				{
					WwiseDBPair<const unsigned, WwiseRefPluginShareSet> ShareSetPair(ShareSet);
					const WwiseMediaIdsMap MediaRefs = ShareSetPair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
					for (const auto& MediaRef : MediaRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
						if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
						{
							return false;
						}
					}
				}
			}

			{
				WwiseAudioDeviceIdsMap AudioDevicesRefs = ShareSetRef.GetPluginAudioDevices(PlatformData->AudioDevices);
				for (const auto& AudioDevice : AudioDevicesRefs)
				{
					WwiseDBPair<const unsigned, WwiseRefAudioDevice> AudioDevicePair(AudioDevice);
					const WwiseMediaIdsMap MediaRefs = AudioDevicePair.GetSecond().GetPluginMedia(PlatformData->MediaFiles);
					for (const auto& MediaRef : MediaRefs)
					{
						WwiseDBPair<const unsigned, WwiseRefMedia> MediaRefPair(MediaRef);
						if (UNLIKELY(!AddRequirementsForMedia(SoundBankSet, MediaSet, MediaRefPair.GetSecond(), WwiseDBSharedLanguageId(), *PlatformData, ContentFolderName)))
						{
							return false;
						}
					}
				}
			}
		}

		for(auto& SoundBank : SoundBankSet)
		{
			CookedData.SoundBanks.Add(SoundBank);
		}
		for(auto& Media : MediaSet)
		{
			CookedData.Media.Add(Media);
		}
		OutCookedData.ShareSetLanguageMap.Add(FWwiseLanguageCookedData((int32)RefPair.GetFirst().GetLanguageId(), *RefPair.GetFirst().GetLanguageName(), (EWwiseLanguageRequirement)RefPair.GetFirst().LanguageRequirement), MoveTemp(CookedData));
	}

	if (UNLIKELY(OutCookedData.ShareSetLanguageMap.Num() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): No ShareSet"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	// Make this a SFX if all CookedData are identical
	{
		auto& Map = OutCookedData.ShareSetLanguageMap;
		TArray<FWwiseLanguageCookedData> Keys;
		Map.GetKeys(Keys);

		auto LhsKey = Keys.Pop(EWwiseAllowShrinking::No);
		const auto* Lhs = Map.Find(LhsKey);
		while (Keys.Num() > 0)
		{
			auto RhsKey = Keys.Pop(EWwiseAllowShrinking::No);
			const auto* Rhs = Map.Find(RhsKey);

			if (Lhs->ShareSetId != Rhs->ShareSetId
				|| Lhs->DebugName != Rhs->DebugName
				|| Lhs->SoundBanks.Num() != Rhs->SoundBanks.Num()
				|| Lhs->Media.Num() != Rhs->Media.Num())
			{
				UE_LOG(LogWwiseResourceCooker, VeryVerbose,
					TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): ShareSet has languages"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return true;
			}
			for (const auto& Elem : Lhs->SoundBanks)
			{
				if (!Rhs->SoundBanks.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): ShareSet has languages due to banks"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
			for (const auto& Elem : Lhs->Media)
			{
				if (!Rhs->Media.Contains(Elem))
				{
					UE_LOG(LogWwiseResourceCooker, VeryVerbose,
						TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): ShareSet has languages due to media"),
						*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
					return true;
				}
			}
		}

		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("GetShareSetCookedData (%s %" PRIu32 " %s): ShareSet is a SFX"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		std::remove_reference_t<decltype(Map)> SfxMap;
		SfxMap.Add(FWwiseLanguageCookedData::Sfx, *Lhs);

		Map = SfxMap;
	}

	return true;
}

bool FWwiseResourceCookerImpl::GetSoundBankCookedData(FWwiseLocalizedSoundBankCookedData& OutCookedData, const TCHAR* ContentFolderName,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.SoundBankId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDBSet<WwiseDBSharedLanguageId>& Languages = DataStructure.GetLanguages();

	const auto* PlatformInfo = PlatformData->PlatformRef.GetPlatformInfo();
	if (UNLIKELY(!PlatformInfo))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): No Platform Info"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseDBMap<WwiseDBSharedLanguageId, WwiseRefSoundBank> RefLanguageMap;
	PlatformData->GetRefMap(RefLanguageMap, Languages, ConvertWwiseObjectInfo(InInfo));
	if (UNLIKELY(RefLanguageMap.Size() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): No ref found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.SoundBankLanguageMap.Empty(RefLanguageMap.Size());

	for (const auto& Ref : RefLanguageMap)
	{
		WwiseDBPair<const WwiseDBSharedLanguageId, WwiseRefSoundBank> RefPair(Ref);
		FWwiseSoundBankCookedData CookedData;
		const auto* SoundBank = RefPair.GetSecond().GetSoundBank();
		if (UNLIKELY(!SoundBank))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): Could not get SoundBank from Ref"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}
		if (UNLIKELY(!FillSoundBankBaseInfo(CookedData, *PlatformInfo, *SoundBank, ContentFolderName)))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): Could not fill SoundBank from Data"),
				*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
			return false;
		}

		if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
		{
			OutCookedData.DebugName = FName();
		}
		else
		{
			CookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? SoundBank->ShortName : SoundBank->ObjectPath));
			OutCookedData.DebugName = CookedData.DebugName;
		}

		OutCookedData.SoundBankId = CookedData.SoundBankId;
		OutCookedData.SoundBankLanguageMap.Add(FWwiseLanguageCookedData(RefPair.GetFirst().GetLanguageId(), FName(*RefPair.GetFirst().GetLanguageName()), (EWwiseLanguageRequirement)RefPair.GetFirst().LanguageRequirement), MoveTemp(CookedData));
	}

	if (UNLIKELY(OutCookedData.SoundBankLanguageMap.Num() == 0))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): No SoundBank"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	// Make this a SFX if all CookedData are identical
	{
		auto& Map = OutCookedData.SoundBankLanguageMap;
		TArray<FWwiseLanguageCookedData> Keys;
		Map.GetKeys(Keys);

		auto LhsKey = Keys.Pop(EWwiseAllowShrinking::No);
		const auto* Lhs = Map.Find(LhsKey);
		while (Keys.Num() > 0)
		{
			auto RhsKey = Keys.Pop(EWwiseAllowShrinking::No);
			const auto* Rhs = Map.Find(RhsKey);

			if (GetTypeHash(*Lhs) != GetTypeHash(*Rhs))
			{
				UE_LOG(LogWwiseResourceCooker, VeryVerbose,
					TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): SoundBank has languages"),
					*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
				return true;
			}
		}

		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("GetSoundBankCookedData (%s %" PRIu32 " %s): SoundBank is a SFX"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		std::remove_reference_t<decltype(Map)> SfxMap;
		SfxMap.Add(FWwiseLanguageCookedData::Sfx, *Lhs);

		Map = SfxMap;
	}
	return true;
}

bool FWwiseResourceCookerImpl::GetStateCookedData(FWwiseGroupValueCookedData& OutCookedData,
	const FWwiseGroupValueInfo& InInfo) const
{
	OutCookedData.Id = InInfo.WwiseShortId;
	OutCookedData.GroupId = InInfo.GroupShortId;
	OutCookedData.DebugName = InInfo.WwiseName;
	
	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetStateCookedData (%s %" PRIu32 " %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetStateCookedData (%s %" PRIu32 " %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefState StateRef;
	if (UNLIKELY(!PlatformData->GetRef(StateRef, WwiseDBSharedLanguageId(), ConvertWwiseGroupValueInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetStateCookedData (%s %" PRIu32 " %" PRIu32 " %s): No state found"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}
	const auto* State = StateRef.GetState();
	const auto* StateGroup = StateRef.GetStateGroup();
	if (UNLIKELY(!State || !StateGroup))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetStateCookedData (%s %" PRIu32 " %" PRIu32 " %s): No state in ref"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.Type = EWwiseGroupType::State;
	OutCookedData.GroupId = StateGroup->Id;
	OutCookedData.Id = State->Id;
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutCookedData.DebugName = FName();
	}
	else
	{
		OutCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? State->Name : State->ObjectPath));
	}
	return true;
}

bool FWwiseResourceCookerImpl::GetSwitchCookedData(FWwiseGroupValueCookedData& OutCookedData,
	const FWwiseGroupValueInfo& InInfo) const
{
	OutCookedData.Id = InInfo.WwiseShortId;
	OutCookedData.GroupId = InInfo.GroupShortId;
	OutCookedData.DebugName = InInfo.WwiseName;

	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetSwitchCookedData (%s %" PRIu32 " %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetSwitchCookedData (%s %" PRIu32 " %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefSwitch SwitchRef;
	if (UNLIKELY(!PlatformData->GetRef(SwitchRef, WwiseDBSharedLanguageId(), ConvertWwiseGroupValueInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetSwitchCookedData (%s %" PRIu32 " %" PRIu32 " %s): No switch found"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}
	const auto* Switch = SwitchRef.GetSwitch();
	const auto* SwitchGroup = SwitchRef.GetSwitchGroup();
	if (UNLIKELY(!Switch || !SwitchGroup))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("GetSwitchCookedData (%s %" PRIu32 " %" PRIu32 " %s): No switch in ref"),
			*InInfo.WwiseGuid.ToString(), InInfo.GroupShortId, InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	OutCookedData.Type = EWwiseGroupType::Switch;
	OutCookedData.GroupId = SwitchGroup->Id;
	OutCookedData.Id = Switch->Id;
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutCookedData.DebugName = FName();
	}
	else
	{
		OutCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? Switch->Name : Switch->ObjectPath));
	}
	return true;
}

bool FWwiseResourceCookerImpl::GetTriggerCookedData(FWwiseTriggerCookedData& OutCookedData,
	const FWwiseObjectInfo& InInfo) const
{
	OutCookedData.TriggerId = InInfo.WwiseShortId;
	OutCookedData.DebugName = InInfo.WwiseName;

	const auto* ProjectDatabase = GetProjectDatabase();
	if (UNLIKELY(!ProjectDatabase))
	{
		UE_LOG(LogWwiseResourceCooker, Error,
			TEXT("GetTriggerCookedData (%s %" PRIu32 " %s): ProjectDatabase not initialized"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const WwiseDataStructureScopeLock DataStructure(*ProjectDatabase);
	const auto* PlatformData = DataStructure.GetCurrentPlatformData();
	if (UNLIKELY(!PlatformData))
	{
		UE_LOG(LogWwiseResourceCooker, Error, TEXT("GetTriggerCookedData (%s %" PRIu32 " %s): No data for platform"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	WwiseRefTrigger TriggerRef;

	if (UNLIKELY(!PlatformData->GetRef(TriggerRef, WwiseDBSharedLanguageId(), ConvertWwiseObjectInfo(InInfo))))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("GetTriggerCookedData (%s %" PRIu32 " %s): No trigger data found"),
			*InInfo.WwiseGuid.ToString(), InInfo.WwiseShortId, *InInfo.WwiseName.ToString());
		return false;
	}

	const auto* Trigger = TriggerRef.GetTrigger();

	OutCookedData.TriggerId = Trigger->Id;
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutCookedData.DebugName = FName();
	}
	else
	{
		OutCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? Trigger->Name : Trigger->ObjectPath));
	}

	return true;
}

bool FWwiseResourceCookerImpl::FillSoundBankBaseInfo(FWwiseSoundBankCookedData& OutSoundBankCookedData,
	const WwiseMetadataPlatformInfo& InPlatformInfo,
	const WwiseMetadataSoundBank& InSoundBank,
	const TCHAR* ContentFolderName) const
{
	if (CookingCache)
	{
		int A, B, C, D;
		InSoundBank.GUID.GetGuidValues(A, B, C, D);
		if (const auto* CachedData = CookingCache->SoundBankCache.Find(
			FWwiseObjectInfo(FGuid(A, B, C, D), InSoundBank.Id, FString(*InSoundBank.ShortName))))
		{
			for (const auto& Language : CachedData->SoundBankLanguageMap)
			{
				if (Language.Key.GetLanguageName().ToString() == *InSoundBank.Language)
				{
					OutSoundBankCookedData = Language.Value;
					return true;
				}
			}
		}
	}
	
	OutSoundBankCookedData.SoundBankId = InSoundBank.Id;
	OutSoundBankCookedData.PackagedFile.ModularGameplayName = ContentFolderName;
	OutSoundBankCookedData.PackagedFile.MemoryAlignment = InSoundBank.Align == 0
		? InPlatformInfo.DefaultAlign
		: InSoundBank.Align;
	OutSoundBankCookedData.PackagedFile.bDeviceMemory = InSoundBank.bDeviceMemory;
	OutSoundBankCookedData.bContainsMedia = InSoundBank.ContainsMedia();
	switch (InSoundBank.Type)
	{
	case EMetadataSoundBankType::Bus:
		OutSoundBankCookedData.SoundBankType = EWwiseSoundBankType::Bus;
		break;
	case EMetadataSoundBankType::Event:
		OutSoundBankCookedData.SoundBankType = EWwiseSoundBankType::Event;
		break;
	case EMetadataSoundBankType::User:
	default:
		OutSoundBankCookedData.SoundBankType = EWwiseSoundBankType::User;
	}
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutSoundBankCookedData.DebugName = FName();
	}
	else
	{
		OutSoundBankCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? InSoundBank.ShortName : InSoundBank.ObjectPath));
	}
	UpdatePackagedFile(OutSoundBankCookedData.PackagedFile, InSoundBank);
	return true;
}

bool FWwiseResourceCookerImpl::FillMediaBaseInfo(FWwiseMediaCookedData& OutMediaCookedData,
	const WwiseMetadataPlatformInfo& InPlatformInfo,
	const WwiseMetadataSoundBank& InSoundBank,
	const WwiseMetadataMediaReference& InMediaReference,
	const TCHAR* ContentFolderName) const
{
	for (const auto& Media : InSoundBank.Media)
	{
		if (Media.Id == InMediaReference.Id)
		{
			return FillMediaBaseInfo(OutMediaCookedData, InPlatformInfo, InSoundBank, Media, ContentFolderName);
		}
	}
	UE_LOG(LogWwiseResourceCooker, Warning,
		TEXT("FillMediaBaseInfo: Could not get Media Reference %" PRIu32 " in SoundBank %s %" PRIu32),
		InMediaReference.Id, *FWwiseStringConverter::ToFString(InSoundBank.ShortName), InSoundBank.Id);
	return false;
}

bool FWwiseResourceCookerImpl::FillMediaBaseInfo(FWwiseMediaCookedData& OutMediaCookedData,
	const WwiseMetadataPlatformInfo& InPlatformInfo,
	const WwiseMetadataSoundBank& InSoundBank,
	const WwiseMetadataMedia& InMedia,
	const TCHAR* ContentFolderName) const
{
	if (CookingCache)
	{
		if (const auto* CachedData = CookingCache->MediaCache.Find(
			WwiseDatabaseMediaIdKey(InMedia.Id, InSoundBank.Id)))
		{
			OutMediaCookedData = *CachedData;
			return true;
		}
	}
	
	OutMediaCookedData.MediaId = InMedia.Id;

	if (InMedia.Location == WwiseMetadataMediaLocation::Memory)
	{
		// In-Memory (User-defined SoundBank) already have the prefetch in the related SoundBank.
		OutMediaCookedData.PackagedFile.PrefetchSize = 0;
	}
	else
	{
		OutMediaCookedData.PackagedFile.PrefetchSize = InMedia.PrefetchSize;
	}

	OutMediaCookedData.PackagedFile.MemoryAlignment = InMedia.Align == 0 ? InPlatformInfo.DefaultAlign : InMedia.Align;
	OutMediaCookedData.PackagedFile.bDeviceMemory = InMedia.bDeviceMemory;
	OutMediaCookedData.PackagedFile.bStreaming = InMedia.bStreaming;

	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutMediaCookedData.DebugName = FName();
	}
	else
	{
		OutMediaCookedData.DebugName = FName(*InMedia.ShortName);
	}

	UpdatePackagedFile(OutMediaCookedData.PackagedFile, InMedia);
	return true;
}

bool FWwiseResourceCookerImpl::FillExternalSourceBaseInfo(FWwiseExternalSourceCookedData& OutExternalSourceCookedData,
	const WwiseMetadataExternalSource& InExternalSource) const
{
	OutExternalSourceCookedData.Cookie = InExternalSource.Cookie;
	if (ExportDebugNameRule == EWwiseExportDebugNameRule::Release)
	{
		OutExternalSourceCookedData.DebugName = FName();
	}
	else
	{
		OutExternalSourceCookedData.DebugName = FName(*(ExportDebugNameRule == EWwiseExportDebugNameRule::Name ? InExternalSource.Name : InExternalSource.ObjectPath));
	}
	return true;
}

bool FWwiseResourceCookerImpl::AddRequirementsForMedia(WwiseDBSet<FWwiseSoundBankCookedData>& OutSoundBankSet, WwiseDBSet<FWwiseMediaCookedData>& OutMediaSet,
	const WwiseRefMedia& InMediaRef,
	const WwiseDBSharedLanguageId& InLanguage,
	const WwisePlatformDataStructure& InPlatformData,
	const TCHAR* ContentFolderName) const
{
	const auto* Media = InMediaRef.GetMedia();
	if (UNLIKELY(!Media))
	{
		UE_LOG(LogWwiseResourceCooker, Warning, TEXT("AddRequirementsForMedia: Could not get Media from Media Ref"));
		return false;
	}

	const auto* PlatformInfo = InPlatformData.PlatformRef.GetPlatformInfo();
	if (UNLIKELY(!PlatformInfo))
	{
		return false;
	}

	if (Media->Location == WwiseMetadataMediaLocation::Memory && !Media->bStreaming)
	{
		// In-Memory media is already loaded with current SoundBank
		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
				"): Media is in memory and not streaming. Skipping."),
			*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id,
			*FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());
	}
	else if (Media->Location == WwiseMetadataMediaLocation::OtherBank)
	{
		// Media resides in another SoundBank. Find that other SoundBank and add it as a requirement.
		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
				"): Media is in another SoundBank. Locate SoundBank and add requirement."),
			*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id,
			*FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());

		FWwiseObjectInfo MediaInfo;
		MediaInfo.WwiseShortId = Media->Id;
		MediaInfo.WwiseName = FName(*FWwiseStringConverter::ToFString(Media->ShortName));

		WwiseRefMedia OtherSoundBankMediaRef;
		if (UNLIKELY(!InPlatformData.GetRef(OtherSoundBankMediaRef, InLanguage, ConvertWwiseObjectInfo(MediaInfo))))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
					"): Could not get Ref for other SoundBank media %s %" PRIu32 " %s"),
				*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId(),
				*MediaInfo.WwiseGuid.ToString(), MediaInfo.WwiseShortId, *MediaInfo.WwiseName.ToString());
			return false;
		}

		const auto* SoundBank = OtherSoundBankMediaRef.GetSoundBank();
		if (UNLIKELY(!SoundBank))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
					"): Could not get SoundBank from Media in another SoundBank Ref"),
				*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());
			return false;
		}

		if (SoundBank->IsInitBank())
		{
			// We assume Init SoundBanks are fully loaded
			UE_LOG(LogWwiseResourceCooker, VeryVerbose,
				TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
					"): Media is in Init SoundBank. Skipping."),
				*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());
		}

		FWwiseSoundBankCookedData MediaSoundBank;
		if (UNLIKELY(!FillSoundBankBaseInfo(MediaSoundBank, *PlatformInfo, *SoundBank, ContentFolderName)))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
					"): Could not fill SoundBank from Media in another SoundBank Data"),
				*FWwiseStringConverter::ToFString(*Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(*InLanguage.GetLanguageName()),
				InLanguage.GetLanguageId());
			return false;
		}
		OutSoundBankSet.Add(MoveTemp(MediaSoundBank));
	}
	else
	{
		// Media has a required loose file.
		UE_LOG(LogWwiseResourceCooker, VeryVerbose,
			TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32 "): Adding loose media requirement."),
			*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());

		const auto* SoundBank = InMediaRef.GetSoundBank();
		if (UNLIKELY(!SoundBank))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32 "): Could not get SoundBank from Media"
				),
				*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());
			return false;
		}

		FWwiseMediaCookedData MediaCookedData;
		if (UNLIKELY(!FillMediaBaseInfo(MediaCookedData, *PlatformInfo, *SoundBank, *Media, ContentFolderName)))
		{
			UE_LOG(LogWwiseResourceCooker, Warning,
				TEXT("AddRequirementsForMedia (%s %" PRIu32 " in %s %" PRIu32
					"): Could not fill Media from Media Ref"),
				*FWwiseStringConverter::ToFString(Media->ShortName), Media->Id, *FWwiseStringConverter::ToFString(InLanguage.GetLanguageName()), InLanguage.GetLanguageId());
			return false;
		}

		OutMediaSet.Add(MoveTemp(MediaCookedData));
	}

	return true;
}

bool FWwiseResourceCookerImpl::AddRequirementsForExternalSource(
	WwiseDBSet<FWwiseExternalSourceCookedData>& OutExternalSourceSet,
	const WwiseRefExternalSource& InExternalSourceRef) const
{
	const auto* ExternalSource = InExternalSourceRef.GetExternalSource();
	if (UNLIKELY(!ExternalSource))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("AddRequirementsForExternalSource: Could not get External Source from External Source Ref"));
		return false;
	}
	FWwiseExternalSourceCookedData ExternalSourceCookedData;
	if (UNLIKELY(!FillExternalSourceBaseInfo(ExternalSourceCookedData, *ExternalSource)))
	{
		UE_LOG(LogWwiseResourceCooker, Warning,
			TEXT("AddRequirementsForExternalSource (%s %" PRIu32
				"): Could not fill External Source from External Source Ref"),
			*FWwiseStringConverter::ToFString(ExternalSource->Name), ExternalSource->Cookie);
		return false;
	}
	OutExternalSourceSet.Add(MoveTemp(ExternalSourceCookedData));
	return true;
}

WwiseDBObjectInfo FWwiseResourceCookerImpl::ConvertWwiseObjectInfo(const FWwiseObjectInfo& InInfo) const
{
	return WwiseDBObjectInfo
	(WwiseDBGuid(InInfo.WwiseGuid.A, InInfo.WwiseGuid.B, InInfo.WwiseGuid.C, InInfo.WwiseGuid.D), InInfo.WwiseShortId,
		FWwiseStringConverter::ToWwiseDBString(InInfo.WwiseName.ToString()), InInfo.HardCodedSoundBankShortId);
}

WwiseDBGroupValueInfo FWwiseResourceCookerImpl::ConvertWwiseGroupValueInfo(
	const FWwiseGroupValueInfo& InInfo) const
{
	return WwiseDBGroupValueInfo
		(WwiseDBGuid(InInfo.WwiseGuid.A, InInfo.WwiseGuid.B, InInfo.WwiseGuid.C, InInfo.WwiseGuid.D),
		InInfo.GroupShortId, InInfo.WwiseShortId,
		FWwiseStringConverter::ToWwiseDBString(InInfo.WwiseName.ToString()));
}
