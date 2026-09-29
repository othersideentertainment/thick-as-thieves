// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "SaveGame/TATSavedLoot.h"
#include "SaveGame/Proxy/TATSavedLootItemUIProxy.h"

struct FTATCharacterSaveId;
class UTATSaveGame;

struct FTATSavedLootSellRequest
{
   TMap<FTATLootIdentifier, int32> ItemsToSell;

   void PopulateFrom(const TMap<FTATLootIdentifier, UTATSavedLootItemUIProxy*>& items);
};

namespace TATSavedLootTransactionContext
{
   bool ApplySellRequest(UTATSaveGame* saveGame, const FTATCharacterSaveId& character, const FTATSavedLootSellRequest& sellRequest);

   int32 FindSellLootValue(const FTATSavedLootSellRequest& sellRequest, const UObject* contextObject);

   FTATSavedLootRemoveRequest BuildLootRemoveRequest(const FTATSavedLootSellRequest& sellRequest);
};
