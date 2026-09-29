// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Tools/TATToolComponent.h"

// ue5
#include "CoreMinimal.h"

#include "TATLootInventoryToolComponent.generated.h"

class UTATLootInventoryComponent;

/// Tool that represents an entire loot inventory
UCLASS()
class TAT_API UTATLootInventoryToolComponent : public UTATToolComponent
{
   GENERATED_BODY()

public:
   UTATLootInventoryToolComponent();

   UFUNCTION(BlueprintPure, Category = "Loot Inventory Tool Component")
   UTATLootInventoryComponent* GetOwningInventory() const;

protected:
   /// IToolInterface (protected)
   virtual bool OnAddToToolSet_Implementation() override;

protected:
   UPROPERTY(EditDefaultsOnly)
   bool _automaticallyEquipOnAddToToolset = false;
};
