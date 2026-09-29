// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootTypes.h"

// ue
#include "Components/ActorComponent.h"
#include "Misc/TVariant.h"
#include "ActiveGameplayEffectHandle.h"

class ATATCallingCard;
class IToolSetInterface;
class ATATLootActor;
class ATATLootBagActor;
struct FMatchPersistentData;
struct FOnAttributeChangeData;
struct FPredictionKey;
class UGameplayEffect;
class UTATLargeCarryLootToolComponent;

#include "TATLootInventory.generated.h"

UENUM(BlueprintType)
enum class ETATInventoryUpdateEventType : uint8
{
   Add,
   Remove,
   Update,
   MAX UMETA(Hidden)
};

/// Used for AuthorityMoveLootToOtherInventory
UENUM(BlueprintType)
enum class ETATInventoryMoveItemFailureMode : uint8
{
   DoNothing             UMETA(Tooltip = "No items will be added or removed if the item move fails"),
   DropOnGroundBySource  UMETA(Tooltip = "If the item move fails, the item will be dropped in front of the character who originally had the item"),
   DropOnGroundByDest    UMETA(Tooltip = "If the item move fails, the item will be dropped in front of the character who would have received the item"),
};

UCLASS()
class TAT_API UTATLootInventoryComponent : public UActorComponent
{
   GENERATED_BODY()

   UTATLootInventoryComponent();

   // From UActorComponent
   void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   void BeginPlay() override;
   void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
   static UTATLootInventoryComponent* GetForActor(AActor* actor);

   // Returns the total number of loot items held by the character.
   // Should only be called on server, or for the locally-controlled player on client (returns -1 otherwise).
   UFUNCTION(BlueprintPure, Category = "Loot")
   int GetTotalLootCount() const;

   // Returns the cumulative value of all stashed loot
   UFUNCTION(BlueprintPure, Category = "Loot")
   int GetTotalStashedLootValue() const { return _stashedLootTotalValue; }
   // Returns the cumulative value of all held loot (only valid for local player and authority)
   UFUNCTION(BlueprintPure, Category = "Loot")
   int GetTotalHeldLootValue() const { return _heldLootTotalValue; }
   // Returns the cumulative value of all held AND stashed loot (only valid for local player and authority)
   UFUNCTION(BlueprintPure, Category = "Loot")
   int GetTotalLootValue() const { return GetTotalHeldLootValue() + GetTotalStashedLootValue(); }

   UFUNCTION(BlueprintPure, Category = "Loot")
   int GetMinorLootCount() const { return _minorLoot.Num(); }
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool GetMinorLootByIndex(int32 index, FTATLootIdentifier& outLoot) const;

   UFUNCTION(BlueprintPure, Category = "Loot")
   int GetMajorLootCount() const { return _majorLoot.Num(); }
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool GetMajorLootByIndex(int32 index, FTATLootInstance& outInstance) const;
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool GetMajorLootById(const FTATLootInstanceId lootId, FTATLootInstance& outInstance) const;
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool HasMajorLoot() const { return _majorLoot.Num() > 0; }
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool HasLargeCarryLoot() const { return _largeCarriedLoot.Identifier.IsValid(); }

   UFUNCTION(BlueprintPure, Category = "Loot")
   bool IsValidLootIndex(ETATLootType lootType, int32 index) const;

   bool GetLootItemByIndex(ETATLootType lootType, int32 index, FTATLootItemVariant& outLoot) const;

   /// Gets the number of loot items of a given type held in this inventory
   UFUNCTION(BlueprintPure, Category = "Loot")
   int32 GetLootCount(ETATLootType lootType) const;

   UFUNCTION(BlueprintPure, Category = "Loot")
   int32 GetLastIndexForLootType(ETATLootType lootType) const;

   /// Gets the loot identifier of a loot item by index. Works with all loot types (including major loot).
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool GetLootIdentifierByIndex(ETATLootType lootType, int32 index, FTATLootIdentifier& outLootId) const;

   /// How full the loot bag is for VFX/SFX purposes. Returns a normalized value between 0 and 1.
   UFUNCTION(BlueprintPure, Category = "Loot")
   float GetLootBagFullness() const { return _lootBagFullness; }

   /// Returns the sum of the SlotSize values for all held inventory items
   UFUNCTION(BlueprintPure, Category = "Loot")
   int32 GetInventorySlotsUsed() const;

