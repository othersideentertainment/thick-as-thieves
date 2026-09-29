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
#include "Misc/CoreMisc.h"

#include "Wwise/Packaging/WwiseFilterableAssetLibrary.h"

#if !WITH_EDITORONLY_DATA
#include "Wwise/CookedData/WwiseAssetLibraryCookedData.h"
#endif

#include "Wwise/WwiseResourceLoaderFuture.h"
#include "Wwise/Loaded/WwiseLoadedAssetLibrary.h"

#include "WwiseAssetLibrary.generated.h"

namespace UE::Cook
{
	struct FCookDependencyContext;
}
class FCbFieldViewIterator;

/**
 * A group of Wwise Assets selected according to filter criteria.
 */
UCLASS(BlueprintType, Category="Wwise", DisplayName="Wwise Asset Library", EditInlineNew)
class WWISEPACKAGING_API UWwiseAssetLibrary : public UWwiseFilterableAssetLibrary
{
	GENERATED_BODY()

public:
	/// Automatically load SoundBanks and associated media associated when Unreal loads this asset.
	UPROPERTY(EditAnywhere, Category = "Default", AdvancedDisplay)
	bool bAutoLoad{ true };


	UWwiseAssetLibrary() {}
	virtual ~UWwiseAssetLibrary() override;

#if WITH_EDITORONLY_DATA 
	virtual void BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform);
#endif
	
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
	virtual void FinishDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkAudioType")
	virtual void LoadData();

	UFUNCTION(BlueprintCallable, Category = "Audiokinetic|AkAudioType")
	virtual void UnloadData(bool bAsync = false);

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	static void HashDependenciesForCook(FCbFieldViewIterator Args, UE::Cook::FCookDependencyContext& Context);
#endif

#if WITH_EDITORONLY_DATA
	/// When selected, assets in this library can be found by other asset libraries as well.
	UPROPERTY(EditAnywhere, Category = "Default", AdvancedDisplay)
	bool bFallthrough{ false };

	/// Package the assets found according to the filter criteria. When this option is cleared, this library is only used for filtering.
	UPROPERTY(EditAnywhere, Category = "Default", AdvancedDisplay)
	bool bPackageAssets{ true };
#endif

#if !WITH_EDITORONLY_DATA
	FWwiseLoadedAssetLibraryPtrAtomic LoadedAssetLibrary{ nullptr };
	FWwiseAssetLibraryCookedData CookedData;
#endif

protected:
	FWwiseResourceUnloadFuture ResourceUnload;
};
