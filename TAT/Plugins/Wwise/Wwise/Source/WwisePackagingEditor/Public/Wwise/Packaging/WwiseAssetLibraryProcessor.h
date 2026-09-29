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
#include "Wwise/WwisePackagingEditorModule.h"

class WwiseRefMedia;
class WwiseRefSoundBank;
class WwiseAnyRef;
class FWwiseProjectDatabase;
struct FWwiseAssetLibraryInfo;
struct FWwiseAssetLibraryRef;
struct FWwiseAssetLibraryFilteringSharedData;

class WWISEPACKAGINGEDITOR_API FWwiseAssetLibraryProcessor
{
public:
	inline static FWwiseAssetLibraryProcessor* Get()
	{
		if (auto* Module = IWwisePackagingEditorModule::GetModule())
		{
			return Module->GetAssetLibraryProcessor();
		}
		return nullptr;
	}

	FWwiseAssetLibraryProcessor() {}
	virtual ~FWwiseAssetLibraryProcessor() {}

	virtual FWwiseAssetLibraryFilteringSharedData* InstantiateSharedData(FWwiseProjectDatabase& ProjectDatabase);

	virtual void RetrieveAssetMap(FWwiseAssetLibraryFilteringSharedData& Shared);
	virtual void RetrieveMediaMap(FWwiseAssetLibraryFilteringSharedData& Shared);
	virtual void RetrieveSoundBankMap(FWwiseAssetLibraryFilteringSharedData& Shared);
	virtual bool FilterMedia(const WwiseRefMedia& Media);
	virtual bool FilterSoundBank(const WwiseRefSoundBank& SoundBank);
	
	virtual void FilterLibraryAssets(FWwiseAssetLibraryFilteringSharedData& Shared, FWwiseAssetLibraryInfo& Library, bool bUpdateRemaining = true, bool bUpdateFilteredAssets = true);
	virtual bool FilterAsset(const FWwiseAssetLibraryFilteringSharedData& Shared, const FWwiseAssetLibraryInfo& Library, const WwiseAnyRef& Item);

	virtual bool CreateAssetLibraryRef(FWwiseAssetLibraryRef&, const WwiseAnyRef& SourceRef);

protected:
	static FCriticalSection IsFilteringCrit;
};