// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATGameplayBehavior_CollectMajorLoot.h"

// tat
#include "Loot/TATLootInventory.h"
#include "Developer/TATLootSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayBehavior_CollectMajorLoot)

void UTATGameplayBehavior_CollectMajorLoot::CollectMajorLoot(ATATLootActor* lootActor,
                                                               TScriptInterface<ITATLootInventoryInterface> lootInventoryInterface)
{
   UTATLootInventoryComponent* inventoryComp = lootInventoryInterface->GetLootInventoryComponent();
   check(inventoryComp != nullptr);
   FPredictionKey predictionKey;
   inventoryComp->HandlePickupLootItem(lootActor->GetLootIdentifier(), lootActor, predictionKey);
}
