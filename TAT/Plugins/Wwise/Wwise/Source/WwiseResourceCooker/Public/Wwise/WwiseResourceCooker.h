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

#include "Wwise/WwiseResourceCookerModule.h"
#include "Wwise/WwiseSharedLanguageId.h"
#include "Wwise/CookedData/WwiseAudioDeviceShareSetCookedData.h"
#include "Wwise/CookedData/WwiseGroupValueCookedData.h"
#include "Wwise/Info/WwiseObjectInfo.h"

class UWwiseAssetLibrary;
struct FWwiseAssetLibraryCookedData;
struct FWwiseAssetLibraryInfo;
struct WwiseMetadataSoundBank;
struct WwiseMetadataMedia;
struct FWwiseShareSetCookedData;
struct FWwiseEventCookedData;
struct FWwiseAuxBusCookedData;
class FWwiseResourceLoader;
class FWwiseProjectDatabase;
struct FWwiseGroupValueInfo;
struct FWwiseLocalizedSoundBankCookedData;
struct FWwiseLocalizedShareSetCookedData;
struct FWwiseMediaCookedData;
struct FWwiseInitBankCookedData;
struct FWwiseExternalSourceCookedData;
struct FWwiseLocalizedEventCookedData;
struct FWwiseLocalizedAuxBusCookedData;
struct FWwiseEventInfo;
struct FWwiseSoundBankCookedData;
struct FWwiseGameParameterCookedData;
struct FWwiseTriggerCookedData;
struct FWwiseAcousticTextureCookedData;
class FWwiseCookingCache;

typedef TSharedPtr<FWwiseResourceLoader> FWwiseResourceLoaderPtr;

class WWISERESOURCECOOKER_API IWwiseResourceCooker
{
public:
	using WriteAdditionalFileFunction = TFunctionRef<void(const TCHAR* Filename, void* Data, int64 Size)>;

	static IWwiseResourceCooker* GetDefault()
	{
		if (auto* Module = IWwiseResourceCookerModule::GetModule())
		{
			return Module->GetResourceCooker();
		}
		return nullptr;
	}
	static IWwiseResourceCooker* Instantiate()
	{
		if (auto* Module = IWwiseResourceCookerModule::GetModule())
		{
			return Module->InstantiateResourceCooker();
		}
		return nullptr;
	}
	static IWwiseResourceCooker* Instantiate(const IWwiseResourceCooker& DefaultResourceCooker)
	{
		if (auto* ResourceCooker = Instantiate())
		{
			ResourceCooker->InitForStaging(DefaultResourceCooker);
			return ResourceCooker;
		}
		return nullptr;
	}

	static IWwiseResourceCooker* CreateForPlatform(
		const ITargetPlatform* TargetPlatform,
		const FWwiseSharedPlatformId& InPlatform,
		EWwisePackagingStrategy InTargetPackagingStrategy,
		EWwiseExportDebugNameRule InExportDebugNameRule)
	{
		if (auto* Module = IWwiseResourceCookerModule::GetModule())
		{
			return Module->CreateCookerForPlatform(TargetPlatform, InPlatform, InTargetPackagingStrategy, InExportDebugNameRule);
		}
		return nullptr;
	}

	static void DestroyForPlatform(const ITargetPlatform* TargetPlatform)
	{
		if (auto* Module = IWwiseResourceCookerModule::GetModule())
		{
			Module->DestroyCookerForPlatform(TargetPlatform);
		}
	}

	static IWwiseResourceCooker* GetForPlatform(const ITargetPlatform* TargetPlatform)
	{
		if (auto* Module = IWwiseResourceCookerModule::GetModule())
		{
			return Module->GetCookerForPlatform(TargetPlatform);
		}
		return nullptr;
	}

	static IWwiseResourceCooker* GetForArchive(const FArchive& InArchive)
	{
		if (auto* Module = IWwiseResourceCookerModule::GetModule())
		{
			return Module->GetCookerForArchive(InArchive);
		}
		return nullptr;
	}

	IWwiseResourceCooker() {}
	virtual ~IWwiseResourceCooker() {}