   /// Returns the number of available inventory slots for adding new items.
   UFUNCTION(BlueprintPure, Category = "Loot")
   int32 GetInventorySlotsFree() const;

   /// Gets the max number of inventory slots.
   /// Returns true if this inventory has a capacity limit, otherwise false.
   UFUNCTION(BlueprintPure, Category = "Loot")
   int32 GetInventorySlotMaxCapacity() const;

   /// Checks if this inventory component limits capacity based on the slot size of the loot items stored in it
   UFUNCTION(BlueprintPure, Category = "Loot")
   FORCEINLINE bool HasInventorySlotLimits() const { return UseInventorySlotMaxCapacity; }

   /// Checks if this inventory is full
   UFUNCTION(BlueprintPure, Category = "Loot")
   bool IsInventoryFull() const;

   // Returns the array of held minor loot items. Should only be called on authority, or on the owning client's machine.
   const TArray<FTATLootIdentifier>& GetMinorLoot() const;

   // Returns a reference to the major loot. Must be a valid index
   const FTATLootInstance& GetMajorLootRefByIndex(int32 index) const;

   /// Sets the toolset, and adds any tools that should be added
   void AuthoritySetToolset(TScriptInterface<IToolSetInterface> toolset);

   // Locally-predicted function that performs the pickup on server. Returns true for successful pickup
   bool HandlePickupLootItem(const FTATLootIdentifier& lootId, ATATLootActor* lootActor, FPredictionKey& predictionKey);

   // locally predict pickup of loot for capacity purposes
   void PredictAddedLoot(const FTATLootIdentifier& lootId, int32 lootCount, FPredictionKey& predictionKey);

   bool AuthorityTryPickupLootItem(const FTATLootItemVariant& lootItem, bool spawnCallingCard = false, const FTransform& callingCardTransform = FTransform());

   /// Picks up a loot item.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   bool AuthorityTryPickupLootItem(const FTATLootIdentifier& lootId);

   /// Picks up a loot instance. If this returns true, this inventory has assumed ownership over the loot instance.
   /// If this is an existing instance, you are expected to call Invalidate() on your copy of the instance.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   bool AuthorityTryPickupLootInstance(const FTATLootInstance& lootInstance);

   bool HasRoomForLoot(const FTATLootInfo& lootInfo) const;
   bool HasRoomForLoot(const FTATLootIdentifier& lootId) const;
   bool HasRoomForLoot(const FDataTableRowHandle& lootRowHandle) const;
   FORCEINLINE bool HasRoomForLoot(const FTATLootItemVariant& lootItem) const { return lootItem.IsValid() && HasRoomForLoot(lootItem.GetIdentifier()); }
   
   bool AuthorityHasLoot(const FTATLootIdentifier& lootId) const;
   int32 AuthorityCountLoot(const FTATLootIdentifier& lootId) const;

   UFUNCTION(BlueprintPure, DisplayName = "Has Room For Loot", Category = "Loot")
   inline bool BP_HasRoomForLootIdentifier(FTATLootIdentifier lootId) const { return HasRoomForLoot(lootId); }

   // Moves all loot (of the given lootType) held by the player to the provided container, updating _stashedLootTotalValue
   int AuthorityMoveLootToStash(ETATLootType lootType, TFunctionRef<void(const FTATLootItemVariant&)> deposit);
   int AuthorityMoveLootToStash(TFunctionRef<void(const FTATLootItemVariant&)> deposit);
   bool HasStashableLoot(ETATLootType lootType) const;
   bool HasStashableLoot() const;

   void SetTeam(uint8 team);

   // Moves held major loot items to another character's inventory.
   // Returns the number of items transferred
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   int32 AuthorityMoveMajorLootToOtherInventory(UTATLootInventoryComponent* otherInventory, int32 maxItemsToTransfer);

   // Moves a loot item to another character's inventory.
   // lootDroppedOnGround is set to true only if loot was removed from the inventory and NOT transferred to the other inventory.
   // Returns true if the item was successfully removed from this inventory, or false if no changes were made.
   // failureMode determines what happens if the other inventory has no room for the item
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   bool AuthorityMoveLootToOtherInventory(UTATLootInventoryComponent* otherInventory, ETATLootType lootType, int32 lootIndex,
      bool& lootDroppedOnGround, ETATInventoryMoveItemFailureMode failureMode = ETATInventoryMoveItemFailureMode::DoNothing, bool sendPickpocketToasts = false);

