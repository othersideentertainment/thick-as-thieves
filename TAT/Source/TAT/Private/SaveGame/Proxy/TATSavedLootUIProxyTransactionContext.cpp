// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "SaveGame/Proxy/TATSavedLootUIProxyTransactionContext.h"

// tat
#include "Developer/TATLootSettings.h"
#include "SaveGame/TATSavedLoot.h"
#include "SaveGame/TATSaveGame.h"

DEFINE_LOG_CATEGORY_STATIC(LogTATSavedLootTransactionContext, Log, All);

void FTATSavedLootSellRequest::PopulateFrom(const TMap<FTATLootIdentifier, UTATSavedLootItemUIProxy*>& items)
{
   for (const auto& it : items)
   {
      const UTATSavedLootItemUIProxy* item = it.Value;
      const int32 quantity = item->GetSellQuantity();
      if (quantity > 0)
      {
         const FTATLootIdentifier& id = it.Key;
         ItemsToSell.Add(id, quantity);
      }
   }
}

bool TATSavedLootTransactionContext::ApplySellRequest(UTATSaveGame* saveGame, const FTATCharacterSaveId& character, const FTATSavedLootSellRequest& sellRequest)
{
   check(saveGame);

   const int32 totalSellValue = FindSellLootValue(sellRequest, saveGame);
   const FTATSavedLootRemoveRequest lootRemoveRequest = BuildLootRemoveRequest(sellRequest);
   if (saveGame->RemoveLoot(character, lootRemoveRequest))
   {
      saveGame->UpdateMoney(totalSellValue);
      return true;
   }
   else
   {
      UE_LOG(LogTATSavedLootTransactionContext, Error, 
         TEXT("[TATSavedLootTransactionContext::ApplySellRequest] Loot removal failed! Sell request transaction could not be completed!"));
      return false;
   }
}

int32 TATSavedLootTransactionContext::FindSellLootValue(const FTATSavedLootSellRequest& sellRequest, const UObject* contextObject)
{
   int32 sellValue = 0;
   for (const auto& it : sellRequest.ItemsToSell)
   {
      const FTATLootIdentifier& proxyID = it.Key;
      const int32 count = it.Value;
      ensure(count > 0);
      const int32 value = UTATLootSettings::Get().GetLootValue(contextObject, proxyID) * count;
      ensure(value >= 0);

      sellValue += value;
   }
   return sellValue;
}

FTATSavedLootRemoveRequest TATSavedLootTransactionContext::BuildLootRemoveRequest(const FTATSavedLootSellRequest& sellRequest)
{
   FTATSavedLootRemoveRequest removeRequest;
   for (const auto& it : sellRequest.ItemsToSell)
   {
      const FTATLootIdentifier& proxyID = it.Key;
      const int32 count = it.Value;
      removeRequest.Stacks.Add(proxyID, count);
   }
   return removeRequest;
}
