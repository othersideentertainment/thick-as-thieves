// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/TATItemInventoryComponent.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Items/TATItemActor.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Items/TATItemInfo.h"
#include "Items/TATItemInventorySystemInterface.h"
#include "Online/TATGameState.h"
#include "Player/TATPlayerState.h"

// ose
#include "OSECommon.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/OSEGameplayAbility.h"
#include "Character/OSECharacterBase.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"

// ue4
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "ComponentReregisterContext.h"
#include "GameFramework/PlayerState.h"
#include "Items/ItemToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATItemInventoryComponent)

namespace InventoryHelpers
{
   bool CanTATItemBeDropped(TSubclassOf<UTATItemInfo> itemInfo)
   {
      const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
      return cdo && cdo->CanBeDropped;
   }

   FORCEINLINE TSubclassOf<UTATItemInfo> GetItemInfoForSlot(const TArray<FTATInventorySlot>& slots, int32 slotIndex)
   {
      return slots.IsValidIndex(slotIndex) ? slots[slotIndex].ItemInfo : nullptr;
   }

   FInventoryStackId GetStackIdForSlot(const TArray<FTATInventorySlot>& slots, int32 slotIndex)
   {
      return slots.IsValidIndex(slotIndex) ? slots[slotIndex].StackId : FInventoryStackId::Invalid;
   }

   void RemoveToolForItem(TScriptInterface<IToolSetInterface> toolset, TSubclassOf<UTATItemInfo> itemInfo)
   {
      // assumption: if Get() returns null, the tool class can't be loaded and assigned to the set, so we can skip
      if (UClass* loadedToolClass = itemInfo.GetDefaultObject()->ToolToGrant.Get())
      {
         int numToolsRemoved = toolset->AuthorityRemoveToolsOfClass(loadedToolClass);
         if (numToolsRemoved == 0)
         {
            UE_LOG(LogTATItems, Error, TEXT("Failed to remove any tools of class %s while removing item info %s from inventory"), *loadedToolClass->GetName(), *itemInfo->GetName());
         }
      }
   }

   bool IsCompatibleDestination(ETATInventoryDestination itemDefaultDestination, EInventoryType target)
   {
      using Dest = ETATInventoryDestination;
      switch (target)
      {
      case EInventoryType::Backpack:
         return (itemDefaultDestination == Dest::Backpack) || (itemDefaultDestination == Dest::ToolbeltIfPossible);
      case EInventoryType::QuestItems:
         return itemDefaultDestination == Dest::QuestItems;
      case EInventoryType::Toolbelt:
         return itemDefaultDestination == Dest::ToolbeltIfPossible;
      default:
         return false;
         
      }
   }
}

FGameplayTag FTATInventorySlot::GetProgressionTag() const
{
   const UTATItemInfo* itemInfo = ItemInfo.GetDefaultObject();
   return itemInfo ? itemInfo->ProgressionItemTag : FGameplayTag::EmptyTag;
}

bool FTATInventorySlot::ShouldKeepOnMissionSuccess() const
{
   const UTATItemInfo* itemInfo = ItemInfo.GetDefaultObject();
   return itemInfo && itemInfo->KeepOnMissionSuccess;
}

const UTATItemInfo* FTATInventorySlot::GetItemCDO() const
{
   return ItemInfo.GetDefaultObject();
}

bool FTATInventorySlot::operator==(const TSubclassOf<UTATItemInfo>& otherItemInfo) const
{
   return ItemInfo == otherItemInfo;
}

bool FTATInventorySlot::operator==(const FTATInventorySlot& other) const
{
   return other.ItemInfo == ItemInfo && other.StackCount == StackCount;
}

bool FTATInventoryWrapper::HasLimitedSlots() const
{
   return MaxSlots >= 0;
}

int32 FTATInventoryWrapper::GetNumSlotsOpenFor(TSubclassOf<UTATItemInfo> itemInfo) const
{
   if (Type == EInventoryType::Toolbelt)
   {
      return HasItemOfClass(itemInfo) ? 0 : FMath::Clamp(MaxSlots - Slots.Num(), 0, 1);
   }
   else
   {
      return HasLimitedSlots() ? FMath::Max(MaxSlots - Slots.Num(), 0) : MAX_int32;
   }
}

bool FTATInventoryWrapper::HasItemOfClass(TSubclassOf<UTATItemInfo> itemInfo) const
{
   return Slots.Contains(itemInfo);
}

FTATInventorySlot* FTATInventoryWrapper::FindFirstStackForClass(TSubclassOf<UTATItemInfo> itemInfo)
{
   return Slots.FindByKey(itemInfo);
}

TSubclassOf<UTATItemInfo> FTATInventoryWrapper::GetItemInfoForSlot(int32 slotIndex) const
{
   return InventoryHelpers::GetItemInfoForSlot(Slots, slotIndex);
}

TSubclassOf<UGameplayEffect> FTATInventoryWrapper::GetUseEffectForSlot(int32 slotIndex) const
{
   TSubclassOf<UTATItemInfo> itemInfo = GetItemInfoForSlot(slotIndex);
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
   return cdo ? cdo->OnUseEffect : nullptr;
}

UTATItemInventoryComponent::UTATItemInventoryComponent()
   : Super()
{
   PrimaryComponentTick.bCanEverTick = true;
}

void UTATItemInventoryComponent::BeginPlay()
{
   Super::BeginPlay();
}

void UTATItemInventoryComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);
}

void UTATItemInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, BucketSizes, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Backpack, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, QuestItems, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ToolbeltItems, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, GoldCarried, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _upgradeCurrencies, params);
}

UTATItemInventoryComponent* UTATItemInventoryComponent::GetTATItemInventoryFromActor(AActor* actor)
{
   if (auto inventoryHolder = Cast<ITATItemInventorySystemInterface>(actor))
   {
      return inventoryHolder->GetTATItemInventory();
   }
   return nullptr;
}

int32 UTATItemInventoryComponent::GetNumBackpackSlotsOpen() const
{
   return FMath::Max(BucketSizes.BackpackSize - Backpack.Num(), 0);
}