   // Selects a random loot item from this inventory, returning true on success.
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Loot")
   bool GetRandomLootItem(ETATLootType& outLootType, int32& outLootIndex, float minorLootChance = 1.0f, float majorLootChance = 1.0f) const;

   // Populates persistentData with the player's held loot
   void AuthorityStashDataForEscape(FMatchPersistentData& persistentData, bool bWasCaught) const;

   // Drops the player's minor loot on the ground in an ATATLootBagActor, removing the loot-bag tool
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   void AuthorityDropLootBagVoluntarily();
   void AuthorityDropLootBagInvoluntarily();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   void AuthorityDropRandomMinorLootVoluntarily();
   void AuthorityDropRandomMinorLootInvoluntarily();

   void AuthorityDropAllMajorLootInvoluntarily();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   void AuthorityDropLargeCarryLootVoluntarily();

   UFUNCTION(BlueprintPure, Category = "Loot")
   FTATLootIdentifier GetLargeCarryLootId() const { return _largeCarriedLoot.Identifier; }

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   void AuthorityDropSingleMajorLootVoluntarily(FTATLootInstanceId lootInstanceId);

   void AuthorityRemoveSingleMajorLootWithoutDropping(FTATLootInstanceId lootInstanceId);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   bool AuthorityDropSingleLootItemVoluntarily(ETATLootType lootType, int32 lootIndex);

   UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loot")
   void ServerRequestDropSingleLootItemVoluntarily(ETATLootType lootType, int32 lootIndex);

   UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loot")
   void ServerRequestDropAllLootVoluntary();

   // Drops all major/minor loot items on the ground, removing from inventory
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
   void AuthorityDropAllLootVoluntary();
   void AuthorityDropAllLootInvoluntary();

   // Drops certain types of loot when KO'd based on match settings
   void AuthorityDropLootOnKO();

   /// Called when health attribute changes, incrementing loot damage accumulator and dropping loot when exceeding threshold
   /// NB. This is intended to be called from ATATCharacter's on health change events.
   void AuthorityOnHealthChanged(const FOnAttributeChangeData& data);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLootInstanceChangeEvent, const FTATLootInstance&, oldLootInstance, const FTATLootInstance&, newLootInstance);

   /// Event fired when the major loot is picked up, dropped
   UPROPERTY(BlueprintAssignable)
   FLootInstanceChangeEvent OnMajorLootDataChanged;

   /// Event fired when any quest loot is picked up or dropped
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAuthorityOnQuestLootChanged, ETATInventoryUpdateEventType, eventType);
   UPROPERTY(BlueprintAssignable)
   FAuthorityOnQuestLootChanged OnAuthorityQuestLootChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMinorLootItemChange);
   UPROPERTY(BlueprintAssignable)
   FOnMinorLootItemChange OnMinorLootChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryUpdated, ETATInventoryUpdateEventType, eventType, ETATLootType, lootType);
   UPROPERTY(BlueprintAssignable)
   FOnInventoryUpdated OnInventoryUpdated;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLootBagFullnessChanged, float, lootBagFullness);
   UPROPERTY(BlueprintAssignable)
   FOnLootBagFullnessChanged OnLootBagFullnessChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashedLootChanged, int, newValue, int, oldValue);
   UPROPERTY(BlueprintAssignable)
   FOnStashedLootChanged OnStashedLootChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAuthorityOnLootActorPickedUp, ATATLootActor*, lootActor);
   UPROPERTY(BlueprintAssignable)
   FAuthorityOnLootActorPickedUp AuthorityOnLootActorPickedUp;

   DECLARE_MULTICAST_DELEGATE(FOnInventoryOrStashChanged);
   FOnInventoryOrStashChanged OnAuthorityInventoryOrStashChanged;
   
   DECLARE_MULTICAST_DELEGATE(FOnAuthorityLootValueChanged);
   FOnAuthorityLootValueChanged OnAuthorityLootValueChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLootValueChanged);
   UPROPERTY(BlueprintAssignable)
   FOnLootValueChanged OnLootValueChanged;

   /// When computing loot volume (e.g. for footstep noise making) for major loot, the number of slots taken up by the major loot is multiplied by this value
   /// e.g. if this is 2, then having 2 major loot taking up 4 slots each is equivalent to having 8 minor loot taking up 1 slot each for the purposes of footstep volume.
   UPROPERTY(EditDefaultsOnly, Category = "Loot", Meta = (UIMin = 0, ClampMin = 0))
   float MajorLootSlotVolumeCoefficient = 1.0f;

