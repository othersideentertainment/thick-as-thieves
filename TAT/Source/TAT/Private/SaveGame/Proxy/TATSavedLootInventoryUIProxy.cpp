// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "SaveGame/Proxy/TATSavedLootInventoryUIProxy.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Player/TATPlayerState.h"
#include "Quests/TATThievesDenQuestSubsystem.h"
#include "SaveGame/Proxy/TATSavedLootItemUIProxy.h"
#include "SaveGame/Proxy/TATSavedLootUIProxyTransactionContext.h"
#include "SaveGame/TATCharacterProgressionViewModel.h"
#include "SaveGame/TATSaveProxySubsystem.h"
#include "SaveGame/TATSaveGame.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSavedLootInventoryUIProxy)
DEFINE_LOG_CATEGORY_STATIC(LogTATSavedLootInventoryUIProxy, Log, All);

void UTATSavedLootInventoryUIProxy::Initialize()
{
   if (ATATPlayerState* playerState = ATATPlayerState::GetLocalTATPlayerState(this))
   {
      playerState->OnCharacterSaveIdChanged.AddDynamic(this, &UTATSavedLootInventoryUIProxy::_OnLocalCharacterSaveIdChanged);

      FTATCharacterSaveId character = playerState->GetCharacterSaveId();
      if (character.IsValid())
      {
         _OnLocalCharacterSaveIdChanged(playerState, character);
      }
   }
}

void UTATSavedLootInventoryUIProxy::Uninitialize()
{
   if (ATATPlayerState* playerState = ATATPlayerState::GetLocalTATPlayerState(this))
   {
      playerState->OnCharacterSaveIdChanged.RemoveDynamic(this, &UTATSavedLootInventoryUIProxy::_OnLocalCharacterSaveIdChanged);
   }
}

void UTATSavedLootInventoryUIProxy::GetLootItems(TArray<UTATSavedLootItemUIProxy*>& items) const
{
   items.Reset(_items.Num());
   for (auto it = _items.CreateConstIterator(); it; ++it)
   {
      items.Add(it.Value());
   }
}

void UTATSavedLootInventoryUIProxy::TrySellMarkedItems()
{
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (ensure(saveGame))
   {
      FTATSavedLootSellRequest sellRequest;
      sellRequest.PopulateFrom(_items);
      TATSavedLootTransactionContext::ApplySellRequest(saveGame, _currentCharacter, sellRequest);
   }
}

void UTATSavedLootInventoryUIProxy::ClearMarkedSellItems()
{
   for (auto& it : _items)
   {
      UTATSavedLootItemUIProxy* item = it.Value;
      item->ClearSellQuantity();
   }
}

void UTATSavedLootInventoryUIProxy::RefreshItemsCanBeModified()
{
   if (const UTATThievesDenQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATThievesDenQuestSubsystem>())
   {
      for (auto& it : _items)
      {
         UTATSavedLootItemUIProxy* item = it.Value;
         item->RefreshIsModifiable(questSubsystem);
      }
   }
   else
   {
      UE_LOG(LogTATSavedLootInventoryUIProxy, Error, TEXT("RefreshItemsCanBeModified() failed to get the quest subsystem!"));
   }
}

void UTATSavedLootInventoryUIProxy::_OnLocalCharacterSaveIdChanged(ATATPlayerState* playerState, FTATCharacterSaveId character)
{
   check(playerState);
   check(character.IsValid());

   if (_currentCharacter == character)
   {
      UE_LOG(LogTATSavedLootInventoryUIProxy, Warning, TEXT("_OnLocalCharacterSaveIdChanged() called for a character already loaded!"));
      return;
   }

   _currentCharacter = character;

   UTATSaveProxySubsystem* saveProxySubsystem = GetWorld()->GetSubsystem<UTATSaveProxySubsystem>();
   check(saveProxySubsystem);

   UTATCharacterProgressionViewModel* proxy = saveProxySubsystem->GetCharacterProgressionProxy(character);
   check(proxy);

   proxy->OnLootChanged.AddUniqueDynamic(this, &UTATSavedLootInventoryUIProxy::_OnSavedLootChanged);

   _OnSavedLootChanged();
   RefreshItemsCanBeModified();
}

UTATSavedLootItemUIProxy* UTATSavedLootInventoryUIProxy::_GenerateProxyItemFromPool()
{
   if (!_itemObjectPool.IsEmpty())
   {
      UTATSavedLootItemUIProxy* poolItem = _itemObjectPool.Last();
      check(IsValid(poolItem));
      _itemObjectPool.RemoveAtSwap(_itemObjectPool.Num() - 1);
      return poolItem;
   }

   return NewObject<UTATSavedLootItemUIProxy>(this);
}

void UTATSavedLootInventoryUIProxy::_ReleaseProxyItemIntoPool(UTATSavedLootItemUIProxy* item)
{
   check(item);
   item->Reset();
   _itemObjectPool.Add(item);
}

void UTATSavedLootInventoryUIProxy::_OnSavedLootChanged()
{
   UTATSaveProxySubsystem* saveProxySubsystem = GetWorld()->GetSubsystem<UTATSaveProxySubsystem>();
   check(saveProxySubsystem);

   UTATCharacterProgressionViewModel* proxy = saveProxySubsystem->GetCharacterProgressionProxy(_currentCharacter);
   check(proxy);

   // Generate a set of all the loot items we're currently tracking. As we iterate through
   // the list of saved loot, remove IDs from this set. At the end, we'll be left with
   // all loot items that were removed from saved loot.
   _items.GetKeys(_lootChangedIDs);

   const UTATLootSettings& lootSettings = UTATLootSettings::Get();
   const FTATSavedLootInventory& savedLoot = proxy->GetLoot();
   for (const FTATLootCountPair& it : savedLoot.GetStacks())
   {
      const FTATSavedLootStack stack(it.Key, it.Value);

      const FTATLootIdentifier proxyID = stack.LootIdentifier;
      _lootChangedIDs.Remove(proxyID);

      UTATSavedLootItemUIProxy* lootProxy = _items.FindRef(proxyID);
      if (lootProxy == nullptr)
      {
         const FTATLootInfo* lootInfo = lootSettings.GetLootInfo(this, stack.LootIdentifier);
         if (!ensure(lootInfo))
         {
            continue;
         }
         lootProxy = _GenerateProxyItemFromPool();
         check(lootProxy);
         lootProxy->BuildFromLootStack(stack, *lootInfo);
         _items.Add(lootProxy->GetID(), lootProxy);
      }
      else
      {
         lootProxy->UpdateLootStackState(stack);
         lootProxy->ClearSellQuantity();
      }
   }

   // Remove the remaining proxy items that were not found in the saved loot inventory
   for (auto it = _lootChangedIDs.CreateIterator(); it; ++it)
   {
      UTATSavedLootItemUIProxy* item = _items.FindChecked(*it);
      _items.Remove(item->GetID());
      _ReleaseProxyItemIntoPool(item);
   }

   OnSavedLootItemsChanged.Broadcast();
   _lootChangedIDs.Reset();
}
