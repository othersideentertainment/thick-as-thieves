// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATItemInventorySystemInterface.generated.h"

class UTATItemInventoryComponent;

// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI, Category = "Inventory", meta = (CannotImplementInterfaceInBlueprint))
class UTATItemInventorySystemInterface : public UInterface
{
   GENERATED_BODY()
};

/// TATItemInventory System Interface
/// This is modeled after the ability system interface. The sole purpose is to ensure that the
/// TATItemInventoryComponent is accessible somehow. The actual component can live on a different
/// object than the one that implements this interface.
class TAT_API ITATItemInventorySystemInterface
{
   GENERATED_BODY()

public:

   /// Returns the TATItemInventoryComponent. It may live on another object.
   UFUNCTION(BlueprintCallable, Category = "Inventory")
   virtual UTATItemInventoryComponent* GetTATItemInventory() const = 0;

};
