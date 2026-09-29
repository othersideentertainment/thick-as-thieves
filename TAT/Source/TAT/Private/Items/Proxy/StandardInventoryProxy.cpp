// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Proxy/StandardInventoryProxy.h"

// tat
#include "Items/TATItemInventoryComponent.h"
#include "Items/Proxy/TATItemUIProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(StandardInventoryProxy)

UStandardInventoryProxy::UStandardInventoryProxy()
{
   _proxyClass = UTATItemUIProxy::StaticClass();
}

void UStandardInventoryProxy::Init(UTATItemInventoryComponent* inventory, TSubclassOf<UTATItemUIProxy> proxyClass)
{
   check(inventory);
   _inventory = inventory;
   _proxyClass = proxyClass;
   // TODO: add bucket-agnostic event to component itself?
   _inventory->BackpackChanged.AddUniqueDynamic(this, &UStandardInventoryProxy::_OnBackpackChanged);
   _inventory->QuestItemsChanged.AddUniqueDynamic(this, &UStandardInventoryProxy::_OnQuestItemsChanged);
   _inventory->ToolbeltChanged.AddUniqueDynamic(this, &UStandardInventoryProxy::_OnToolbeltChanged);
   _inventory->UpgradeCurrencyChanged.AddUniqueDynamic(this, &UStandardInventoryProxy::_OnUpgradeCurrencyChanged);

   _OnBackpackChanged();
   _OnQuestItemsChanged();
   _OnToolbeltChanged();
}

bool UStandardInventoryProxy::CanMoveStackToBucket(UTATItemUIProxy* stack, EInventoryType bucket) const
{
   check(stack);
   return ensure(_inventory) && _inventory->HasRoomInBucket(bucket, stack->ItemInfo, stack->Amount);
}

void UStandardInventoryProxy::MoveStackToBucket(UTATItemUIProxy* stack, EInventoryType bucket)
{
   check(stack);
   if (ensure(_inventory))
   {
      _inventory->ServerMoveStackToBucket(stack->StackId, bucket);
   }
}

const TArray< UTATItemUIProxy*>& UStandardInventoryProxy::GetBucket(EInventoryType bucketType) const
{
   const FStandardInventoryProxyBucket* bucket = _standardBuckets.Find(bucketType);
   check(bucket);
   return bucket->Items;
}

int32 UStandardInventoryProxy::GetUpgradeCurrency(FTATCharacterSaveId character, FGameplayTag currencyTag) const
{
   if (ensure(_inventory))
   {
      return _inventory->GetUpgradeCurrency(currencyTag);
   }
   
   return 0;
}

void UStandardInventoryProxy::_UpdateBucket(EInventoryType bucketType, const TArray<FTATInventorySlot>& slots)
{
   FStandardInventoryProxyBucket& bucket = _standardBuckets.FindOrAdd(bucketType);

   // add new item proxies, reusing as needed
   bucket.Items.Reset(slots.Num());
   for (const FTATInventorySlot& slot : slots)
   {
      UTATItemUIProxy*& proxy = bucket.Cache.FindOrAdd(slot.StackId, nullptr);
      if (proxy == nullptr)
      {
         proxy = NewObject<UTATItemUIProxy>(this, _proxyClass);
         proxy->InitForMission(slot.ItemInfo, slot.StackId, bucketType);
      }
      proxy->SetAmount(slot.StackCount);

      bucket.Items.Add(proxy);
   }

   // remove stale cache items
   for (TMap<FInventoryStackId, UTATItemUIProxy*>::TIterator cacheIterator = bucket.Cache.CreateIterator(); cacheIterator; ++cacheIterator)
   {
      if (!bucket.Items.Contains(cacheIterator.Value()))
      {
         cacheIterator.RemoveCurrent();
      }
   }

   OnBucketChanged.Broadcast(bucketType);
}

void UStandardInventoryProxy::_OnBackpackChanged()
{
   _UpdateBucket(EInventoryType::Backpack, _inventory->GetBackpack());
}

void UStandardInventoryProxy::_OnQuestItemsChanged()
{
   _UpdateBucket(EInventoryType::QuestItems, _inventory->GetQuestItems());
}

void UStandardInventoryProxy::_OnToolbeltChanged()
{
   _UpdateBucket(EInventoryType::Toolbelt, _inventory->GetToolbeltItems());
}

void UStandardInventoryProxy::_OnUpgradeCurrencyChanged(FGameplayTag currencyTag, int32 amount)
{
   UpgradeCurrencyChanged.Broadcast(currencyTag, amount);
}