int32 UTATItemInventoryComponent::GetNumberOfItemByClass(const TSubclassOf<UItemInfo> itemInfoClass) const
{
   int itemCount = 0;
   _ForEachSlotWithBucket([&itemCount, itemInfoClass](EInventoryType bucket, const FTATInventorySlot& slot)
   {
      if (slot.ItemInfo == itemInfoClass)
      {
         itemCount += slot.StackCount;
      }
   });

   return itemCount;
}

bool UTATItemInventoryComponent::HasItemOfClass(TSubclassOf<UTATItemInfo> itemInfo) const
{
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
   if (!cdo)
   {
      return false;
   }

   switch (cdo->InventoryDestination)
   {
      case ETATInventoryDestination::Backpack:
      {
         return Backpack.Contains(itemInfo);
      }
      case ETATInventoryDestination::QuestItems:
      {
         return QuestItems.Contains(itemInfo);
      }
      case ETATInventoryDestination::ToolbeltIfPossible:
      {
         ToolbeltItems.Contains(itemInfo) || Backpack.Contains(itemInfo);
      }
      case ETATInventoryDestination::Gold:
      case ETATInventoryDestination::UpgradeCurrency:
      {
         return false;
      }
      default:
      {
         checkNoEntry();
         return false;
      }
   }
}

bool UTATItemInventoryComponent::HasItemOfCategory(FGameplayTag categoryTag) const
{
   const auto slotHasItemOfCategory = [&](const FTATInventorySlot& slot)
   {
      const UTATItemInfo* cdo = slot.ItemInfo.GetDefaultObject();
      return cdo && cdo->Category.MatchesTag(categoryTag);
   };

   return Backpack.ContainsByPredicate(slotHasItemOfCategory) || QuestItems.ContainsByPredicate(slotHasItemOfCategory);
}

bool UTATItemInventoryComponent::HasItemForMissionObjective(FGameplayTag missionObjectiveTag) const
{
   const auto slotHasItemForObjective = [&](const FTATInventorySlot& slot)
   {
      const UTATItemInfo* cdo = slot.ItemInfo.GetDefaultObject();
      return cdo && cdo->MissionObjectiveTag == missionObjectiveTag;
   };

   return Backpack.ContainsByPredicate(slotHasItemForObjective) || QuestItems.ContainsByPredicate(slotHasItemForObjective);
}

bool UTATItemInventoryComponent::FindItemForMissionObjective(FGameplayTag missionObjectiveTag, FInventoryStackId& stackId) const
{
   const auto slotHasItemForObjective = [&](const FTATInventorySlot& slot)
   {
      const UTATItemInfo* cdo = slot.ItemInfo.GetDefaultObject();
      return cdo && cdo->MissionObjectiveTag == missionObjectiveTag;
   };

   stackId = _FindStackIdByPredicate(slotHasItemForObjective);
   return stackId.IsValid();
}

bool UTATItemInventoryComponent::FindItemWithMetadata(FGameplayTag metadataTag, FInventoryStackId& stackId) const
{
   stackId = _FindStackIdByPredicate([&](const FTATInventorySlot& slot)
   {
      const UTATItemInfo* cdo = slot.ItemInfo.GetDefaultObject();
      return cdo && cdo->Metadata.HasTag(metadataTag);
   });
   return stackId.IsValid();
}

bool UTATItemInventoryComponent::FindItemOfCategory(FGameplayTag categoryObjectiveTag, FInventoryStackId& stackId) const
{
   stackId = _FindStackIdByPredicate([&](const FTATInventorySlot& slot)
   {
      const UTATItemInfo* cdo = slot.ItemInfo.GetDefaultObject();
      return cdo && cdo->Category.MatchesTag(categoryObjectiveTag);
   });
   return stackId.IsValid();
}

bool UTATItemInventoryComponent::FindItemOfItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass, FInventoryStackId& stackId) const
{
   stackId = _FindStackIdByPredicate([&](const FTATInventorySlot& slot)
   {
      const UTATItemInfo* cdo = slot.ItemInfo.GetDefaultObject();
      return cdo && cdo->GetClass() == itemInfoClass;
   });
   return stackId.IsValid();
}

bool UTATItemInventoryComponent::FindRandomItemOfCategory(FGameplayTag categoryTag, FInventoryStackId& stackId) const
{
   TArray<FInventoryStackId, TInlineAllocator<16>> foundStacks;
   ForEachInventorySlot(
      [&foundStacks, categoryTag](const FTATInventorySlot& slot, bool& done)
      {
         if (const UTATItemInfo* itemCDO = slot.GetItemCDO())
         {
            if (itemCDO->Category.MatchesTag(categoryTag) && slot.StackCount > 0)
            {
               foundStacks.Add(slot.StackId);
            }
         }
      }
   );
   if (foundStacks.Num() > 0)
   {
      stackId = foundStacks[FMath::RandRange(0, foundStacks.Num() - 1)];
      return true;
   }
   return false;
}

bool UTATItemInventoryComponent::FindPreferredStackToConsumeOfClass(TSubclassOf<UItemInfo> itemInfoClass, FInventoryStackId& stackId) const
{
   EInventoryType foundBucket;
   int32 foundCount;

   stackId = FInventoryStackId::Invalid;
   _ForEachSlotWithBucket([&, itemInfoClass](EInventoryType bucket, const FTATInventorySlot& slot)
   {
      if (slot.ItemInfo != itemInfoClass) return;
         
      if(!stackId.IsValid() || (bucket < foundBucket) || (foundBucket == bucket && slot.StackCount <= foundCount))
      {
         foundBucket = bucket;
         foundCount = slot.StackCount;
         stackId = slot.StackId;
      }
   });

   return stackId.IsValid();
}

TSubclassOf<UItemInfo> UTATItemInventoryComponent::GetItemClassForStack(FInventoryStackId stackId) const
{
   const FTATInventorySlot* slot = _FindSlotPtrForId(stackId);
   return slot ? slot->ItemInfo : nullptr;
}

bool UTATItemInventoryComponent::HasRoomForItem(TSubclassOf<UTATItemInfo> itemInfo, int32 amount) const
{
   if (itemInfo == nullptr) return false;
   if (amount == 0) return true;

   // TODO: some form of negative client prediction so it doesn't think it can pick something up while a pickup is pending?
   
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
   switch (cdo->InventoryDestination)
   {
   case ETATInventoryDestination::Backpack:
      return _GetOverflowForItemInBackpack(itemInfo, amount) == 0;
   case ETATInventoryDestination::ToolbeltIfPossible:
   {
      amount = _GetOverflowForItemInToolbelt(itemInfo, amount);

      // No longer using the backpack for toolbelt overflow
      return amount == 0 /*|| (_GetOverflowForItemInBackpack(itemInfo, amount) == 0)*/;
   }
   default:
      return true;
   }
}

