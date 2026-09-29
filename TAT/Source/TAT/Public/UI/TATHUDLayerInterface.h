// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATHUDLayerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class TAT_API UTATHUDLayerInterface : public UInterface
{
   GENERATED_BODY()
};

/**
 *
 */
class TAT_API ITATHUDLayerInterface
{
   GENERATED_BODY()

      // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "TAT|UI")
   void OnShow();

   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "TAT|UI")
   void OnHide();

};
