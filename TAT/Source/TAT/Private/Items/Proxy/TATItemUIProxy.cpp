// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Proxy/TATItemUIProxy.h"

// tat
#include "Items/TATItemInfo.h"

// ue4
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATItemUIProxy)

void UTATItemUIProxy::InitForHub(TSubclassOf<UTATItemInfo> itemInfo, FInventoryStackId id, ETATCharacter character)
{
   ItemInfo = itemInfo;
   StackId = id;
   Character = character;
}

void UTATItemUIProxy::InitForMission(TSubclassOf<UTATItemInfo> itemInfo, FInventoryStackId id, EInventoryType bucket)
{
   ItemInfo = itemInfo;
   StackId = id;
   InventoryBucket = bucket;
}

void UTATItemUIProxy::SetAmount(int32 newAmount)
{
   if (newAmount != Amount)
   {
      Amount = newAmount;
      OnAmountChanged.Broadcast();
   }
}

UPaperSprite* UTATItemUIProxy::GetIcon() const
{
    return ItemInfo.GetDefaultObject()->Icon;
}

FText UTATItemUIProxy::GetName() const
{
   return ItemInfo.GetDefaultObject()->Name;
}

FText UTATItemUIProxy::GetDescription() const
{
   return ItemInfo.GetDefaultObject()->Description;
}

int UTATItemUIProxy::GetGoldValue() const
{
   return GetInfoCDO()->GoldValue;
}

UTATItemInfo* UTATItemUIProxy::GetInfoCDO() const
{
    return ItemInfo.GetDefaultObject();
}

bool UTATItemUIProxy::CanUse() const
{
   const UTATItemInfo* cdo = ItemInfo.GetDefaultObject();
   return cdo && cdo->OnUseEffect.Get() != nullptr;
}

bool UTATItemUIProxy::CanDrop() const
{
   const UTATItemInfo* cdo = ItemInfo.GetDefaultObject();
   return cdo && cdo->CanBeDropped;
}

bool UTATItemUIProxy::CanEverEquip() const
{
   const UTATItemInfo* cdo = ItemInfo.GetDefaultObject();
   return cdo && cdo->InventoryDestination == ETATInventoryDestination::ToolbeltIfPossible;
}

