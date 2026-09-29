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

#include "AssetTypeActions_Base.h"

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_Base_WwisePackaging : public FAssetTypeActions_Base
{
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_WwiseAssetLibrary : public FAssetTypeActions_Base_WwisePackaging
{
public:
    virtual FText GetName() const override;
    virtual FColor GetTypeColor() const override;
	virtual const TArray<FText>& GetSubMenus() const override;
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_Base_WwiseAssetLibraryFilter : public FAssetTypeActions_Base_WwisePackaging
{
	virtual FColor GetTypeColor() const override;
	virtual const TArray<FText>& GetSubMenus() const override;
	virtual uint32 GetCategories() override;
};

class WWISEPACKAGINGEDITOR_API FAssetTypeActions_WwiseSharedAssetLibraryFilter : public FAssetTypeActions_Base_WwisePackaging
{
    virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual const TArray<FText>& GetSubMenus() const override;
    virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
