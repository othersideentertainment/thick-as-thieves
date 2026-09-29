// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Factories/Factory.h"

#include "TATModularQuestGraphFactories.generated.h"


UCLASS()
class UTATModularQuestGraphModuleFactory : public UFactory
{
   GENERATED_BODY()

public:
   UTATModularQuestGraphModuleFactory();
   virtual UObject* FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn) override;
   virtual FText GetDisplayName() const override { return FText::FromString(FName::NameToDisplayString(SupportedClass->GetName(), false)); }
   virtual FString GetDefaultNewAssetName() const override { return SupportedClass->GetName(); }
   virtual uint32 GetMenuCategories() const override;
};


UCLASS()
class UTATModularQuestGraphFactory : public UFactory
{
   GENERATED_BODY()

public:
   UTATModularQuestGraphFactory();
   virtual UObject* FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn) override;
   virtual FText GetDisplayName() const override { return FText::FromString(FName::NameToDisplayString(SupportedClass->GetName(), false)); }
   virtual FString GetDefaultNewAssetName() const override { return SupportedClass->GetName(); }
   virtual uint32 GetMenuCategories() const override;
};
