// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Items/TATInventoryTypes.h"

// ose
#include "Abilities/OSESerializedTagMap.h"
#include "Items/ItemInventoryComponent.h"

// ue4
#include "GameplayTagContainer.h"

#include "TATItemInventoryComponent.generated.h"

class AItemActor;
class UGameplayEffect;
class UTATItemInfo;

class IToolSetInterface;

struct FTATItemLoadout;
struct FInventoryBucketSet;
struct FTATInventoryNotifyScope;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FItemInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGoldCarriedUpdated, int32, gold);

USTRUCT(BlueprintType)
struct TAT_API FTATInventorySlot
{
   GENERATED_BODY()

public:

   FTATInventorySlot() {}

   FTATInventorySlot(TSubclassOf<UTATItemInfo> itemInfo, int32 stackCount, FInventoryStackId stackId)
      : ItemInfo(itemInfo)
      , StackCount(stackCount)
      , StackId(stackId)
   {}

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSubclassOf<UTATItemInfo> ItemInfo;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ClampMin = "1", UIMin = "1"))
   int32 StackCount = 1;

   // A non-persistent id that can be used to identify an inventory item stack, no guarantee that it will remain the same if dropped and picked up, or even moved
   UPROPERTY(Transient, BlueprintReadOnly)
   FInventoryStackId StackId;

   FGameplayTag GetProgressionTag() const;
   bool ShouldKeepOnMissionSuccess() const;
   const UTATItemInfo* GetItemCDO() const;

   bool operator==(const FTATInventorySlot& other) const;
   bool operator==(const TSubclassOf<UTATItemInfo>& otherItemInfo) const;
};

// A struct that wraps around different inventories--like the backpack and quest items--so we
// can use the same code to update each of them.
struct FTATInventoryWrapper
{
   FTATInventoryWrapper(
      TArray<FTATInventorySlot>& slots,
      int32 maxSlots,
      EInventoryType bucketType
   )
      : Slots(slots)
      , MaxSlots(maxSlots)
      , Type(bucketType)
   {}

   bool HasLimitedSlots() const;
   int32 GetNumSlotsOpenFor(TSubclassOf<UTATItemInfo> itemInfo) const;
   bool HasItemOfClass(TSubclassOf<UTATItemInfo> itemInfo) const;
   FTATInventorySlot* FindFirstStackForClass(TSubclassOf<UTATItemInfo> itemInfo);
   TSubclassOf<UTATItemInfo> GetItemInfoForSlot(int32 slotIndex) const;
   TSubclassOf<UGameplayEffect> GetUseEffectForSlot(int32 slotIndex) const;

   TArray<FTATInventorySlot>& Slots;
   const int32 MaxSlots = -1;
   const EInventoryType Type;
};

// A simple wrapper around a bitmask of inventory buckets
struct FInventoryTypeSet
{
   FInventoryTypeSet() {}
   FInventoryTypeSet(EInventoryType startingBucket)
   {
      Add(startingBucket);
   }

   void Add(EInventoryType bucketType)
   {
      _bucketMask |= (1 << static_cast<int32>(bucketType));
   }

   bool Contains(EInventoryType bucketType) const
   {
      return (_bucketMask & (1 << static_cast<int32>(bucketType))) != 0;
   }

   bool IsEmpty() const { return _bucketMask == 0; }

   void Reset()
   {
      _bucketMask = 0;
   }
private:
   uint8 _bucketMask = 0;
};

UCLASS()
class TAT_API UTATItemInventoryComponent : public UItemInventoryComponent
{
   GENERATED_BODY()

public:

   UTATItemInventoryComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   static UTATItemInventoryComponent* GetTATItemInventoryFromActor(AActor* actor);

   // TODO: delete method
   int32 GetNumBackpackSlotsOpen() const;

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Items")
   int32 GetNumberOfItemByClass(const TSubclassOf<UItemInfo> itemInfoClass) const;