private:
   enum class ETATLootDropReason : uint8
   {
      Involuntary,
      Voluntary
   };

   bool _CanLootBeStashed(const FTATLootIdentifier& lootIdentifier) const;
   bool _CanLootBeStashed(const FTATLootInstance& lootInstance) const;

   // Gets the first index of a piece of major loot that can be stashed
   int _GetLastIndexOfStashableMajorLoot() const;

   // Removes the given loot item from the inventory.
   // Returns the loot identifier tag, and optionally the loot instance associated with it.
   bool _AuthorityRemoveLootItemByIndex(ETATLootType lootType, int32 lootIndex, FTATLootItemVariant* outLootItem = nullptr);

   // Removes all loot of the given type in this inventory.
   void _AuthorityRemoveAllLootOfType(ETATLootType lootType);

   struct FLootIndex
   {
      ETATLootType Type = ETATLootType::None;
      int32 Index = INDEX_NONE;
   };

   struct FLargeCarryTag {};

   using FLootDropParams = TVariant<FLootIndex, FTATLootIdentifier, FTATLootInstanceId, FLargeCarryTag>;

   // Removes the given loot item from the inventory
   // Returns the loot identifier tag, and optionally the loot instance associated with it
   bool _AuthorityRemoveLootItem(const FLootDropParams& dropParams, FTATLootItemVariant* outLootItem = nullptr);

   bool _AuthorityRemoveLargeCarryLoot(FTATLootItemVariant* outLootItem = nullptr);

   // Drops the given loot item on the ground
   void _AuthorityDropLootItem(
      const FLootDropParams& dropParams,
      ETATLootDropReason dropReason);

   // Drops the given loot instance on the ground
   inline void _AuthorityDropLootItem(const FTATLootInstanceId lootInstanceId, ETATLootDropReason dropReason)
   {
      _AuthorityDropLootItem(FLootDropParams(TInPlaceType<FTATLootInstanceId>(), lootInstanceId), dropReason);
   }

   // Drops a loot with the given identifier tag on the ground
   inline void _AuthorityDropLootItem(const FTATLootIdentifier& lootIdentifier, ETATLootDropReason dropReason)
   {
      _AuthorityDropLootItem(FLootDropParams(TInPlaceType<FTATLootIdentifier>(), lootIdentifier), dropReason);
   }

   void _AuthorityDropLargeCarry(ETATLootDropReason dropReason)
   {
      _AuthorityDropLootItem(FLootDropParams(TInPlaceType<FLargeCarryTag>()), dropReason);
   }

   void _AuthorityDropLootByIndex(ETATLootType lootType, int32 index, ETATLootDropReason dropReason);

   void _AuthorityDropRandomLoot(ETATLootType lootType, ETATLootDropReason dropReason);

   void _AuthorityDropLootBag(ETATLootDropReason dropReason);

   // Spawns a ATATLootBag actor containing the specified loot
   bool _AuthoritySpawnLootBagActor(TSubclassOf<ATATLootBagActor> lootBagActorClass, TConstArrayView<FTATLootIdentifier> lootIdentifiers, TConstArrayView<FTATLootInstance> lootInstances, ETATLootDropReason dropReason) const;

   // Spawns all loot on the ground around the owner. Called when owner is KO'd.
   void _AuthoritySpawnLootPinata(TConstArrayView<FTATLootIdentifier> lootIdentifiers, TConstArrayView<FTATLootInstance> lootInstances) const;

   void _AuthoritySpawnCallingCard(TSubclassOf<ATATCallingCard> callingCardActorClass, const FTransform& spawnTransform, const FTATLootItemVariant& lootItem, const FTATLootInfo& lootInfo);

   void _AuthorityOnLootAdded(const FTATLootIdentifier& lootId);
   void _AuthorityOnLootRemoved(const FTATLootIdentifier& lootId);

   // Called after major loot is added (server only)
   void _AuthorityOnMajorLootAdded(FTATLootInstance& addedLoot);
   // Called after major loot is removed (server only)
   void _AuthorityOnMajorLootRemoved(const FTATLootInstance& removedLoot);

   void _TrySendMajorLootDroppedToast(const FTATLootInstance& removedLoot);

   void _OnMajorLootAdded(FTATLootInstance& addedLoot);
   void _OnMajorLootRemoved(const FTATLootInstance& removedLoot);

   void _OnLargeCarryLootAdded(const FTATLootInstance& addedLoot);
   void _OnLargeCarryLootRemoved(const FTATLootInstance& removedLoot);
   void _AddLargeCarryTool();
   void _AuthorityFinishAddingLargeCarryTool(TSoftClassPtr<UTATLargeCarryLootToolComponent> toolClass, FTATLootIdentifier lootId);
   void _RemoveLargeCarryTool();

   // TODO: make more granular?
   void _NotifyInventoryChanged();
   void _FlushNotifies();
   void _FireCoarseEvents();

   // Adds or removes major loot-related gameplay tags
   void _UpdateMajorLootGameplayTags(bool majorLootAdded);

   bool _IncrementPredictedLoot(const FTATLootIdentifier& lootId, int32 delta);
   void _OnPredictedLootCatchup(FTATLootIdentifier lootId, int32 amount);

   bool _IsQuestLoot(const FTATLootIdentifier& lootId) const;

   // Sets a timer to spawn an instance of _carriedMajorLootTransientGlyphClass at an interval matching its InitialLifeSpan
   void _AuthoritySetCarriedMajorLootGlyphActorSpawnTimer();

   UFUNCTION()
   void _AuthoritySpawnCarriedMajorLootGlyphActor();
   
   UFUNCTION()
   void _AuthorityResetLootDamageAccumulator();

   void _OnMinorLootChanged(ETATInventoryUpdateEventType eventType);
   void _RefreshLootBagFullness();
   void _RefreshLootBagFullnessIfAuthorityOrLocalPlayer();
   void _RefreshHeldLootValue();

   UFUNCTION()
   void _OnRep_MinorLoot(const TArray<FTATLootIdentifier>& oldValue);
   UFUNCTION()
   void _OnRep_MajorLoot(const TArray<FTATLootInstance>& oldValue);
   UFUNCTION()
   void _OnRep_LargeCarriedLoot(const FTATLootInstance& oldValue);
   UFUNCTION()
   void _OnRep_LootBagFullness();

   // Since the loot drop involves an async load, we split the position calculations into two phases:
   //  - _GenerateSuggestedLootDropLocation is done synchronously, assuming that the owning pawn is still valid
   //  - _GenerateDroppedLootSpawnLocation finalizes the position once the loot actor has been loaded,
   //    using the drop position suggested by _GenerateSuggestedLootDropLocation. The owning pawn may be invalid at this point
   FVector _GenerateSuggestedLootDropLocation(const APawn* ownerPawn, ETATLootDropReason dropReason) const;
   FVector _GenerateDroppedLootSpawnLocation(const AActor* lootActor, const FVector& suggestedDropStart, ETATLootDropReason dropReason) const;

   void _AddMajorLootGameplayEffect();
   void _RemoveMajorLootGameplayEffect();

   APawn* _GetOwningPawn() const;
   void _UpdateTeamStashedValue(int32 newStashedValue);

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (InlineEditConditionToggle))
   bool UseInventorySlotMaxCapacity = false;
   UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (EditCondition = "UseInventorySlotMaxCapacity", UIMin = "0", ClampMin = "0"))
   int32 InventorySlotMaxCapacity = 16;

   UPROPERTY(EditDefaultsOnly, Category="Loot")
   TSoftClassPtr<ATATLootBagActor> _lootBagActorClass;

   UPROPERTY(EditDefaultsOnly, Category="Loot")
   TSoftClassPtr<ATATCallingCard> _callingCardActorClass;