	virtual void CookAuxBus(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookEvent(const FWwiseEventInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookExternalSource(uint32 InCookie, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookInitBank(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookMedia(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookShareSet(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookSoundBank(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;

	virtual void PreCacheAssetLibraries(const IWwiseResourceCookerModule::FAssetLibraryInfoMap& AssetLibraryInfoMap) = 0;
	virtual void PrepareAssetLibrary(UObject* Owner, FWwiseAssetLibraryCookedData& OutCookedData, bool bPackageAssets) = 0;
	
	virtual bool PrepareCookedData(FWwiseAcousticTextureCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseAudioDeviceShareSetCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseLocalizedAuxBusCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseLocalizedEventCookedData& OutCookedData, const UObject* Owner, const FWwiseEventInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseExternalSourceCookedData& OutCookedData, uint32 InCookie) = 0;
	virtual bool PrepareCookedData(FWwiseGameParameterCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseGroupValueCookedData& OutCookedData, const FWwiseGroupValueInfo& InInfo, EWwiseGroupType InGroupType) = 0;
	virtual bool PrepareCookedData(FWwiseInitBankCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo = FWwiseObjectInfo::DefaultInitBank) = 0;
	virtual bool PrepareCookedData(FWwiseMediaCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseLocalizedShareSetCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseLocalizedSoundBankCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) = 0;
	virtual bool PrepareCookedData(FWwiseTriggerCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) = 0;

	virtual void PrepareResourceCookerForPlatform(FWwiseProjectDatabase*&& InProjectDatabaseOverride,
		EWwisePackagingStrategy InTargetPackagingStrategy,
		EWwiseExportDebugNameRule InExportDebugNameRule) = 0;

	virtual FWwiseProjectDatabase* GetProjectDatabase() = 0;
	virtual const FWwiseProjectDatabase* GetProjectDatabase() const = 0;
	virtual FWwiseResourceLoaderPtr GetResourceLoader() = 0;
	virtual const FWwiseResourceLoaderPtr GetResourceLoader() const = 0;

	virtual FWwiseSharedLanguageId GetCurrentLanguage() const = 0;
	virtual FWwiseSharedPlatformId GetCurrentPlatform() const = 0;

	virtual FString GetOwnerContentFolder(const UObject* Owner) const = 0;

	//
	// Low-level operations
	//

	virtual FWwiseCookingCache* GetCookingCache() = 0;

	virtual void CookLocalizedAuxBusToSandbox(const FWwiseLocalizedAuxBusCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookLocalizedSoundBankToSandbox(const FWwiseLocalizedSoundBankCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookLocalizedEventToSandbox(const FWwiseLocalizedEventCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookLocalizedShareSetToSandbox(const FWwiseLocalizedShareSetCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;

	virtual void CookAuxBusToSandbox(const FWwiseAuxBusCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookEventToSandbox(const FWwiseEventCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookExternalSourceToSandbox(const FWwiseExternalSourceCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookInitBankToSandbox(const FWwiseInitBankCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookMediaToSandbox(const FWwiseMediaCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookShareSetToSandbox(const FWwiseShareSetCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;
	virtual void CookSoundBankToSandbox(const FWwiseSoundBankCookedData& InCookedData, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;

	virtual void CookFileToSandbox(const FWwisePackagedFile& PackagedFile, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) = 0;

	virtual bool GetAcousticTextureCookedData(FWwiseAcousticTextureCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetAudioDeviceShareSetCookedData(FWwiseAudioDeviceShareSetCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetAuxBusCookedData(FWwiseLocalizedAuxBusCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetEventCookedData(FWwiseLocalizedEventCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseEventInfo& InInfo) const = 0;
	virtual bool GetExternalSourceCookedData(FWwiseExternalSourceCookedData& OutCookedData, uint32 InCookie) const = 0;
	virtual bool GetGameParameterCookedData(FWwiseGameParameterCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetInitBankCookedData(FWwiseInitBankCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo = FWwiseObjectInfo::DefaultInitBank) const = 0;
	virtual bool GetMediaCookedData(FWwiseMediaCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetShareSetCookedData(FWwiseLocalizedShareSetCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetSoundBankCookedData(FWwiseLocalizedSoundBankCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const = 0;
	virtual bool GetStateCookedData(FWwiseGroupValueCookedData& OutCookedData, const FWwiseGroupValueInfo& InInfo) const = 0;
	virtual bool GetSwitchCookedData(FWwiseGroupValueCookedData& OutCookedData, const FWwiseGroupValueInfo& InInfo) const = 0;
	virtual bool GetTriggerCookedData(FWwiseTriggerCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const = 0;

	/**
	 * Returns the root Content folder, based on CookAdditionalFilesOverride's PackageFilename.
	 *
	 * Used by CookFileToSandbox to determine the final output location of cooked files.
	 * 
	 * @param PackageFilename The Package Filename, as provided by CookAdditionalFilesOverride.
	 * @return The path to the root Content folder
	 */
	virtual FString GetCookRootPath(const TCHAR* PackageFilename) = 0;
	
	/**
	 * Set the path relative to the Content folder to store additional packaged files.
	 *
	 * This is retrieved from UAkSettings::WwiseStagingDirectory, where the files will be located.
	 *
	 * @param InPath Path relative to the Content folder where the additional packaged files will be stored.
	 */
	virtual void SetWwiseStagePath(const FString& InPath) = 0;

	/**
	 * Retrieve the path relative to the Content folder where the additional packaged files will be stored.
	 *
	 * This is used only in UpdatePackagedFile to write additional file paths. However, a game developer can
	 * override this default behavior through UpdatePackagedFile. The Wwise Stage Path is only used by
	 * this function.
	 * 
	 * @return The default path in which to store the additional Wwise packaged files. 
	 */
	virtual FString GetWwiseStagePath() const = 0;
	
	/**
	 * Update the Packaged File structure with all the necessary information for serializing and writing media assets.
	 *
	 * @param OutPackagedFile Packaged File information that must be populated. Expect OutPackagedFile data to be filled
	 *			except for the different paths.
	 * @param InMedia Media metadata that requested this packaged file.
	 */
	virtual void UpdatePackagedFile(FWwisePackagedFile& OutPackagedFile, const WwiseMetadataMedia& InMedia) const = 0;

	/**
	 * Update the Packaged File structure with all the necessary information for serializing and writing SoundBank assets.
	 *
	 * @param OutPackagedFile Packaged File information that must be populated. Expect OutPackagedFile data to be filled
	 *			except for the different paths.
	 * @param InSoundBank SoundBank metadata that requested this packaged file.
	 */
	virtual void UpdatePackagedFile(FWwisePackagedFile& OutPackagedFile, const WwiseMetadataSoundBank& InSoundBank) const = 0;

protected:
	/**
	 * This is called by Instantiate when a new Resource Cooker is instantiated for staging purposes.
	 * 
	 * @param DefaultResourceCooker The original Resource Cooker
	 */
	virtual void InitForStaging(const IWwiseResourceCooker& DefaultResourceCooker) = 0;
};