bool UTATItemInventoryComponent::HasRoomInBucket(EInventoryType bucket, TSubclassOf<UTATItemInfo> itemInfo, int32 amount) const
{
   if (itemInfo == nullptr) return false;
   if (amount == 0) return true;


   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
   using Dest = ETATInventoryDestination;
   const ETATInventoryDestination cdoBucket = cdo->InventoryDestination;

   switch (bucket)
   {
   case EInventoryType::Backpack:
      return (cdoBucket == Dest::Backpack || cdoBucket == Dest::ToolbeltIfPossible) && _GetOverflowForItemInBackpack(itemInfo, amount) == 0;
   case EInventoryType::QuestItems:
      return cdoBucket == Dest::QuestItems;
   case EInventoryType::Toolbelt:
      return cdoBucket == Dest::ToolbeltIfPossible && _GetOverflowForItemInToolbelt(itemInfo, amount) == 0;
   default:
      return false;
   }
}

UTATItemInfo* UTATItemInventoryComponent::GetItemInfoForId(FInventoryStackId stackId) const
{
   const FTATInventorySlot* slot = _FindSlotPtrForId(stackId);
   return slot ? slot->ItemInfo.GetDefaultObject() : nullptr;
}

void UTATItemInventoryComponent::ForEachInventorySlot(const TFunctionRef<void(const FTATInventorySlot&, bool&)>& cb) const
{
   bool done = false;

   for (const FTATInventorySlot& slot : Backpack)
   {
      cb(slot, done);
      if (done)
         return;
   }

   for (const FTATInventorySlot& slot : ToolbeltItems)
   {
      cb(slot, done);
      if (done)
         return;
   }

   for (const FTATInventorySlot& slot : QuestItems)
   {
      cb(slot, done);
      if (done)
         return;
   }
}

bool UTATItemInventoryComponent::AuthorityAddItem(TSubclassOf<UTATItemInfo> itemInfo)
{
   return AuthorityAddItemMultiple(itemInfo, 1) > 0;
}

int32 UTATItemInventoryComponent::AuthorityAddItemMultiple(TSubclassOf<UTATItemInfo> itemInfo, int32 amount)
{
   check(GetOwner()->HasAuthority());

   int32 amountAdded = 0;
   if (!itemInfo || amount < 1)
   {
      return amountAdded;
   }

   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();

   // batch for potential removals
   FTATInventoryNotifyScope batchNotifies(this);

   // If item being added is toolbelt-equippable or a quest item, drop any other items with the same destination
   // This creates a de-facto limit of one for each.
   if (cdo->InventoryDestination == ETATInventoryDestination::ToolbeltIfPossible || cdo->InventoryDestination == ETATInventoryDestination::QuestItems)
   {
      const ETATInventoryDestination destinationToRemove = cdo->InventoryDestination;
      TArray<FInventoryStackId, TInlineAllocator<4>> itemStackIDsToRemove;
      _ForEachSlotWithBucket([&](EInventoryType bucket, const FTATInventorySlot& slot)
      {
         const UTATItemInfo* slotItemCDO = slot.GetItemCDO();

         // Ignore items of same type
         if (slot.ItemInfo == itemInfo) return;

         if (slotItemCDO->InventoryDestination == destinationToRemove)
         {
            itemStackIDsToRemove.Emplace(slot.StackId);
         }
      });

      for(const FInventoryStackId& stackId : itemStackIDsToRemove)
      {
         UE_LOG(LogTATItems, Verbose, TEXT("Removing item %s to make room for newly-added %s")
            , *GetItemInfoForId(stackId)->Name.ToString()
            , *cdo->Name.ToString());

         const bool spawnOnDrop = true;
         _AuthorityRemoveItemMultiple(stackId, -1, spawnOnDrop);
      }
   }

   // Route the item to the proper location in the inventory.
   switch (cdo->InventoryDestination)
   {
      case ETATInventoryDestination::Backpack:
      {
         FTATInventoryWrapper buckets[] = { _GetWrappedBackpack() };
         amountAdded = _AuthorityAddItemToInventoryBuckets(buckets, itemInfo, amount);
         break;
      }
      case ETATInventoryDestination::QuestItems:
      {
         FTATInventoryWrapper buckets[] = { _GetWrappedQuestItems() };
         amountAdded = _AuthorityAddItemToInventoryBuckets(buckets, itemInfo, amount);
         break;
      }
      case ETATInventoryDestination::ToolbeltIfPossible:
      {
         // No longer using the backpack for toolbelt overflow
         FTATInventoryWrapper buckets[] = { _GetWrappedToolbelt()/*, _GetWrappedBackpack()*/ };
         amountAdded = _AuthorityAddItemToInventoryBuckets(buckets, itemInfo, amount);
         break;
      }
      case ETATInventoryDestination::Gold:
      {
         if (cdo->GoldValue > 0)
         {
            const int goldWorth = cdo->GoldValue * amount;

            GoldCarried += goldWorth;
            MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, GoldCarried, this);

            GoldCarriedUpdated.Broadcast(GoldCarried);

            // assumption here is that the quest update below will want to use the gold value
            amountAdded = goldWorth;
         }
         else
         {
            // eh?  something is better than nothing...?
            amountAdded = amount;
         }

         break;
      }
      case ETATInventoryDestination::UpgradeCurrency:
      {
         const FGameplayTag currencyTag = cdo->UpgradeCurrencyTag;
         const int32 increment = cdo->UpgradeCurrencyValue * amount;
         if (currencyTag.IsValid() && increment > 0)
         {
            _upgradeCurrencies.Values.FindOrAdd(currencyTag) += increment;
            MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, _upgradeCurrencies, this);

            UpgradeCurrencyChanged.Broadcast(currencyTag, _upgradeCurrencies.GetValue(currencyTag));
         }
         amountAdded = amount;

         break;
      }
      default:
      {
         checkNoEntry();
         break;
      }
   }

   return amountAdded;
}