private:
   UPROPERTY(ReplicatedUsing=_OnRep_MinorLoot, Transient)
   TArray<FTATLootIdentifier> _minorLoot;

   UPROPERTY(ReplicatedUsing=_OnRep_MajorLoot, Transient)
   TArray<FTATLootInstance> _majorLoot;

   UPROPERTY(ReplicatedUsing=_OnRep_LargeCarriedLoot, Transient)
   FTATLootInstance _largeCarriedLoot;

   UPROPERTY(Transient)
   TScriptInterface<IToolSetInterface> _toolset;

   // Normalized value replicated to non-owning clients for visualizing how full the loot bag is. Owning clients self-compute this using the _minorLoot collection
   UPROPERTY(ReplicatedUsing=_OnRep_LootBagFullness, Transient)
   float _lootBagFullness;

   // Cached for the total value of loot stashed by the team
   UPROPERTY(Transient)
   int _stashedLootTotalValue;

   // Cached proxy value for the total value of loot held by the player
   UPROPERTY(Transient)
   int _heldLootTotalValue;

   // Range around the player that loot will be dropped when pinata-dropping (uses a random value between min and max)
   UPROPERTY(EditAnywhere, Category = "Loot|Drop", Meta = (ClampMin = "0", UIMin = "1"))
   FFloatInterval _pinataRadiusRange{ 50.0f, 100.0f };

   // Range in from of the player dropped loot should spawn in when dropping voluntarily
   UPROPERTY(EditAnywhere, Category = "Loot|Drop", Meta = (ClampMin = "0", UIMin = "0"))
   float _voluntaryDroppedLootSpawnRange = 100.f;

   // Radius around the player dropped loot should spawn in when dropping involuntarily
   UPROPERTY(EditAnywhere, Category = "Loot|Drop", Meta = (ClampMin = "0", UIMin = "0"))
   float _involuntaryDroppedLootSpawnRadius = 30.f;

   UPROPERTY(EditAnywhere, Category = "Loot|Drop", Meta = (ClampMin = "0.01", UIMin = "0.01"))
   float _lootDropDamageThresholdResetDuration = 5.f;

   // Threshold that determines how much damage the player should take before dropping a loot item.
   UPROPERTY(EditAnywhere, Category = "Loot|Drop", Meta = (ClampMin = "0", UIMin = "0"))
   float _lootDropDamageThreshold = 35.f;

   UPROPERTY(EditAnywhere, Category = "Loot|Drop", Meta = (ClampMin = "0", UIMin = "0"))
   bool _isAllowedToEverDropItemFromDamage { false };
   
   // Keeps track of damage taken, so we can trigger a loot-drop event when it exceeds _lootDropDamageThreshold
   float _authorityLootDropDamageAccumulator = 0.f;

   // Keeps track of time between damage events, resetting _authorityLootDropDamageAccumulator if no damage is received for some amount of time
   FTimerHandle _authorityLootDamageEventTimer;

   int32 _predictedAdditionalUsedSlots = 0;

   friend struct FTATLootInventoryNotifyScope;
   // TODO: add bitflags for more batching more granular notifies? (a la TATItemInventoryComponent)
   bool _batchNotifies;
   bool _hasBatchedChanges = false;

   // True if in the middle of dropping something
   bool _authorityDropping = false;

   // Thief vision indicator type to spawn on a periodic basis (_glyphSpawnPeriod + glyph's InitialLifeSpan) while carrying major loot
   UPROPERTY(EditAnywhere, Category = "Loot|Glyph", Meta = (Categories = "Indicator"))
   FGameplayTag _carriedMajorLootGlyphIndicatorType;

   // Period of time after the last spawned glyph's lifespan elapsed in which to spawn another
   UPROPERTY(EditAnywhere, Category = "Loot|Glyph", meta = (UIMin = "0.0", ClampMin = "0.0"))
   float _glyphSpawnDelaySeconds = 0.f;

   // Handle used for the timer driving the spawning of a transient glyph actor every X seconds while holding major loot
   FTimerHandle _carriedMajorLootGlyphSpawnTimerHandle;

   // Cached gameplay effect reference - this is loaded from a soft pointer in loot settings
   UPROPERTY(Transient)
   TSubclassOf<UGameplayEffect> _gameplayEffectToApplyWhenHoldingMajorLoot;

   struct FLootEffectEntry
   {
      int32 Count = 0;
      FActiveGameplayEffectHandle EffectHandle;
   };
   TMap<FTATLootIdentifier, FLootEffectEntry> _authorityLootEffects;

   uint8 _team;
};


// a scope struct for batching notifications from inventory changes
struct FTATLootInventoryNotifyScope
{
   FTATLootInventoryNotifyScope(UTATLootInventoryComponent* inventory);
   ~FTATLootInventoryNotifyScope();

   UE_NONCOPYABLE(FTATLootInventoryNotifyScope);

private:
   // This does not escape into the heap, so this should be fine
   UTATLootInventoryComponent* _inventory;
   bool _ownsBatch = false;
};
