// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATPrivateSpaceCharacterInterface.generated.h"

class UTATPrivateSpaceCharacterComponent;
// This class does not need to be modified.
UINTERFACE(NotBlueprintable, BlueprintType)
class UTATPrivateSpaceCharacterInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATPrivateSpaceCharacterInterface
{
   GENERATED_BODY()
public:
   UFUNCTION(BlueprintCallable)
   virtual UTATPrivateSpaceCharacterComponent* GetPrivateSpaceCharacterComponent() = 0;
   virtual bool CanBecomeSuspiciousOrIntruder() const = 0;
   virtual bool CanEverBeAllowedInPrivateArea() const = 0;
};
