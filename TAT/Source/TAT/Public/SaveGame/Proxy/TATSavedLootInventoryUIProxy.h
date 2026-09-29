// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "SaveGame/TATCharacterSaveId.h"
#include "SaveGame/Proxy/TATSavedLootItemUIProxy.h"

#include "TATSavedLootInventoryUIProxy.generated.h"

class ATATPlayerState;
class UTATCharacterProgressionViewModel;

UCLASS(BlueprintType)
class TAT_API UTATSavedLootInventoryUIProxy : public UObject
{
   GENERATED_BODY()

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSavedLootStateChangeEvent);
   
public:
   void Initialize();
   void Uninitialize();

   UFUNCTION(BlueprintCallable)
   void GetLootItems(TArray<UTATSavedLootItemUIProxy*>& items) const;

   UFUNCTION(BlueprintCallable)
   void TrySellMarkedItems();

   void ClearMarkedSellItems();

   void RefreshItemsCanBeModified();

   UPROPERTY(BlueprintAssignable)
   FOnSavedLootStateChangeEvent OnSavedLootItemsChanged;

private:
   UFUNCTION()
   void _OnLocalCharacterSaveIdChanged(ATATPlayerState* playerState, FTATCharacterSaveId character);

   UTATSavedLootItemUIProxy* _GenerateProxyItemFromPool();
   void _ReleaseProxyItemIntoPool(UTATSavedLootItemUIProxy* item);

   UFUNCTION()
   void _OnSavedLootChanged();

   UPROPERTY(Transient)
   FTATCharacterSaveId _currentCharacter;

   UPROPERTY(Transient)
   TMap<FTATLootIdentifier, UTATSavedLootItemUIProxy*> _items;

   // Used in _OnSavedLootChanged() for finding loot items removed on any changed event
   // Cached here to prevent multiple construction/destructions on multiple calls to _OnSavedLootChanged()
   UPROPERTY(Transient)
   TSet<FTATLootIdentifier> _lootChangedIDs;

   UPROPERTY(Transient)
   TArray<UTATSavedLootItemUIProxy*> _itemObjectPool;

};
