// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

// 5/31/2024: This factory class allows creating graph types defined entirely in blueprints, which we don't want to support at this time
#if 0

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "OSEGenericGraph.h"
#include "OSEGenericGraphFactory.generated.h"

UCLASS()
class OSEGENERICGRAPHEDITOR_API UOSEGenericGraphFactory : public UFactory
{
   GENERATED_BODY()

public:
   UOSEGenericGraphFactory();
   virtual ~UOSEGenericGraphFactory();

   UPROPERTY(EditAnywhere, Category=DataAsset)
   TSubclassOf<UOSEGenericGraph> GenericGraphClass;

   virtual bool ConfigureProperties() override;
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

#endif
