// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "UI/TATUserWidget.h"

// ue
#include "Blueprint/IUserObjectListEntry.h"

#include "TATThiefLootEntryWidget.generated.h"

enum class ETATLootType : uint8;
class UPaperSprite;

UCLASS(Blueprintable, BlueprintType, meta = (DisableNativeTick))
class TAT_API UTATThiefLootEntryWidget : public UTATUserWidget,
   public IUserObjectListEntry
{
   GENERATED_BODY()

   // from IUserObjectListEntry
   virtual void NativeOnListItemObjectSet(UObject* listItemObject) override;

public:
   const FTATLootInfo& GetLootInfo() const;
   const FTATLootIdentifier& GetLootIdentifier() const { return _lootInstance.Identifier; }
   const FTATLootInstance& GetLootInstance() const { return _lootInstance; }

   // Caches the loot identifier and triggers a visual refresh
   void SetLoot(const FTATLootIdentifier& lootIdentifier);
   void SetLoot(const FTATLootInstance& lootInstance);

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "Loot")
   void RefreshVisuals(const FTATLootMetadataBP& lootMetadata, int count, const FTATLootInstance& lootInstanceData);

private:
   // Loot item represented by this widget
   UPROPERTY()
   FTATLootInstance _lootInstance;
};

// Data object used to construct UTATThiefLootEntryWidgets inserted into a UListView
UCLASS(BlueprintType)
class TAT_API UTATThiefLootEntryData : public UObject
{
   GENERATED_BODY()

public:
   const FTATLootInfo& GetLootInfo() const;

public:
   UPROPERTY(BlueprintReadOnly)
   FTATLootInstance LootInstance;

   // For creating stacked entries of the same LootIdentifier
   UPROPERTY(BlueprintReadOnly)
   int Count = 1;
};
