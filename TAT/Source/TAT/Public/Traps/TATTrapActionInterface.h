// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATTrapActionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTATTrapActionInterface : public UInterface
{
   GENERATED_BODY()
};


class TAT_API ITATTrapActionInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
   void TriggerActionFromTrap(AActor* optionalTarget);
   virtual void TriggerActionFromTrap_Implementation(AActor* optionalTarget) = 0;
};
