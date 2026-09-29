// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "ItemInfo.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSEItems, Log, All);

class AItemActor;
class UGameplayEffect;
class UPaperSprite;
class UToolComponent;

UENUM(BlueprintType)
enum class EStackBehavior : uint8
{
   /// Does not stack. Each instance takes up one inventory slot.
   Unique,
   /// Stacks an infinite amount of times.
   Infinite,
   /// Stacks a limited amount set by MaxStackCount.
   Limited
};

UCLASS(Blueprintable, Abstract)
class OSEITEM_API UItemInfo : public UObject
{
   GENERATED_BODY()

public:
   // UObject interface
   virtual FPrimaryAssetId GetPrimaryAssetId() const override;

   static const FPrimaryAssetType PrimaryAssetType;

   /// User-facing name for this item.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   FText Name;

   /// User-facing description for this item.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   FText Description;

   /// What category of item this is.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   FGameplayTag Category;

   /// Extra metadata associated with this item.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   FGameplayTagContainer Metadata;

   /// Icon when shown in Inventory.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   UPaperSprite* Icon = nullptr;

   /// How this item stacks, if it does.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   EStackBehavior StackBehavior = EStackBehavior::Limited;

   /// Max of this item that stack in one Inventory slot.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", Meta = (ClampMin = "2", UIMin = "2", EditCondition = "StackBehavior == EStackBehavior::Limited"))
   int32 MaxStackCount = 10;

   // ItemActor to spawn when Dropped.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   TSoftClassPtr<AItemActor> ItemActor;

   /// Whether the item can be dropped.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   bool CanBeDropped = true;

   /// GameplayEffect to trigger when added to the Inventory. If persistent, removed when removed from the Inventory.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
   TSubclassOf<UGameplayEffect> OnAddEffect;

   /// GameplayEffect to trigger when Used from the Inventory.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
   TSubclassOf<UGameplayEffect> OnUseEffect;

   // Tool class to add to the ToolSet when Equipped.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tools")
   TSoftClassPtr<UToolComponent> ToolToGrant;

   // Helper to get the item actor from the item info class default object
   UFUNCTION(BlueprintCallable, Category = "Item")
   static TSoftClassPtr<AItemActor> GetItemActorFromItemInfoClass(TSubclassOf<UItemInfo> itemInfoClass);
};