void UTATItemInventoryComponent::AuthorityRemoveItem(FInventoryStackId stackId)
{
   _AuthorityRemoveItemMultiple(stackId, 1, false);
}

void UTATItemInventoryComponent::AuthorityRemoveItemStack(FInventoryStackId stackId)
{
   _AuthorityRemoveItemMultiple(stackId, -1, false);
}

void UTATItemInventoryComponent::AuthorityRemoveItemMultiple(FInventoryStackId stackId, int32 amount)
{
   _AuthorityRemoveItemMultiple(stackId, amount, false);
}

void UTATItemInventoryComponent::ServerDropItem_Implementation(FInventoryStackId stackId)
{
   _AuthorityRemoveItemMultiple(stackId, 1, true);
}

void UTATItemInventoryComponent::ServerDropItemStack_Implementation(FInventoryStackId stackId)
{
   _AuthorityRemoveItemMultiple(stackId, -1, true);
}

void UTATItemInventoryComponent::ServerDropItemMultiple_Implementation(FInventoryStackId stackId, int32 amount)
{
   _AuthorityRemoveItemMultiple(stackId, amount, true);
}

void UTATItemInventoryComponent::ServerUseItem_Implementation(FInventoryStackId stackId)
{
   EInventoryType fromInventory;
   int32 slotIndex;
   if (_FindSlotForIdOrWarn(stackId, TEXT("UseItem"), fromInventory, slotIndex))
   {
      _AuthorityUseItem(fromInventory, slotIndex);
   }
}

void UTATItemInventoryComponent::ServerMoveStackToBucket_Implementation(FInventoryStackId stackId, EInventoryType destinationBucket)
{
   EInventoryType fromInventory;
   int32 slotIndex;
   if (!_FindSlotForIdOrWarn(stackId, TEXT("MoveStackToBucket"), fromInventory, slotIndex))
   {
      return;
   }

   if (fromInventory == destinationBucket)
   {
      UE_LOG(LogTATItems, Warning, TEXT("MoveStackToBucket: Cannot move item to same bucket"));
      return;
   }

   FTATInventoryWrapper sourceInventory = _GetWrapperForBucket(fromInventory);
   const TSubclassOf<UTATItemInfo> itemInfo = sourceInventory.GetItemInfoForSlot(slotIndex);
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
   if (!InventoryHelpers::IsCompatibleDestination(cdo->InventoryDestination, destinationBucket))
   {
      UE_LOG(LogTATItems, Warning, TEXT("MoveStackToBucket: Cannot put item in incompatible bucket"));
      return;
   }

   FTATInventoryNotifyScope notifyBatch(this);
   const int32 amountToTry = sourceInventory.Slots[slotIndex].StackCount;

   // add first
   FTATInventoryWrapper bucketsToMoveTo[] = { _GetWrapperForBucket(destinationBucket) };
   const int32 amountAdded = _AuthorityAddItemToInventoryBuckets(bucketsToMoveTo, itemInfo, amountToTry);

   // then remove what was added
   _AuthorityRemoveItemMultiple(stackId, amountAdded, false);
}

void UTATItemInventoryComponent::_CollectItemsThatNeedTools(TArray<TSubclassOf<UTATItemInfo>, TInlineAllocator<8>>& result) const
{
   for (const FTATInventorySlot& slot : ToolbeltItems)
   {
      const UTATItemInfo* cdo = slot.GetItemCDO();
      if (cdo && !cdo->ToolToGrant.IsNull())
      {
         result.AddUnique(slot.ItemInfo);
      }
   }

   ForEachInventorySlot([&result](const FTATInventorySlot& slot, bool& stop) {
      const UTATItemInfo* cdo = slot.GetItemCDO();
      if (cdo && !cdo->ToolToGrant.IsNull() && cdo->InventoryDestination != ETATInventoryDestination::ToolbeltIfPossible)
      {
         result.AddUnique(slot.ItemInfo);
      }
   });
}

void UTATItemInventoryComponent::_TryAddLoadedTool(TSubclassOf<UTATItemInfo> itemClass)
{
   if (itemClass == nullptr || _toolset == nullptr) return;

   TSubclassOf<UToolComponent> toolClass = itemClass.GetDefaultObject()->ToolToGrant.Get();
   if (toolClass == nullptr) return;

   // TODO: adjust when (some) items are explicitly equipped
   bool forToolbelt = itemClass.GetDefaultObject()->InventoryDestination == ETATInventoryDestination::ToolbeltIfPossible;
   if (!((forToolbelt && ToolbeltItems.Contains(itemClass)) || (!forToolbelt && HasItemOfClass(itemClass)))) return;

   if (UToolComponent* spawnedTool = _toolset->AuthorityCreateToolClass(toolClass))
   {
      // set this before we add the tool to the tool set so that the property gets sent over on initial replication
      if (UItemToolComponent* spawnedItemTool = Cast<UItemToolComponent>(spawnedTool))
      {
         spawnedItemTool->AuthoritySetGrantedByItemInfoClass(itemClass);
      }
      _toolset->AuthorityAddTool(spawnedTool);
   }
}

void UTATItemInventoryComponent::_NotifyBucketChanged(EInventoryType bucketType)
{
   if (_batchNotifies)
   {
      _pendingNotifies.Add(bucketType);
   }
   else
   {
      _FireEventsFor(bucketType);
   }
}

void UTATItemInventoryComponent::_FlushNotifies()
{
   check(_batchNotifies);

   _FireEventsFor(_pendingNotifies);
   _pendingNotifies.Reset();
   _batchNotifies = false;
}

void UTATItemInventoryComponent::_FireEventsFor(FInventoryTypeSet buckets)
{
   if (buckets.IsEmpty()) return;

   if (buckets.Contains(EInventoryType::Backpack))
   {
      BackpackChanged.Broadcast();
   }
   if (buckets.Contains(EInventoryType::QuestItems))
   {
      QuestItemsChanged.Broadcast();
   }
   if (buckets.Contains(EInventoryType::Toolbelt))
   {
      ToolbeltChanged.Broadcast();
   }

   ItemsChanged.Broadcast();
}

void UTATItemInventoryComponent::AuthorityOverrideSize(const FTATInventorySize& sizes)
{
   check(GetOwner()->HasAuthority());

   BucketSizes = sizes;
   MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, BucketSizes, this);
}

