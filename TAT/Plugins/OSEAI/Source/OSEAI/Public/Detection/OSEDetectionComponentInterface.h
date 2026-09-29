// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OSEDetectionComponentInterface.generated.h"

class UOSEDetectionComponent;
// This class does not need to be modified.

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOSEDetectionComponentInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IOSEDetectionComponentInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   virtual UOSEDetectionComponent* GetDetectionComponent() const = 0;
};
