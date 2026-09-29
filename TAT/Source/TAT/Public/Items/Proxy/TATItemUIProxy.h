// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

// tat
#include "Items/TATInventoryTypes.h"
#include "Character/TATCharacterMetadata.h"

// ue4

#include "TATItemUIProxy.generated.h"

class UPaperSprite;
class UTATItemInfo;

/// A proxy object for a stack of items for use by the UI
UCLASS(Blueprintable)
class TAT_API UTATItemUIProxy : public UObject
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintReadOnly)
   TSubclassOf<UTATItemInfo> ItemInfo;

   UPROPERTY(BlueprintReadOnly)
   int32 Amount;

   /// A non-persistent item id identifying this stack. Its meaning depends on the context;
   UPROPERTY(BlueprintReadOnly)
   FInventoryStackId StackId;

   /// The inventory bucket the item is in, only has meaning in an inventory
   UPROPERTY(BlueprintReadOnly)
   EInventoryType InventoryBucket;

   /// The character type holding this item. Only has meaning in the Stash
   UPROPERTY(BlueprintReadOnly)
   ETATCharacter Character;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemAmountChanged);
   UPROPERTY(BlueprintAssignable, Category = "Items")
   FOnItemAmountChanged OnAmountChanged;

public:
   void InitForHub(TSubclassOf<UTATItemInfo> itemInfo, FInventoryStackId id, ETATCharacter character);
   void InitForMission(TSubclassOf<UTATItemInfo> itemInfo, FInventoryStackId id, EInventoryType bucket);
   void SetAmount(int32 newAmount);

   UFUNCTION(BlueprintPure)
   UPaperSprite* GetIcon() const;
   UFUNCTION(BlueprintPure)
   FText GetName() const;
   UFUNCTION(BlueprintPure)
   FText GetDescription() const;
   UFUNCTION(BlueprintPure)
   int GetGoldValue() const;
   UFUNCTION(BlueprintPure)
   UTATItemInfo* GetInfoCDO() const;

   // TODO: refactor for Hub?
   UFUNCTION(BlueprintPure)
   bool CanUse() const;

   UFUNCTION(BlueprintPure)
   bool CanDrop() const;

   UFUNCTION(BlueprintPure)
   bool CanEverEquip() const;
};
