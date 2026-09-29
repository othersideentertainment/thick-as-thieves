// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "SaveGame/Proxy/TATSavedLootInventoryUIProxyComponent.h"

// tat
#include "Player/TATPlayerState.h"
#include "SaveGame/TATCharacterProgressionViewModel.h"
#include "SaveGame/TATSaveProxySubsystem.h"
#include "SaveGame/Proxy/TATSavedLootInventoryUIProxy.h"
#include "SaveGame/Proxy/TATSavedLootItemUIProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSavedLootInventoryUIProxyComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATSavedLootInventoryUIProxyComponent, Log, All);

// Sets default values for this component's properties
UTATSavedLootInventoryUIProxyComponent::UTATSavedLootInventoryUIProxyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTATSavedLootInventoryUIProxyComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (_inventoryProxy != nullptr)
   {
      _inventoryProxy->Uninitialize();
      _inventoryProxy = nullptr;
   }

   Super::EndPlay(endPlayReason);
}

void UTATSavedLootInventoryUIProxyComponent::TryRefreshItemsCanBeModified()
{
   if (_inventoryProxy != nullptr)
   {
      _inventoryProxy->RefreshItemsCanBeModified();
   }
}

void UTATSavedLootInventoryUIProxyComponent::TryClearItemModifications()
{
   if (_inventoryProxy != nullptr)
   {
      _inventoryProxy->ClearMarkedSellItems();
   }
}

UTATSavedLootInventoryUIProxy* UTATSavedLootInventoryUIProxyComponent::GetInventoryProxy()
{
   if (_inventoryProxy == nullptr)
   {
      _inventoryProxy = NewObject<UTATSavedLootInventoryUIProxy>(this);
      _inventoryProxy->Initialize();
   }

   return _inventoryProxy;
}
