// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"

#include "TATLootInterface.generated.h"

class UTATLootInventoryComponent;

//---------------------------------------------------------------------------------------
// ITATLootInventoryInterface
//---------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, MinimalAPI, Category = "Inventory", meta = (CannotImplementInterfaceInBlueprint))
class UTATLootInventoryInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATLootInventoryInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "Loot")
   virtual UTATLootInventoryComponent* GetLootInventoryComponent() const = 0;
};