void UTATItemInventoryComponent::AuthoritySetToolset(TScriptInterface<IToolSetInterface> toolset)
{
   check(GetOwner()->HasAuthority());
   if (_toolset == toolset) return;

   using SmallItemArray = TArray<TSubclassOf<UTATItemInfo>, TInlineAllocator<8>>;
   SmallItemArray itemsToLoad;
   _CollectItemsThatNeedTools(itemsToLoad);

   // remove tools from old toolset if applicable
   if (_toolset && itemsToLoad.Num() > 0)
   {
      for (const TSubclassOf<UTATItemInfo>& itemClass : itemsToLoad)
      {
         InventoryHelpers::RemoveToolForItem(_toolset, itemClass);
      }
   }

   _toolset = toolset;

   // load and add tools to new toolset if any
   if (_toolset && itemsToLoad.Num() > 0)
   {
      TArray<FSoftObjectPath> pathsToLoad;
      for (const TSubclassOf<UTATItemInfo>& itemClass : itemsToLoad)
      {
         pathsToLoad.Add(FSoftObjectPath(itemClass.GetDefaultObject()->ToolToGrant.ToSoftObjectPath()));
      }

      TWeakObjectPtr<UTATItemInventoryComponent> weakThis(this);
      UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), [weakThis] {
         if(UTATItemInventoryComponent* self = weakThis.Get())
         {
            // looking up a second time, this may try to add tools a second time in the case
            // of races, but that should be relatively benign
            SmallItemArray itemsToAddToolsFor;
            self->_CollectItemsThatNeedTools(itemsToAddToolsFor);
            for (const TSubclassOf<UTATItemInfo>& itemClass : itemsToAddToolsFor)
            {
               self->_TryAddLoadedTool(itemClass);
            }
         }
      });
   }
}

int32 UTATItemInventoryComponent::_AuthorityRemoveItemMultiple(FInventoryStackId stackId, int32 amount, bool spawnItem)
{
   check(GetOwner()->HasAuthority());

   EInventoryType fromInventory;
   int32 slotIndex;
   if (!_FindSlotForIdOrWarn(stackId, TEXT("RemoveItem"), fromInventory, slotIndex))
   {
      return 0;
   }

   FTATInventoryWrapper wrappedInventory = _GetWrapperForBucket(fromInventory);
   TSubclassOf<UTATItemInfo> removedItemInfo = wrappedInventory.GetItemInfoForSlot(slotIndex);

   // If we're trying to drop this item, make sure it can be dropped!
   if (spawnItem && !InventoryHelpers::CanTATItemBeDropped(removedItemInfo))
   {
      return 0;
   }

   int32 amountRemoved = _AuthorityRemoveItemFromInventory(wrappedInventory, slotIndex, amount);

   if (spawnItem && removedItemInfo && amountRemoved > 0)
   {
      // Spawn the ItemActor with the amount we removed.
      const UTATItemInfo* cdo = removedItemInfo.GetDefaultObject();
      _AuthorityLoadAndSpawnItemActor(cdo->ItemActor, amountRemoved);
   }

   return amountRemoved;
}

bool UTATItemInventoryComponent::_AuthorityUseItem(EInventoryType fromInventory, int32 slotIndex)
{
   check(GetOwner()->HasAuthority());

   FTATInventoryWrapper wrappedInventory = _GetWrapperForBucket(fromInventory);
   TSubclassOf<UGameplayEffect> useEffect = wrappedInventory.GetUseEffectForSlot(slotIndex);
   // Remove one count of the item from the slot if it can be used.
   if (useEffect)
   {
      _AuthorityRemoveItemFromInventory(wrappedInventory, slotIndex, 1);
   }

   // Try to apply the effect.
   if (const UGameplayEffect* effect = useEffect.GetDefaultObject())
   {
      UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(GetOwner());
      asc->ApplyGameplayEffectToSelf(effect, UGameplayEffect::INVALID_LEVEL, asc->MakeEffectContext());
      return true;
   }

   return false;
}

FTATInventoryWrapper UTATItemInventoryComponent::_GetWrappedBackpack()
{
   return FTATInventoryWrapper(
      Backpack,
      BucketSizes.BackpackSize,
      EInventoryType::Backpack
   );
}

FTATInventoryWrapper UTATItemInventoryComponent::_GetWrappedQuestItems()
{
   return FTATInventoryWrapper(
      QuestItems,
      -1,
      EInventoryType::QuestItems
   );
}

FTATInventoryWrapper UTATItemInventoryComponent::_GetWrappedToolbelt()
{
   return FTATInventoryWrapper(
      ToolbeltItems,
      BucketSizes.ToolbeltSize,
      EInventoryType::Toolbelt
   );
}

FTATInventoryWrapper UTATItemInventoryComponent::_GetWrapperForBucket(EInventoryType bucket)
{
   switch (bucket)
   {
   case EInventoryType::Backpack:
      return _GetWrappedBackpack();
   case EInventoryType::QuestItems:
      return _GetWrappedQuestItems();
   case EInventoryType::Toolbelt:
      return _GetWrappedToolbelt();
   default:
      checkNoEntry();
      return _GetWrappedBackpack();
   }
}

int32 UTATItemInventoryComponent::_GetOverflowForItemInBackpack(TSubclassOf<UTATItemInfo>& itemInfo, int32 amount) const
{
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();

   // check if has space for worst case number of slots
   int32 maxStacksNeeded = 1;
   if (cdo->StackBehavior == EStackBehavior::Unique)
   {
      maxStacksNeeded = amount;
   }
   else if (cdo->StackBehavior == EStackBehavior::Limited)
   {
      maxStacksNeeded = FMath::DivideAndRoundUp(amount, cdo->MaxStackCount);
   }
   const int32 slotsOpen = GetNumBackpackSlotsOpen();
   if (slotsOpen >= maxStacksNeeded)
   {
      return 0;
   }

   if (cdo->StackBehavior == EStackBehavior::Unique)
   {
      return amount - slotsOpen;
   }
   else if (cdo->StackBehavior == EStackBehavior::Infinite)
   {
      const bool hasItem = Backpack.Contains(itemInfo);
      return hasItem ? 0 : amount;
   }

   // try to find space in partial stacks
   const int32 stackSize = cdo->MaxStackCount;
   int32 spacesToFill = amount - (slotsOpen * stackSize);

   for (const FTATInventorySlot& slot : Backpack)
   {
      if (slot.ItemInfo == itemInfo)
      {
         spacesToFill -= (stackSize - slot.StackCount);
      }
   }
   return FMath::Max(spacesToFill, 0);
}