   UFUNCTION(BlueprintCallable, Category = "Items")
   bool HasItemOfClass(TSubclassOf<UTATItemInfo> itemInfo) const;
   UFUNCTION(BlueprintCallable, Category = "Items")
   bool HasItemOfCategory(FGameplayTag categoryTag) const;
   UFUNCTION(BlueprintCallable, Category = "Items")
   bool HasItemForMissionObjective(FGameplayTag missionObjectiveTag) const;
   UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Items")
   bool FindItemForMissionObjective(FGameplayTag missionObjectiveTag, FInventoryStackId& stackId) const;
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Items")
   bool FindItemWithMetadata(FGameplayTag metadataTag, FInventoryStackId& stackId) const;
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Items")
   bool FindItemOfCategory(FGameplayTag categoryObjectiveTag, FInventoryStackId& stackId) const;
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Items")
   bool FindItemOfItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass, FInventoryStackId& stackId) const;

   /// Picks a random item stack matching the specified category and returns it, if available
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Items")
   bool FindRandomItemOfCategory(FGameplayTag categoryTag, FInventoryStackId& stackId) const;

   // find a stack to consume, preferring backpack and stacks with fewer items
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Items")
   bool FindPreferredStackToConsumeOfClass(TSubclassOf<UItemInfo> itemInfoClass, FInventoryStackId& stackId) const;

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Items")
   TSubclassOf<UItemInfo> GetItemClassForStack(FInventoryStackId stackId) const;

   bool HasRoomForItem(TSubclassOf<UTATItemInfo> itemInfo, int32 amount) const;

   bool HasRoomInBucket(EInventoryType bucket, TSubclassOf<UTATItemInfo> itemInfo, int32 amount) const;

   UFUNCTION(BlueprintCallable, Category = "Items")
   const TArray<FTATInventorySlot>& GetBackpack() const { return Backpack; }
   UFUNCTION(BlueprintCallable, Category = "Items")
   const TArray<FTATInventorySlot>& GetQuestItems() const { return QuestItems; }
   UFUNCTION(BlueprintCallable, Category = "Items")
   const TArray<FTATInventorySlot>& GetToolbeltItems() const { return ToolbeltItems; }

   /// Returns the CDO of the item info at the given inventory slot, or nullptr if invalid
   UFUNCTION(BlueprintCallable, Category = "Items")
   UTATItemInfo* GetItemInfoForId(FInventoryStackId stackId) const;

   /// Callback is called for each item in the Backpack and Quest, set the bool to true to stop looping
   void ForEachInventorySlot(const TFunctionRef<void(const FTATInventorySlot&, bool&)>& cb) const;

   UFUNCTION(BlueprintCallable, Category = "Items")
   int32 GetGoldCarried() const { return GoldCarried; }

   UFUNCTION(BlueprintCallable, Category = "Items")
   int32 GetUpgradeCurrency(FGameplayTag currencyTag) const { return _upgradeCurrencies.GetValue(currencyTag); }
   
   // keeping this non-blueprint exposed for now to make it easier to change
   const FOSESerializedTagMap& GetAllUpgradeCurrencies() const { return _upgradeCurrencies; }

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
   bool AuthorityAddItem(TSubclassOf<UTATItemInfo> itemInfo);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
   int32 AuthorityAddItemMultiple(TSubclassOf<UTATItemInfo> itemInfo, int32 amount);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
   void AuthorityRemoveItem(FInventoryStackId stackId);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
   void AuthorityRemoveItemStack(FInventoryStackId stackId);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Items")
   void AuthorityRemoveItemMultiple(FInventoryStackId stackId, int32 amount);

   UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Items")
   void ServerDropItem(FInventoryStackId stackId);
   UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Items")
   void ServerDropItemStack(FInventoryStackId stackId);
   UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Items")
   void ServerDropItemMultiple(FInventoryStackId stackId, int32 amount);

   UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Items")
   void ServerUseItem(FInventoryStackId stackId);

   UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Items")
   void ServerMoveStackToBucket(FInventoryStackId stackId, EInventoryType destinationBucket);

   /// Overrides the max number of slots. No items will be removed, even if the new size is smaller than the current item count.
   void AuthorityOverrideSize(const FTATInventorySize& sizes);

   /// Sets the toolset, and adds any tools that should be added
   void AuthoritySetToolset(TScriptInterface<IToolSetInterface> toolset);

   UPROPERTY(BlueprintAssignable, Category = "Items")
   FItemInventoryChanged BackpackChanged;

   UPROPERTY(BlueprintAssignable, Category = "Items")
   FItemInventoryChanged QuestItemsChanged;

   UPROPERTY(BlueprintAssignable, Category = "Items")
   FItemInventoryChanged ToolbeltChanged;

   UPROPERTY(BlueprintAssignable, Category = "Items")
   FItemInventoryChanged ItemsChanged;

   UPROPERTY(BlueprintAssignable, Category = "Items")
   FGoldCarriedUpdated GoldCarriedUpdated;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUpgradeCurrencyChanged, FGameplayTag, currency, int32, value);
   UPROPERTY(BlueprintAssignable, Category = "Items")
   FUpgradeCurrencyChanged UpgradeCurrencyChanged;

