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


#include "AkInclude.h"
#include "UObject/Object.h"
#include "Wwise/Info/WwiseObjectInfo.h"
#include "Wwise/WwiseResourceLoaderFuture.h"
#include "WwiseUnrealDefines.h"
#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Cooker/CookDependency.h"
#include "Serialization/CompactBinary.h"
#endif
#if UE_5_4_OR_LATER
#include "UObject/AssetRegistryTagsContext.h"
#endif

#include "AkAudioType.generated.h"

class FWwiseProjectDatabase;
class WwiseAnyRef;

/**
 * Wwise Audio Type abstract Unreal base class.
 *
 * All Unreal assets derive from this type.
 */
UCLASS(Abstract)
class AKAUDIO_API UAkAudioType : public UObject
{
	GENERATED_BODY()

public:
	virtual ~UAkAudioType() override;

	/// Automatically load SoundBanks and associated media associated when Unreal loads this asset.
	UPROPERTY(EditAnywhere, Category = "AkAudioType|Behaviour")
	bool bAutoLoad = true;

	// Deprecated ID properties used in migration
#if WITH_EDITORONLY_DATA
	UPROPERTY(meta=(Deprecated))
	FGuid ID_DEPRECATED;

	UPROPERTY(meta=(Deprecated))
	uint32 ShortID_DEPRECATED = 0;
#endif
	
	UPROPERTY(EditAnywhere, Category = "AkAudioType")
	TArray<TObjectPtr<UObject>> UserData;

public:
	void Serialize(FArchive& Ar) override;
	void PostLoad() override;
	void BeginDestroy() override;
	void FinishDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkAudioType")
	virtual void LoadData()   {}

	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkAudioType")
	virtual void UnloadData(bool bAsync = false) {}

	void LogSerializationState(const FArchive& Ar);
	virtual AkUInt32 GetShortID() const;

	bool IsPostLoadThreadSafe() const override { return true; }
	bool IsDestructionThreadSafe() const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkAudioType")
	int32 GetWwiseShortID() const { return GetShortID();}

	template<typename T>
	T* GetUserData()
	{
		for (auto Entry : UserData)
		{
			if (Entry && Entry->GetClass()->IsChildOf(T::StaticClass()))
			{
				return Entry;
			}
		}

		return nullptr;
	}
	
#if WITH_EDITORONLY_DATA
	virtual FWwiseObjectInfo* GetInfoMutable();
	virtual FWwiseObjectInfo GetInfo() const {return {};}
	virtual FGuid GetWwiseGuid() const { return GetInfo().WwiseGuid; }
	virtual FName GetWwiseName() const { return GetInfo().WwiseName; }
	virtual FName GetWwiseGroupName() { return {}; }
	virtual FName GetAssetDefaultName();
	virtual FName GetAssetDefaultPackagePath();
	virtual bool ObjectIsInSoundBanks() { return false; }
	virtual void BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform) override;

	// Checks whether the metadata for this UAkAudioType matches what is in the Project Database
	virtual bool IsAssetOutOfDate(const WwiseAnyRef& CurrentWwiseRef);
	virtual void FillInfo(const WwiseAnyRef& CurrentWwiseRef);
	virtual void FillInfo() {}
	virtual void FillMetadata(FWwiseProjectDatabase* ProjectDatabase) {}
	virtual void CheckWwiseObjectInfo();
	virtual void MigrateWwiseObjectInfo();
	void WaitForResourceUnloaded();
	virtual void EnsureResourceCookerCreated(const ITargetPlatform* TargetPlatform);

	template <class InfoType>
	InfoType GetValidatedInfo(const InfoType& InInfo)
	{
		InfoType TempInfo(InInfo);
		ValidateShortID(TempInfo);
		return TempInfo;
	}
	virtual void ValidateShortID(FWwiseObjectInfo& WwiseInfo) const;
#endif

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#if UE_5_4_OR_LATER 
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
#else
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
#endif // UE_5_4_OR_LATER
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
	static void HashDependenciesForCook(FCbFieldViewIterator Args, UE::Cook::FCookDependencyContext& Context);
#endif

protected:
	FWwiseResourceUnloadFuture ResourceUnload;

	static bool CanLoadObjects();
	void LoadOnceSoundEngineReady();
};
