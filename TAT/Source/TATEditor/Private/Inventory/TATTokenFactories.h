// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Factories/Factory.h"

#include "TATTokenFactories.generated.h"


UCLASS()
class TATEDITOR_API UTATInventoryTokenFactory : public UFactory
{
   GENERATED_BODY()
public:
   UTATInventoryTokenFactory();
   virtual UObject* FactoryCreateNew(UClass* cls, UObject* parent, FName name, EObjectFlags flags, UObject* context, FFeedbackContext* warn) override;
   virtual FString GetDefaultNewAssetName() const override { return TEXT("TK_NewToken"); }
};