protected:

   FTATInventoryWrapper _GetWrappedBackpack();
   FTATInventoryWrapper _GetWrappedQuestItems();
   FTATInventoryWrapper _GetWrappedToolbelt();
   FTATInventoryWrapper _GetWrapperForBucket(EInventoryType bucket);

   int32 _GetOverflowForItemInBackpack(TSubclassOf<UTATItemInfo>& itemInfo, int32 amount) const;
   int32 _GetOverflowForItemInToolbelt(TSubclassOf<UTATItemInfo>& itemInfo, int32 amount) const;

   template<typename T>
   bool _FindSlotByPredicate(EInventoryType& inventoryType, int32& slotIndex, const T& predicate) const;

   template<typename T>
   const FTATInventorySlot* _FindSlotPtrByPredicate(const T& predicate) const;

   template<typename T>
   FInventoryStackId _FindStackIdByPredicate(const T& predicate) const;

   template<typename T>
   void _ForEachSlotWithBucket(const T& predicate) const;

   const FTATInventorySlot* _FindSlotPtrForId(FInventoryStackId stackId) const;
   bool _FindSlotForId(FInventoryStackId stackId, EInventoryType& inventoryType, int32& slotIndex) const;
   bool _FindSlotForIdOrWarn(FInventoryStackId stackId, const TCHAR* context, EInventoryType& inventoryType, int32& slotIndex) const;
   int32 _AuthorityRemoveItemMultiple(FInventoryStackId stackId, int32 amount, bool spawnItem);
   bool _AuthorityUseItem(EInventoryType fromInventory, int32 slotIndex);

   int32 _AuthorityAddItemToInventoryBuckets(TArrayView<FTATInventoryWrapper> buckets, TSubclassOf<UTATItemInfo> itemInfo, int32 amount);
   int32 _AuthorityRemoveItemFromInventory(FTATInventoryWrapper& wrappedInventory, int32 slotIndex, int32 amount);
   void _AuthorityLoadAndSpawnItemActor(TSoftClassPtr<AItemActor> itemActorClass, int32 stackCount);
   static void _AuthoritySpawnItemActor(TSubclassOf<AItemActor> itemActorClass, int32 stackCount, UWorld* world, APawn* pawn, const FVector& dropLocation);

   void _AuthorityRemoveLoadoutRelevantItems();
   void _AuthorityUseLoadedLoadout(const FTATItemLoadout& loadout, const TArray<FTATInventorySlot>& additionalItems);

   void _CollectItemsThatNeedTools(TArray<TSubclassOf<UTATItemInfo>, TInlineAllocator<8>>& result) const;
   void _TryAddLoadedTool(TSubclassOf<UTATItemInfo> itemClass);

   void _NotifyBucketChanged(EInventoryType bucketType);
   void _FlushNotifies();
   void _FireEventsFor(FInventoryTypeSet buckets);

   UFUNCTION()
   void _OnRep_Backpack();
   UFUNCTION()
   void _OnRep_QuestItems();
   UFUNCTION()
   void _OnRep_Toolbelt();
   UFUNCTION()
   void _OnRep_GoldCarried();
   UFUNCTION()
   void _OnRep_UpgradeCurrencies(const FOSESerializedTagMap& previousValue);

   FInventoryStackId _GenerateStackId() { return FInventoryStackId(++_nextStackId); }

   /// The number of items the backpack can hold.
   UPROPERTY(BlueprintReadOnly, Replicated, Category = "Inventory")
   FTATInventorySize BucketSizes;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_Backpack)
   TArray<FTATInventorySlot> Backpack;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_QuestItems)
   TArray<FTATInventorySlot> QuestItems;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_Toolbelt)
   TArray<FTATInventorySlot> ToolbeltItems;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_UpgradeCurrencies)
   FOSESerializedTagMap _upgradeCurrencies;

   UPROPERTY(Transient)
   TScriptInterface<IToolSetInterface> _toolset;

   friend struct FTATInventoryNotifyScope;
   FInventoryTypeSet _pendingNotifies;
   bool _batchNotifies;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_GoldCarried)
   int32 GoldCarried = 0;

   int32 _loadoutVersion;
   int32 _nextStackId;
};

// a scope struct for batching notifications from inventory changes
struct FTATInventoryNotifyScope
{
   FTATInventoryNotifyScope(UTATItemInventoryComponent* inventory);
   ~FTATInventoryNotifyScope();

   FTATInventoryNotifyScope(const FTATInventoryNotifyScope&) = delete;
   FTATInventoryNotifyScope(FTATInventoryNotifyScope&&) = delete;

private:
   // This does not escape into the heap, so this should be fine
   UTATItemInventoryComponent* _inventory;
   bool _ownsBatch = false;
};
