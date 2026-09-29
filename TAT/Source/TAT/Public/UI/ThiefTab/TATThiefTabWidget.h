// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "UI/TATUserWidget.h"
#include "UI/ThiefTab/TATThiefLootEntryMgr.h"
#include "UI/ThiefTab/TATThiefToolEntryMgr.h"

// ue
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

#include "TATThiefTabWidget.generated.h"

class ATATPlayerState;
class UTATLootInventoryComponent;
class UTATThiefToolEntryWidget;
class UTATToolSetComponent;
class UToolSetComponent;
class UToolComponent;

UCLASS(Blueprintable, meta = (DisableNativeTick))
class TAT_API UTATThiefTabWidget : public UTATUserWidget
{
   GENERATED_BODY()

   UTATThiefTabWidget();

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif //WITH_EDITOR

public:
   // from UUserWidget
   virtual void NativeDestruct() override;
   virtual void _OnLocalPlayerStateAdded_Implementation(ATATPlayerState* ps) override;

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Gear")
   void AddToolEntryWidgetToLayout(UTATThiefToolEntryWidget* toolEntryWidget);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Gear")
   void RemoveToolEntryWidgetFromLayout(UTATThiefToolEntryWidget* toolEntryWidget);

   UFUNCTION(BlueprintImplementableEvent, BlueprintPure, Category = "TAT|Loot")
   UListView* GetListViewForLoot(ETATLootType lootType) const;

private:
   // Removes all loot entry widgets of the given loot type
   void _ClearLootEntryWidgetsFromLayout(ETATLootType lootType);
   // Removes all loot entry widgets
   void _ClearAllLootEntryWidgetsFromLayout();
   // Removes all tool entry widgets
   void _ClearToolEntryWidgetsFromLayout();

   // Creates entry widgets for each loot item held by the player, adding them to the layout
   void _ConstructLootEntryWidgets(ETATLootType lootType);
   // Creates entry widgets for each tool item held by the player, adding them to the layout
   void _ConstructToolEntryWidgets();
   
   void _BindToLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent);
   void _UnbindFromLootInventoryComponent(UTATLootInventoryComponent* lootInventoryComponent);
   void _BindToToolsetComponent(UToolSetComponent* toolsetComponent);
   void _UnbindFromToolsetComponent(UToolSetComponent* toolsetComponent);

   UFUNCTION()
   void _OnMinorLootChanged();
   UFUNCTION()
   void _OnMajorLootChanged(const FTATLootInstance& oldMajorLoot, const FTATLootInstance& newMajorLoot);

   UFUNCTION()
   void _OnToolAdded(UToolComponent* toolComponent);
   UFUNCTION()
   void _OnToolRemoved(UToolComponent* toolComponent);

   bool _ShouldShowTool(const UToolComponent* toolComponent) const;

protected:
   // Class used for constructing tool entry widgets
   UPROPERTY(EditDefaultsOnly, Category = "Tools")
   TSubclassOf<UTATThiefToolEntryWidget> ToolEntryWidgetClass;

   // Tool's FOSEAbilityInfo::ToolCategory must match this tag (or derive from it) to be represented in the UI
   UPROPERTY(EditDefaultsOnly, Category = "Tools")
   FGameplayTagQuery ToolRequiredCategoryQuery;

private:
   UPROPERTY()
   FTATThiefToolEntryMgr _toolEntryMgr;

   UPROPERTY()
   FTATThiefLootEntryMgr _lootEntryMgr;

   UPROPERTY(Transient)
   TWeakObjectPtr<UTATToolSetComponent> _toolSetComponent = nullptr;

   UPROPERTY(Transient)
   TWeakObjectPtr<UTATLootInventoryComponent> _lootInventoryComponent = nullptr;
};

