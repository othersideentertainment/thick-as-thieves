// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Items/TATPickpocketableComponent.h"

// ose
#include "Items/ItemInfo.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "TATItemInfo.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATItems, Log, All);

UENUM(BlueprintType)
enum class ETATInventoryDestination : uint8
{
   /// Place in the limited size backpack.
   Backpack,
   /// Place in the unlimited size list of quest items.
   QuestItems,
   /// Place in Toolbelt if there is room, else backpack
   ToolbeltIfPossible,
   /// Convert immediately to gold based on GoldValue.
   Gold,
   // Convert immediately to upgrade currency
   UpgradeCurrency
};

UENUM(BlueprintType)
enum class ETATItemMissionInteractableRequirement : uint8
{
   InteractableAlways,
   InteractableForActiveObjective,
};

UCLASS(Blueprintable, Abstract)
class TAT_API UTATItemInfo : public UItemInfo
{
   GENERATED_BODY()

public:
   /// Whether this item should be directly added to the the stash if held when mission succeeds
   /// NOTE: this may become redundant if there are blanket categories of items that should persist
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT")
   bool KeepOnMissionSuccess;

   /// Tag that is added to ItemTags in save game if held at end of mission.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT")
   FGameplayTag ProgressionItemTag;

   /// Mission objective that will be updated when this item is picked up or dropped.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT", meta = (Categories = "MissionSystem.Objectives"))
   FGameplayTag MissionObjectiveTag;

   /// Player stat that will be updated with the number of distince items of this type
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT", meta = (Categories = "PlayerStats"))
   FGameplayTag UniquePickupPlayerStatTag;

   UPROPERTY(EditDefaultsOnly, Category = "Item|TAT")
   ETATItemMissionInteractableRequirement MissionInteractRequirement = ETATItemMissionInteractableRequirement::InteractableForActiveObjective;

   /// Where in the inventory should this item go.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT")
   ETATInventoryDestination InventoryDestination = ETATInventoryDestination::Backpack;

   /// The tag of the upgrade currency granted by taking this
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT", meta = (Categories = "UpgradeCurrencies", EditCondition = "InventoryDestination == ETATInventoryDestination::UpgradeCurrency", EditConditionHides))
   FGameplayTag UpgradeCurrencyTag;

   // the amount of upgrade currency to grant when this is taken
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT", meta = (ClampMin = "0", UIMin = "0", EditCondition = "InventoryDestination == ETATInventoryDestination::UpgradeCurrency", EditConditionHides))
   int32 UpgradeCurrencyValue = 1;

   /// How much one count of this item is worth in gold.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|TAT", Meta = (ClampMin = "0", UIMin = "0"))
   int32 GoldValue = 0;

   // Whether this item is pickpocketable.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickpocketing")
   bool IsPickpocketable = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickpocketing", meta = (EditCondition = "IsPickpocketable"))
   FPickpocketableVisuals PickpocketVisuals;

   bool ConvertsToGoldOnMissionComplete() const;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

