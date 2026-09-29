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

#include "Wwise/Packaging/WwiseAssetLibrary.h"

#include "Wwise/WwisePackagingModule.h"
#include "Wwise/Stats/Packaging.h"

#if !UE_SERVER && WITH_EDITORONLY_DATA 
#include "Wwise/WwiseProjectDatabaseModule.h"
#include "Wwise/WwiseResourceCooker.h"
#include "Wwise/CookedData/WwiseAssetLibraryCookedData.h"

#include "Interfaces/ITargetPlatform.h"
#else
#include "Wwise/WwiseResourceLoader.h"
#endif

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
#include "Cooker/CookDependency.h"
#include "UObject/ObjectSaveContext.h"
#include "Serialization/CompactBinaryWriter.h"
#endif

UWwiseAssetLibrary::~UWwiseAssetLibrary()
{
	SCOPED_WWISEPACKAGING_EVENT_3(TEXT("UWwiseAssetLibrary Dtor"));
	ResourceUnload.Wait();
}

#if WITH_EDITORONLY_DATA 
void UWwiseAssetLibrary::BeginCacheForCookedPlatformData(const ITargetPlatform* TargetPlatform)
{
	UObject::BeginCacheForCookedPlatformData(TargetPlatform);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	IWwisePackagingModule::CreateResourceCookerForPlatform(TargetPlatform);
}
#endif

void UWwiseAssetLibrary::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

#if !UE_SERVER
#if WITH_EDITORONLY_DATA 
	if (Ar.IsCooking() && Ar.IsSaving() && !Ar.CookingTarget()->IsServerOnly())
	{
		FWwiseAssetLibraryCookedData PlatformCookedData;
		{
			if (Ar.CookingTarget() != nullptr)
			{
				IWwisePackagingModule::CreateResourceCookerForPlatform(Ar.CookingTarget());
			}
			if (auto* ResourceCooker = IWwiseResourceCooker::GetForArchive(Ar))
			{
				ResourceCooker->PrepareAssetLibrary(this, PlatformCookedData, bPackageAssets);
			}
			else
			{
				UE_LOG(LogWwisePackaging, Error, TEXT("Could not retrieve Resource Cooker for Archive. Archive will be empty."));
			}
		}
		PlatformCookedData.Serialize(Ar);

		FWwisePackagedFileSerializationOptions Options(this);
		Options.bOptional = true;		// We want to load the Bulk Data structure, but not assets in themselves. 
		
		PlatformCookedData.SerializeBulkData(Ar, Options);
	}
#else
	FWwisePackagedFileSerializationOptions Options(this);
	Options.bOptional = true;		// We want to load the Bulk Data structure, but not assets in themselves. 

	CookedData.Serialize(Ar);
	CookedData.SerializeBulkData(Ar, Options);
#endif
#endif
}

void UWwiseAssetLibrary::PostLoad()
{
	UObject::PostLoad();

	if (LIKELY(bAutoLoad))
	{
		LoadData();
	}
}

void UWwiseAssetLibrary::BeginDestroy()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return Super::BeginDestroy();
	}

	{
		SCOPED_WWISEPACKAGING_EVENT_F_2(TEXT("UWwiseAssetLibrary::BeginDestroy %s"), *GetClass()->GetName());
		UE_LOG(LogWwisePackaging, Verbose, TEXT("UWwiseAssetLibrary::BeginDestroy[%p] %s %s"), this, *GetClass()->GetName(), *GetName());

		UnloadData(true);
	}
	Super::BeginDestroy();
}

void UWwiseAssetLibrary::FinishDestroy()
{
	{
		SCOPED_WWISEPACKAGING_EVENT_2(TEXT("UWwiseAssetLibrary::FinishDestroy"));
		UE_LOG(LogWwisePackaging, VeryVerbose, TEXT("UWwiseAssetLibrary::FinishDestroy[%p]"), this);

		ResourceUnload.Wait();
	}
	Super::FinishDestroy();
}

void UWwiseAssetLibrary::LoadData()
{
#if !WITH_EDITORONLY_DATA
	SCOPED_WWISEPACKAGING_EVENT_2(TEXT("UWwiseAssetLibrary::LoadData"));
	FWwiseResourceLoaderPtr ResourceLoader = FWwiseResourceLoader::Get();
	if (UNLIKELY(!ResourceLoader))
	{
		return;
	}

	UnloadData(false);

	const auto NewlyLoadedAssetLibrary = ResourceLoader->LoadAssetLibrary(CookedData);
	auto PreviouslyLoadedAssetLibrary = LoadedAssetLibrary.exchange(NewlyLoadedAssetLibrary);
	if (UNLIKELY(PreviouslyLoadedAssetLibrary))
	{
		ResourceLoader->UnloadAssetLibrary(MoveTemp(PreviouslyLoadedAssetLibrary));
	}
#endif
}

void UWwiseAssetLibrary::UnloadData([[maybe_unused]] bool bAsync)
{
#if !WITH_EDITORONLY_DATA
	SCOPED_WWISEPACKAGING_EVENT_2(TEXT("UWwiseAssetLibrary::UnloadData"));
	auto PreviouslyLoadedAssetLibrary = LoadedAssetLibrary.exchange(nullptr);
	if (PreviouslyLoadedAssetLibrary)
	{
		FWwiseResourceLoaderPtr ResourceLoader = FWwiseResourceLoader::Get();
		if (UNLIKELY(!ResourceLoader))
		{
			return;
		}

		if (bAsync)
		{
			FWwiseLoadedAssetLibraryPromise Promise;
			Promise.EmplaceValue(MoveTemp(PreviouslyLoadedAssetLibrary));
			ResourceUnload = ResourceLoader->UnloadAssetLibraryAsync(Promise.GetFuture());
		}
		else
		{
			ResourceLoader->UnloadAssetLibrary(MoveTemp(PreviouslyLoadedAssetLibrary));
		}
	}
#endif
}

#if WITH_EDITORONLY_DATA && UE_5_5_OR_LATER
UE_COOK_DEPENDENCY_FUNCTION(HashWwiseAssetLibraryDependenciesForCook, UWwiseAssetLibrary::HashDependenciesForCook);

void UWwiseAssetLibrary::PreSave(FObjectPreSaveContext SaveContext)
{
	ON_SCOPE_EXIT
	{
		Super::PreSave(SaveContext);
	};

	if (!SaveContext.IsCooking())
	{
		return;
	}

	auto* ResourceCooker = IWwiseResourceCooker::GetForPlatform(SaveContext.GetTargetPlatform());
	if (UNLIKELY(!ResourceCooker))
	{
		return;
	}

	FWwiseAssetLibraryCookedData CookedDataToArchive;
	ResourceCooker->PrepareAssetLibrary(this, CookedDataToArchive, bPackageAssets);

	FCbWriter Writer;
	Writer.BeginObject();
	CookedDataToArchive.PreSave(SaveContext, Writer);
	Writer.EndObject();
	
	SaveContext.AddCookBuildDependency(
		UE::Cook::FCookDependency::Function(
			UE_COOK_DEPENDENCY_FUNCTION_CALL(HashWwiseAssetLibraryDependenciesForCook), Writer.Save()));
}

void UWwiseAssetLibrary::HashDependenciesForCook(FCbFieldViewIterator Args, UE::Cook::FCookDependencyContext& Context)
{
	TArray<uint8> Memory;
	Memory.AddUninitialized(Args.GetSize());
	FMutableMemoryView MemoryView(Memory.GetData(), Memory.Num());
	Args.CopyTo(MemoryView);
	Context.Update(Memory.GetData(), Memory.Num());
}
#endif
