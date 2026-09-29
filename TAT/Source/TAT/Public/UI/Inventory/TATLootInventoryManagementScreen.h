// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATScreenWidget.h"

#include "TATLootInventoryManagementScreen.generated.h"

class UTATSavedLootInventoryUIProxy;
class UTATSavedLootInventoryUIProxyComponent;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATLootInventoryManagementScreen : public UTATScreenWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeOnInitialized() override;

   // from UTATScreenWidget
   virtual void HandleOnScreenAddedToStack(ATATPlayerController& ownerPC) override;
   virtual void HandleOnScreenRemovedFromStack(ATATPlayerController& ownerPC) override;

   UFUNCTION(BlueprintCallable)
   const UTATSavedLootInventoryUIProxy* GetInventoryProxy() const;

private:
   TWeakObjectPtr<UTATSavedLootInventoryUIProxyComponent> _savedLootInventory;

};