int32 UTATItemInventoryComponent::_GetOverflowForItemInToolbelt(TSubclassOf<UTATItemInfo>& itemInfo, int32 amount) const
{
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
   const FTATInventorySlot* currentItem = ToolbeltItems.FindByKey(itemInfo);
   const bool hasEmptySlot = ToolbeltItems.Num() < BucketSizes.ToolbeltSize;
   
   // If item of different type is blocking this one from being added, return 0 overflow so it can be dropped in AuthorityAddItemMultiple()
   if (!hasEmptySlot && !currentItem)
   {
      return 0;
   }

   switch (cdo->StackBehavior)
   {
   case EStackBehavior::Unique:
      return (!currentItem && hasEmptySlot) ? amount - 1 : amount;
   case EStackBehavior::Infinite:
      return (currentItem || hasEmptySlot) ? 0 : amount;
   case EStackBehavior::Limited:
   {
      if (currentItem)
      {
         return FMath::Max(0, amount - (cdo->MaxStackCount - currentItem->StackCount));
      }
      else if (hasEmptySlot)
      {
         return FMath::Max(0, amount - cdo->MaxStackCount);
      }
      else
      {
         return amount;
      }
   }
   default:
      checkNoEntry();
      return amount;
   }
}

int32 UTATItemInventoryComponent::_AuthorityAddItemToInventoryBuckets(TArrayView<FTATInventoryWrapper> buckets, TSubclassOf<UTATItemInfo> itemInfo, int32 amount)
{
   check(GetOwner()->HasAuthority());

   if (!itemInfo || amount < 1)
   {
      return 0;
   }
   
   const bool alreadyContainedItem = HasItemOfClass(itemInfo);
   const UTATItemInfo* cdo = itemInfo.GetDefaultObject();

   FTATInventoryNotifyScope notifyBatch(this);
   int32 amountAdded = 0;
   int32 amountRemaining = amount;
   FInventoryTypeSet bucketsWithNewStacks;

   auto createStack = [&, this](FTATInventoryWrapper& bucket, int32 amount)
   {
      check(amount > 0);
      check(_batchNotifies);
      bucket.Slots.Emplace(itemInfo, amount, _GenerateStackId());
      amountRemaining -= amount;
      amountAdded += amount;
      bucketsWithNewStacks.Add(bucket.Type);
      _NotifyBucketChanged(bucket.Type);

      switch (bucket.Type)
      {
         case EInventoryType::Backpack: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Backpack, this); break;
         case EInventoryType::QuestItems: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, QuestItems, this); break;
         case EInventoryType::Toolbelt: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ToolbeltItems, this); break;
      }
   };

   auto addToStack = [&, this](FTATInventoryWrapper& bucket, FTATInventorySlot& slot, int32 amount)
   {
      check(_batchNotifies);
      if (amount <= 0) return;
      slot.StackCount += amount;
      amountRemaining -= amount;
      amountAdded += amount;
      _NotifyBucketChanged(bucket.Type);

      switch (bucket.Type)
      {
         case EInventoryType::Backpack: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Backpack, this); break;
         case EInventoryType::QuestItems: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, QuestItems, this); break;
         case EInventoryType::Toolbelt: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ToolbeltItems, this); break;
      }
   };

   switch (cdo->StackBehavior)
   {
   case EStackBehavior::Unique:
   {
      for (FTATInventoryWrapper& bucket : buckets)
      {
         const int32 toAdd = FMath::Min(amountRemaining, bucket.GetNumSlotsOpenFor(itemInfo));
         for (int i = 0; i < toAdd; ++i)
         {
            createStack(bucket, 1);
         }
      }

      break;
   }
   case EStackBehavior::Infinite:
   {
      [&]()
      {
         for (FTATInventoryWrapper& bucket : buckets)
         {
            if (FTATInventorySlot* slot = bucket.FindFirstStackForClass(itemInfo))
            {
               addToStack(bucket, *slot, amountRemaining);
               return;
            }
         }

         for (FTATInventoryWrapper& bucket : buckets)
         {
            if (bucket.GetNumSlotsOpenFor(itemInfo) > 0)
            {
               createStack(bucket, amountRemaining);
               return;
            }
         }
      }();
      break;
   }
   case EStackBehavior::Limited:
   {
      [&]()
      {
         // Find all the existing stacks of itemInfo and add as much as we can to each one.
         for (FTATInventoryWrapper& bucket : buckets)
         {
            for (FTATInventorySlot& slot : bucket.Slots)
            {
               if (slot.ItemInfo == itemInfo)
               {
                  const int32 toAdd = FMath::Min(amountRemaining, cdo->MaxStackCount - slot.StackCount);
                  addToStack(bucket, slot, toAdd);
                  if (amountRemaining <= 0)
                  {
                     return;
                  }
               }
            }
         }

         // If we still have some amount left to add, then add as many stacks as we can.
         for (FTATInventoryWrapper& bucket : buckets)
         {
            while (amountRemaining > 0 && bucket.GetNumSlotsOpenFor(itemInfo) > 0)
            {
               const int32 toAdd = FMath::Min(amountRemaining, cdo->MaxStackCount);
               createStack(bucket, toAdd);
            }
         }
      }();
      break;
   }
   default:
   {
      checkNoEntry();
      break;
   }
   }

   if (amountAdded > 0)
   {
      // Grant OnAdd GameplayEffect if we didn't have an instance of this already.
      // TODO: Allow stacking effects for each instance of an item?
      if (cdo->OnAddEffect && !alreadyContainedItem)
      {
         UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(GetOwner());
         const UGameplayEffect* effect = cdo->OnAddEffect.GetDefaultObject();
         if (asc && asc->AbilityActorInfo.IsValid())
         {
            // TODO: Store the gameplay effect handle to remove it later when all instances of this item leave the inventory.
            FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
            effectContext.AddSourceObject(this);
            asc->ApplyGameplayEffectToSelf(effect, UGameplayEffect::INVALID_LEVEL, effectContext);
         }
         else
         {
            // UE5 Port TODO: Looks like this can happen before the guard has a configured asc??
            UE_LOG(LogTATItems, Error, TEXT("Failed to apply inventory effect %s!"), *effect->GetName());
         }
      }

      if(!alreadyContainedItem && cdo->UniquePickupPlayerStatTag.IsValid())
      {
         UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatUniqueByName(GetOwner(), cdo->UniquePickupPlayerStatTag, cdo->GetFName());
      }

      // Grant Tools
      // NOTE: Not granting multiple tools for multiple items for now, because tools only support 1-per-class-type,
      //       but we may need to change this for the health potion use-case where we want two health potion tools
      if (!cdo->ToolToGrant.IsNull() && _toolset && 
         ((cdo->InventoryDestination == ETATInventoryDestination::ToolbeltIfPossible && bucketsWithNewStacks.Contains(EInventoryType::Toolbelt)) ||
          (cdo->InventoryDestination != ETATInventoryDestination::ToolbeltIfPossible && !alreadyContainedItem)))
      {
         TWeakObjectPtr<UTATItemInventoryComponent> weakThis(this);
         TSoftClassPtr<UTATItemInfo> softItemClass(itemInfo);
         UAssetManager::GetStreamableManager().RequestAsyncLoad(cdo->ToolToGrant.ToSoftObjectPath(), [weakThis, softItemClass]
            {
               if (weakThis.IsValid())
               {
                  weakThis->_TryAddLoadedTool(softItemClass.Get());
               }
            });
      }
   }

   return amountAdded;
}

