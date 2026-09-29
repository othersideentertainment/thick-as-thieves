// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Factories/Factory.h"

#include "TATUpgradeGraphFactories.generated.h"


UCLASS()
class UTATUpgradeGraphFactory : public UFactory
{
   GENERATED_BODY()

public:
   UTATUpgradeGraphFactory();
   virtual UObject* FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn) override;
   virtual FText GetDisplayName() const override { return FText::FromString(FName::NameToDisplayString(SupportedClass->GetName(), false)); }
   virtual FString GetDefaultNewAssetName() const override { return SupportedClass->GetName(); }
};

UCLASS(hidecategories = Object)
class UTATUpgradeTypeFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
