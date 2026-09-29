// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATSavedLootInventoryUIProxyComponent.generated.h"

class UTATSavedLootInventoryUIProxy;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATSavedLootInventoryUIProxyComponent : public UActorComponent
{
   GENERATED_BODY()

   UTATSavedLootInventoryUIProxyComponent();

public:
   UFUNCTION(BlueprintCallable)
   UTATSavedLootInventoryUIProxy* GetInventoryProxy();

   void TryRefreshItemsCanBeModified();
   void TryClearItemModifications();

private:
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   UPROPERTY(Transient)
   UTATSavedLootInventoryUIProxy* _inventoryProxy = nullptr;
		
};