int32 UTATItemInventoryComponent::_AuthorityRemoveItemFromInventory(FTATInventoryWrapper& wrappedInventory, int32 slotIndex, int32 amount)
{
   check(GetOwner()->HasAuthority());

   // Verify slot and amount are valid.
   int32 amountDropped = 0;
   TSubclassOf<UTATItemInfo> itemInfo;
   if (!wrappedInventory.Slots.IsValidIndex(slotIndex) || amount == 0)
   {
      return amountDropped;
   }

   bool emptiedSlot = false;
   {
      FTATInventorySlot& slot = wrappedInventory.Slots[slotIndex];
      itemInfo = slot.ItemInfo;

      // Figure out how much of this item we're actually dropping and remove it from the slot.
      // If amount is negative, drop the full stack.
      amountDropped = amount > 0 ? FMath::Min(amount, slot.StackCount) : slot.StackCount;
      slot.StackCount -= amountDropped;

      // Check if we emptied the slot just now.
      emptiedSlot = slot.StackCount <= 0;
   }

   if (emptiedSlot)
   {
      // We emptied the slot, so remove it.
      wrappedInventory.Slots.RemoveAt(slotIndex);

      switch (wrappedInventory.Type)
      {
         case EInventoryType::Backpack: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Backpack, this); break;
         case EInventoryType::QuestItems: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, QuestItems, this); break;
         case EInventoryType::Toolbelt: MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ToolbeltItems, this); break;
      }

      const bool anyRemaining = wrappedInventory.HasItemOfClass(itemInfo);
      const UTATItemInfo* cdo = itemInfo.GetDefaultObject();
      if (!anyRemaining && cdo)
      {
         // remove effects granted on add
         if (cdo->OnAddEffect)
         {
            UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(GetOwner());
            if (asc)
            {
               // TODO: This mostly works, but multiple items that grant the same effect could run into problems. May want to track more explicitly
               FGameplayEffectQuery query;
               query.EffectSource = this;
               query.EffectDefinition = cdo->OnAddEffect;
               asc->RemoveActiveEffects(query);
            }
         }

         // remove tools granted on add
         // TODO: Similar to effects above, this mostly works, but multiple items that grant the same tool could run into problems. May want to track more explicitly
         if (!cdo->ToolToGrant.IsNull() && _toolset && (cdo->InventoryDestination != ETATInventoryDestination::ToolbeltIfPossible || wrappedInventory.Type == EInventoryType::Toolbelt))
         {
            InventoryHelpers::RemoveToolForItem(_toolset, itemInfo);
         }
      }
   }

   // Broadcast ItemChanged events.
   _NotifyBucketChanged(wrappedInventory.Type);

   return amountDropped;
}

void UTATItemInventoryComponent::_AuthorityLoadAndSpawnItemActor(TSoftClassPtr<AItemActor> itemActorClass, int32 stackCount)
{
   check(GetOwner()->HasAuthority());
   if (itemActorClass.IsNull() || stackCount <= 0)
   {
      return;
   }

   // Get our pawn, since we might be owned by player state.
   APawn* pawn = UOSECommon::GetPawn(GetOwner());
   if (!pawn)
   {
      UE_LOG(LogTATItems, Error, TEXT("Failed to spawn item actor %s: %s does not have a Pawn."), *itemActorClass->GetName(), *GetPathName());
      return;
   }

   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   float dropDistance = 100.0f;
   const FVector dropLocation = UTATItemFunctionLibrary::FindSuggestedDropStartFromActorEyes(pawn, dropDistance, settings.ItemDropTraceProfile);


   UAssetManager::GetStreamableManager().RequestAsyncLoad(itemActorClass.ToSoftObjectPath(), [itemActorClass, stackCount, weakWorld = MakeWeakObjectPtr(GetWorld()), weakPawn = MakeWeakObjectPtr(pawn), dropLocation] {
   
      if (UWorld* world = weakWorld.Get())
      {
         _AuthoritySpawnItemActor(itemActorClass.Get(), stackCount, world, weakPawn.Get(), dropLocation);
      }
   });
}

