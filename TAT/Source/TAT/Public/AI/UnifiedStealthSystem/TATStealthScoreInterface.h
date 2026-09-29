// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATStealthScoreInterface.generated.h"

class UTATStealthScoreComponent;
// This class does not need to be modified.
UINTERFACE(BlueprintType, NotBlueprintable)
class UTATStealthScoreInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATStealthScoreInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   virtual float GetStealthScore() const = 0; 
   virtual UTATStealthScoreComponent* GetStealthScoreComponent() const = 0;
};
