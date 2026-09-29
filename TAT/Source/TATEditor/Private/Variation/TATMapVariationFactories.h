// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Factories/Factory.h"

#include "TATMapVariationFactories.generated.h"


UCLASS(hidecategories = Object)
class UTATSpawnDataAssetFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(hidecategories = Object)
class UTATSceneVariantConfigFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(hidecategories = Object)
class UTATSceneAssetFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};


UCLASS(hidecategories = Object)
class UTATSceneSetFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(hidecategories = Object)
class UTATClueSetFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
   virtual FString GetDefaultNewAssetName() const override;
};

UCLASS(hidecategories = Object)
class UTATCompoundClueSetFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
   virtual FString GetDefaultNewAssetName() const override;
};

UCLASS(hidecategories = Object)
class UTATMatchQuestDescriptionFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
   virtual FString GetDefaultNewAssetName() const override;
};