void UTATItemInventoryComponent::_AuthoritySpawnItemActor(TSubclassOf<AItemActor> itemActorClass, int32 stackCount, UWorld* world, APawn* pawn, const FVector& dropLocation)
{
   if (!itemActorClass || stackCount <= 0)
   {
      return;
   }

   FTransform transform;
   if (pawn)
   {
      FVector fwd = pawn->GetActorForwardVector();
      fwd.Normalize();
      // TODO: Spawn it facing the player instead of away from the player.
      const FRotator rotation = fwd.Rotation();
      transform.SetRotation(rotation.Quaternion());
   }


   // Use SpawnActorDeferred so we can set StackCount and have it replicate properly.
   if (auto* itemActor = world->SpawnActorDeferred<ATATItemActor>(itemActorClass, transform))
   {
      itemActor->AuthoritySetStackCount(stackCount);

      const UTATProjectSettings& settings = UTATProjectSettings::Get();

      // Get our owning actor's location or possessed pawn if we live on player state.
      UGameplayStatics::FinishSpawningActor(itemActor, FTransform());

      // We spawn at origin and then move to the drop location because the spawned actor's
      // root component not initialized until after FinishSpawningActor, and we need it
      // to calculate the final position.
      FVector newDropLocation;
      UTATItemFunctionLibrary::FindDropLocationFromSuggestedStart(itemActor, pawn, settings.ItemDropTraceProfile, dropLocation, newDropLocation);
      transform.SetLocation(newDropLocation);
      itemActor->HandleDrop(dropLocation, newDropLocation, pawn);
   }
}

void UTATItemInventoryComponent::_AuthorityRemoveLoadoutRelevantItems()
{
   auto shouldRemove = [](const FTATInventorySlot& slot) {
      return slot.ShouldKeepOnMissionSuccess();
   };

   FTATInventoryNotifyScope batchNotifies(this);

   if (Backpack.RemoveAll(shouldRemove))
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Backpack, this);
      _NotifyBucketChanged(EInventoryType::Backpack);
   }

   if (ToolbeltItems.RemoveAll(shouldRemove))
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ToolbeltItems, this);
      _NotifyBucketChanged(EInventoryType::Toolbelt);
   }
}

void UTATItemInventoryComponent::_OnRep_Backpack()
{
   _NotifyBucketChanged(EInventoryType::Backpack);
}

void UTATItemInventoryComponent::_OnRep_QuestItems()
{
   _NotifyBucketChanged(EInventoryType::QuestItems);
}

void UTATItemInventoryComponent::_OnRep_Toolbelt()
{
   _NotifyBucketChanged(EInventoryType::Toolbelt);
}

void UTATItemInventoryComponent::_OnRep_GoldCarried()
{
   GoldCarriedUpdated.Broadcast(GoldCarried);
}

void UTATItemInventoryComponent::_OnRep_UpgradeCurrencies(const FOSESerializedTagMap& previousValue)
{
   // assumption is that currencies are never removed
   for (const TPair<FGameplayTag, int32>& pair : _upgradeCurrencies.Values)
   {
      if (previousValue.GetValue(pair.Key) != pair.Value)
      {
         UpgradeCurrencyChanged.Broadcast(pair.Key, pair.Value);
      }
   }
}

template<typename T>
inline bool UTATItemInventoryComponent::_FindSlotByPredicate(EInventoryType& inventoryType, int32& slotIndex, const T& predicate) const
{
   slotIndex = Backpack.IndexOfByPredicate(predicate);
   if (slotIndex > INDEX_NONE)
   {
      inventoryType = EInventoryType::Backpack;
      return true;
   }

   slotIndex = ToolbeltItems.IndexOfByPredicate(predicate);
   if (slotIndex > INDEX_NONE)
   {
      inventoryType = EInventoryType::Toolbelt;
      return true;
   }

   slotIndex = QuestItems.IndexOfByPredicate(predicate);
   if (slotIndex > INDEX_NONE)
   {
      inventoryType = EInventoryType::QuestItems;
      return true;
   }

   return false;
}

template<typename T>
const FTATInventorySlot* UTATItemInventoryComponent::_FindSlotPtrByPredicate(const T& predicate) const
{
   const FTATInventorySlot* slot = Backpack.FindByPredicate(predicate);
   if (slot)
   {
      return slot;
   }

   slot = ToolbeltItems.FindByPredicate(predicate);
   if (slot)
   {
      return slot;
   }

   slot = QuestItems.FindByPredicate(predicate);
   return slot;
}

template<typename T>
void UTATItemInventoryComponent::_ForEachSlotWithBucket(const T& callback) const
{
   using Entry = TPair<EInventoryType, const TArray<FTATInventorySlot>&>;
   const Entry buckets[] = {
      Entry {EInventoryType::Backpack, Backpack},
      Entry {EInventoryType::Toolbelt, ToolbeltItems},
      Entry {EInventoryType::QuestItems, QuestItems}
   };

   for (const Entry& entry : buckets)
   {
      const EInventoryType bucketType = entry.Key;
      for (const FTATInventorySlot& slot : entry.Value)
      {
         callback(bucketType, slot);
      }
   }
}

template<typename T>
FInventoryStackId UTATItemInventoryComponent::_FindStackIdByPredicate(const T& predicate) const
{
   const FTATInventorySlot* slot = _FindSlotPtrByPredicate(predicate);
   return slot ? slot->StackId : FInventoryStackId::Invalid;
}

bool UTATItemInventoryComponent::_FindSlotForId(FInventoryStackId stackId, EInventoryType& inventoryType, int32& slotIndex) const
{
   return _FindSlotByPredicate(inventoryType, slotIndex, [stackId](const FTATInventorySlot& slot) { return slot.StackId == stackId; });
}

const FTATInventorySlot* UTATItemInventoryComponent::_FindSlotPtrForId(FInventoryStackId stackId) const
{
   return _FindSlotPtrByPredicate([stackId](const FTATInventorySlot& slot) { return slot.StackId == stackId; });
}

bool UTATItemInventoryComponent::_FindSlotForIdOrWarn(FInventoryStackId stackId, const TCHAR* context, EInventoryType& inventoryType, int32& slotIndex) const
{
   if (!_FindSlotForId(stackId, inventoryType, slotIndex))
   {
      UE_LOG(LogTATItems, Warning, TEXT("%s: Failed find item stack for id %d"), context, stackId.GetValue());
      return false;
   }
   return true;
}

FTATInventoryNotifyScope::FTATInventoryNotifyScope(UTATItemInventoryComponent* inventory) :_inventory(inventory)
{
   check(inventory);
   if (!inventory->_batchNotifies)
   {
      inventory->_batchNotifies = true;
      _ownsBatch = true;
   }
}

FTATInventoryNotifyScope::~FTATInventoryNotifyScope()
{
   if (_ownsBatch)
   {
      _inventory->_FlushNotifies();
   }
}
