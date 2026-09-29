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

#include "Factories/Factory.h"

#include "WwisePackagingFactories.generated.h"

UCLASS(HideCategories=(Object))
class WWISEPACKAGINGEDITOR_API UWwisePackagingFactory : public UFactory
{
	GENERATED_BODY()

public:
	UWwisePackagingFactory(const FObjectInitializer& ObjectInitializer);
	
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
		FFeedbackContext* Warn) override;

	virtual uint32 GetMenuCategories() const override;
	virtual const TArray<FText>& GetMenuCategorySubMenus() const override;
};

UCLASS(HideCategories=(Object))
class WWISEPACKAGINGEDITOR_API UWwiseSharedAssetLibraryFilterFactory : public UFactory
{
	GENERATED_BODY()

public:
	UWwiseSharedAssetLibraryFilterFactory(const FObjectInitializer& ObjectInitializer);
	
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
		FFeedbackContext* Warn) override;

	virtual uint32 GetMenuCategories() const override;
	virtual const TArray<FText>& GetMenuCategorySubMenus() const override;
};
