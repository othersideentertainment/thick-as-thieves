// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Loot/TATLootTypes.h"
#include "GameFramework/TATDifficulty.h"

// ue4
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

#include "TATLootSettings.generated.h"

class ATATTransientGlyphActor;
class UTATMapActorComponent;
class UGameplayEffect;
class UTATLargeCarryLootToolComponent;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Loot Settings"))
class TAT_API UTATLootSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UTATLootSettings();

   static const UTATLootSettings& Get() { return *GetDefault<UTATLootSettings>(); }

   // Performs a lookup of loot with matching FTATLootIdentifier in _lootDataTable. Returns nullptr if not found.
   // Similar to GetLootInfo, but does not assume the identifier is valid and does not log warnings for failed lookups or invalid identifiers.
   const FTATLootInfo* FindLootInfo(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const;

   // Performs a lookup of loot with matching FTATLootIdentifier in _lootDataTable. Returns nullptr and adds warnings to the log if not found.
   const FTATLootInfo* GetLootInfo(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const;
   const FTATLootInfo* GetLootInfo(const FDataTableRowHandle& lootRowHandle) const;

   // Gets a reference to a loot info data table row. Requires the loot identifier to be valid - crashes if not valid.
   const FTATLootInfo& GetLootInfoChecked(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const;

   FGameplayTag GetLootPickupGameplayCue(const FTATLootInfo& lootInfo) const;
   FGameplayTag GetLootPickupGameplayEventTag(const FTATLootInfo& lootInfo) const;

   // Returns the slot size of the associated loot, or NullOpt if the loot identifier is invalid
   TOptional<int32> GetSlotSize(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const;

   /// Returns the combined slot size of the specified loot
   int32 GetSlotSizeCombined(const UObject* contextObject, TConstArrayView<FTATLootIdentifier> lootIds) const;
   int32 GetSlotSizeCombined(const UObject* contextObject, TConstArrayView<FTATLootInstance> lootInstances) const;
   int32 GetSlotSizeCombined(const UObject* contextObject, TConstArrayView<FTATLootItemVariant> lootItems) const;

   float GetLootMultiplierForCurrentDifficulty(const UObject* contextObject) const;
   // Returns the value of the associated loot, or 0 if the loot identifier is invalid.
   int32 GetLootValue(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier, const bool shouldUseDifficultyMultiplier = true) const;
   int32 GetLootValue(const UObject* contextObject, const FTATLootInstance& lootInstance, const bool shouldUseDifficultyMultiplier = true) const;
   int32 GetLootValue(const UObject* contextObject, TConstArrayView<FTATLootIdentifier> lootIdentifiers) const;
   int32 GetLootValue(const UObject* contextObject, TConstArrayView<FTATLootInstance> lootInstances) const;
   int32 GetLootValue(const UObject* contextObject, const FTATLootContainer& lootContainer) const;

   bool DoesAutoConvertToMoney(const UObject* contextObject, const FTATLootIdentifier& lootIdentifier) const;

   const float GetLootActorLifetimeAfterPickup() const { return _lootActorLifetimeAfterPickup; }

public:
   // Interact prompt shown for picking up a loot item (when inventory has room)
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Prompts")
   FText PromptTakeOne;

   // Interact prompt shown for picking up minor loot from a KO'd player's loot bag
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Prompts")
   FText PromptTakeLootBagContents;

   // Interact prompt shown for picking up a loot item (when inventory is full)
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Prompts")
   FText PromptInventoryFull;

   // Interact prompt shown for picking up a loot item (when the item does not fit in an inventory but it's not full)
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Prompts")
   FText PromptItemDoesNotFitInInventory;

   // Interact prompt shown for stash when depositing loot into a stash
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Prompts")
   FText PromptStashUse;

   // Interact prompt shown for stash when player has no loot to deposit
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Prompts")
   FText PromptStashNoLootToDeposit;

   // Gameplay cues for loot pickup
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Gameplay Cues", Meta = (Categories = "GameplayCue"))
   FGameplayTag MajorLootPickupGameplayCue;

   UPROPERTY(Config, EditAnywhere, Category = "Interact|Gameplay Cues", Meta = (Categories = "GameplayCue"))
   FGameplayTag MinorLootPickupGameplayCue;

   // Gameplay events for loot pickup
   UPROPERTY(Config, EditAnywhere, Category = "Interact|Events")
   FGameplayTag MajorLootPickupGameplayEventTag;

   UPROPERTY(Config, EditAnywhere, Category = "Interact|Events")
   FGameplayTag MinorLootPickupGameplayEventTag;

   UPROPERTY(Config, EditAnywhere, Category = "Interact|Events")
   FGameplayTag LootBagPickupGameplayEventTag;

   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TSoftObjectPtr<const UDataTable> LootDataTable = nullptr;

   /// Per-loot-type default slot sizes (individual loot types can override this - see SlotSizeOverride)
   UPROPERTY(Config, EditAnywhere, Category = "Loot", meta = (UIMin = "1", ClampMin = "1", ArraySizeEnum = "/Script/TAT.ETATLootType"))
   int32 DefaultLootSlotSize[static_cast<uint32>(ETATLootType::MAX)] = { 1 };

   // UTATMapActorComponent class attached to dropped major loot, allowing it to be highlighted it on the map.
   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TSoftClassPtr<UTATMapActorComponent> DroppedMajorLootMapActorClass = nullptr;

   // UTATMapActorComponent class attached to dropped quest loot, allowing it to be highlighted it on the map.
   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TSoftClassPtr<UTATMapActorComponent> DroppedQuestLootMapActorClass = nullptr;

   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TSoftClassPtr<ATATTransientGlyphActor> DefaultMajorLootTransientGlyphClass = nullptr;

   /// Tags to grant to the character when they are holding a major loot item
   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   FGameplayTagContainer CharacterGrantedTagsWhenHoldingMajorLoot;

   /// Gameplay effect to apply to the character when they are holding a major loot item.
   /// The effect will be removed when they are no longer holding any major loot.
   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TSoftClassPtr<UGameplayEffect> GameplayEffectToApplyWhenHoldingMajorLoot;

   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TSoftClassPtr<UTATLargeCarryLootToolComponent> LargeCarryLootToolClass;

   UPROPERTY(Config, EditAnywhere, Category = "Loot")
   TMap<ETATDifficulty, float> DifficultyToLootMultiplier;
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "Pickup")
   float TakeSpeed = 12.0f;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Pickup")
   float CloseDistance = 10.0f;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Pickup", meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag TakeInteractAnimationTag;

private:
   const UDataTable* _GetLootDataTable(const UObject* contextObject) const;

private:

   // Delay after pickup input before the actor is destroyed. Useful for timing destruction with a lerp-to-player.
   UPROPERTY(Config, EditAnywhere, Category = "Loot", Meta = (ClampMin = "0.01", UIMin = "0.01"))
   float _lootActorLifetimeAfterPickup = 0.05f;

};
