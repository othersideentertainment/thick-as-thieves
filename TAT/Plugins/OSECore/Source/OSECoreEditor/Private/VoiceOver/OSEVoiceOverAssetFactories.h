// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Factories/Factory.h"

#include "OSEVoiceOverAssetFactories.generated.h"


UCLASS(hidecategories = Object)
class UOSEVoiceOverLineFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(hidecategories = Object)
class UOSEVoiceOverConversationFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(hidecategories = Object)
class UOSEVoiceOverBucketFactory : public UFactory
{
   GENERATED_UCLASS_BODY()

public:
   virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
