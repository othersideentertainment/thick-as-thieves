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

#include "Wwise/WwiseProjectDatabase.h"
#include "Wwise/WwiseResourceCooker.h"

struct FWwiseAssetLibraryRef;

class WWISERESOURCECOOKER_API FWwiseResourceCookerImpl : public IWwiseResourceCooker
{
public:
	FWwiseResourceCookerImpl();
	virtual ~FWwiseResourceCookerImpl() override;

	EWwiseExportDebugNameRule ExportDebugNameRule;
	EWwisePackagingStrategy TargetPackagingStrategy;

	virtual void CookAuxBus(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookEvent(const FWwiseEventInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookExternalSource(uint32 InCookie, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookInitBank(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookMedia(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookShareSet(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookSoundBank(const FWwiseObjectInfo& InInfo, const UObject* Owner, const TCHAR* PackageFilename, const WriteAdditionalFileFunction& WriteAdditionalFile) override;

	using FCookedAssetLibraryFiles = TSet<FWwisePackagedFile>;
	virtual void PreCacheAssetLibraries(const IWwiseResourceCookerModule::FAssetLibraryInfoMap& AssetLibraryInfoMap) override;
	virtual void PreCacheAssetLibraryInfo(FCookedAssetLibraryFiles& OutFiles, UObject* Owner, const FWwiseAssetLibraryInfo& AssetLibraryInfo);
	virtual void PreCacheAssetLibraryRef(FCookedAssetLibraryFiles& OutFiles, UObject* Owner, const FWwiseAssetLibraryRef& AssetLibraryRef);
	virtual void UpdateAssetLibraryPackagedFile(FWwisePackagedFile& File);
	
	virtual void PrepareAssetLibrary(UObject* Owner, FWwiseAssetLibraryCookedData& OutCookedData, bool bPackageAssets) override;
	virtual bool PrepareCookedData(FWwiseAcousticTextureCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseAudioDeviceShareSetCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseLocalizedAuxBusCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseLocalizedEventCookedData& OutCookedData, const UObject* Owner, const FWwiseEventInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseExternalSourceCookedData& OutCookedData, uint32 InCookie) override;
	virtual bool PrepareCookedData(FWwiseGameParameterCookedData& OutCookedData, const FWwiseObjectInfo& InInf) override;
	virtual bool PrepareCookedData(FWwiseGroupValueCookedData& OutCookedData, const FWwiseGroupValueInfo& InInfo, EWwiseGroupType InGroupType) override;
	virtual bool PrepareCookedData(FWwiseInitBankCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseMediaCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseLocalizedShareSetCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseLocalizedSoundBankCookedData& OutCookedData, const UObject* Owner, const FWwiseObjectInfo& InInfo) override;
	virtual bool PrepareCookedData(FWwiseTriggerCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) override;

	virtual void PrepareResourceCookerForPlatform(FWwiseProjectDatabase*&& InProjectDatabaseOverride,
		EWwisePackagingStrategy InTargetPackagingStrategy,
		EWwiseExportDebugNameRule InExportDebugNameRule) override;

	virtual FWwiseProjectDatabase* GetProjectDatabase() override;
	virtual const FWwiseProjectDatabase* GetProjectDatabase() const override;
	virtual FWwiseResourceLoaderPtr GetResourceLoader() override;
	virtual const FWwiseResourceLoaderPtr GetResourceLoader() const override;
	virtual FWwiseSharedLanguageId GetCurrentLanguage() const override;
	virtual FWwiseSharedPlatformId GetCurrentPlatform() const override;

	virtual FString GetOwnerContentFolder(const UObject* Owner) const override;

	virtual FString GetCookRootPath(const TCHAR* PackageFilename) override;
	virtual void SetWwiseStagePath(const FString& InPath) override;
	virtual FString GetWwiseStagePath() const override;

	virtual void UpdatePackagedFile(FWwisePackagedFile& OutPackagedFile, const WwiseMetadataMedia& InMedia) const override;
	virtual void UpdatePackagedFile(FWwisePackagedFile& OutPackagedFile, const WwiseMetadataSoundBank& InSoundBank) const override;
	virtual void UpdatePackagedFileStreaming(FWwisePackagedFile& OutPackagedFile, uint32 Id) const;

protected:
	TUniquePtr<FWwiseCookingCache> CookingCache;
	FString WwiseStagePath;
	TUniquePtr<FWwiseProjectDatabase> ProjectDatabaseOverride;
	bool bCookingForPackaging{ false };
	bool bUnrealMultiprocessCooking{ false };

	virtual FWwiseCookingCache* GetCookingCache() override { return CookingCache.Get(); }

	virtual void CookLocalizedAuxBusToSandbox(const FWwiseLocalizedAuxBusCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookLocalizedSoundBankToSandbox(const FWwiseLocalizedSoundBankCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookLocalizedEventToSandbox(const FWwiseLocalizedEventCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookLocalizedShareSetToSandbox(const FWwiseLocalizedShareSetCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookAuxBusToSandbox(const FWwiseAuxBusCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookEventToSandbox(const FWwiseEventCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookExternalSourceToSandbox(const FWwiseExternalSourceCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookInitBankToSandbox(const FWwiseInitBankCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookMediaToSandbox(const FWwiseMediaCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookShareSetToSandbox(const FWwiseShareSetCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;
	virtual void CookSoundBankToSandbox(const FWwiseSoundBankCookedData& InCookedData, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;

	virtual void CookFileToSandbox(const FWwisePackagedFile& PackagedFile, const TCHAR* PackageFilename,
		const WriteAdditionalFileFunction& WriteAdditionalFile) override;

	virtual bool GetAcousticTextureCookedData(FWwiseAcousticTextureCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetAudioDeviceShareSetCookedData(FWwiseAudioDeviceShareSetCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetAuxBusCookedData(FWwiseLocalizedAuxBusCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetEventCookedData(FWwiseLocalizedEventCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseEventInfo& InInfo) const override;
	virtual bool GetExternalSourceCookedData(FWwiseExternalSourceCookedData& OutCookedData, uint32 InCookie) const override;
	virtual bool GetGameParameterCookedData(FWwiseGameParameterCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetInitBankCookedData(FWwiseInitBankCookedData& OutCookedData, const TCHAR* ContentFolderName,
		const FWwiseObjectInfo& InInfo = FWwiseObjectInfo::DefaultInitBank) const override;
	virtual bool GetMediaCookedData(FWwiseMediaCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetShareSetCookedData(FWwiseLocalizedShareSetCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetSoundBankCookedData(FWwiseLocalizedSoundBankCookedData& OutCookedData, const TCHAR* ContentFolderName, const FWwiseObjectInfo& InInfo) const override;
	virtual bool GetStateCookedData(FWwiseGroupValueCookedData& OutCookedData, const FWwiseGroupValueInfo& InInfo) const override;
	virtual bool GetSwitchCookedData(FWwiseGroupValueCookedData& OutCookedData, const FWwiseGroupValueInfo& InInfo) const override;
	virtual bool GetTriggerCookedData(FWwiseTriggerCookedData& OutCookedData, const FWwiseObjectInfo& InInfo) const override;

	virtual void InitForStaging(const IWwiseResourceCooker& DefaultResourceCooker) override;

	virtual bool FillSoundBankBaseInfo(FWwiseSoundBankCookedData& OutSoundBankCookedData,
		const WwiseMetadataPlatformInfo& InPlatformInfo,
		const WwiseMetadataSoundBank& InSoundBank,
		const TCHAR* ContentFolderName) const;
	virtual bool FillMediaBaseInfo(FWwiseMediaCookedData& OutMediaCookedData,
		const WwiseMetadataPlatformInfo& InPlatformInfo,
		const WwiseMetadataSoundBank& InSoundBank,
		const WwiseMetadataMediaReference& InMediaReference,
		const TCHAR* ContentFolderName) const;
	virtual bool FillMediaBaseInfo(FWwiseMediaCookedData& OutMediaCookedData,
		const WwiseMetadataPlatformInfo& InPlatformInfo,
		const WwiseMetadataSoundBank& InSoundBank,
		const WwiseMetadataMedia& InMedia,
		const TCHAR* ContentFolderName) const;
	virtual bool FillExternalSourceBaseInfo(FWwiseExternalSourceCookedData& OutExternalSourceCookedData,
		const WwiseMetadataExternalSource& InExternalSource) const;

	virtual bool AddRequirementsForMedia(WwiseDBSet<FWwiseSoundBankCookedData>& OutSoundBankSet, WwiseDBSet<FWwiseMediaCookedData>& OutMediaSet,
		const WwiseRefMedia& InMediaRef, const WwiseDBSharedLanguageId& InLanguage,
		const WwisePlatformDataStructure& InPlatformData,
		const TCHAR* ContentFolderName) const;
	virtual bool AddRequirementsForExternalSource(WwiseDBSet<FWwiseExternalSourceCookedData>& OutExternalSourceSet,
		const WwiseRefExternalSource& InExternalSourceRef) const;

	virtual WwiseDBObjectInfo ConvertWwiseObjectInfo(const FWwiseObjectInfo& InInfo) const;
	virtual WwiseDBGroupValueInfo ConvertWwiseGroupValueInfo(const FWwiseGroupValueInfo& InInfo) const;
};
