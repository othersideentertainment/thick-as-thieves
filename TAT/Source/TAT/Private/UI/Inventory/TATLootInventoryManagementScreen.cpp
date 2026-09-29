// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Inventory/TATLootInventoryManagementScreen.h"

// tat
#include "Player/TATPlayerState.h"
#include "SaveGame/TATCharacterProgressionViewModel.h"
#include "SaveGame/TATSaveProxySubsystem.h"
#include "SaveGame/Proxy/TATSavedLootInventoryUIProxyComponent.h"
#include "TATGameInstance.h"
#include "ThievesDen/TATThievesDenPlayerController.h"
#include "UI/Inventory/TATLootInventoryListEntryWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootInventoryManagementScreen)
DEFINE_LOG_CATEGORY_STATIC(LogTATLootInventoryManagementScreen, Log, All);

void UTATLootInventoryManagementScreen::NativeOnInitialized()
{
   Super::NativeOnInitialized();

   for (ULocalPlayer* localPlayer : UTATGameInstance::Get(this).GetLocalPlayers())
   {
      if (ATATThievesDenPlayerController* pc = Cast<ATATThievesDenPlayerController>(localPlayer->PlayerController))
      {
         _savedLootInventory = pc->GetSavedLootInventoryUIProxyComponent();
      }
   }
}

void UTATLootInventoryManagementScreen::HandleOnScreenAddedToStack(ATATPlayerController& ownerPC)
{
   // refresh data before calling super which will call blueprint methods which rely
   // on this data being set
   check(_savedLootInventory.IsValid());
   _savedLootInventory->TryRefreshItemsCanBeModified();

   Super::HandleOnScreenAddedToStack(ownerPC);
}

void UTATLootInventoryManagementScreen::HandleOnScreenRemovedFromStack(ATATPlayerController& ownerPC)
{
   Super::HandleOnScreenRemovedFromStack(ownerPC);

   if (_savedLootInventory.IsValid())
   {
      _savedLootInventory->TryClearItemModifications();
   }
}

const UTATSavedLootInventoryUIProxy* UTATLootInventoryManagementScreen::GetInventoryProxy() const
{
   UTATSavedLootInventoryUIProxyComponent* inventoryComponent = _savedLootInventory.Get();
   return (inventoryComponent != nullptr) ? inventoryComponent->GetInventoryProxy() : nullptr;
}
