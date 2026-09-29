// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Upgrades/TATUpgradeCurrency.h"
#include "Upgrades/TATUpgradeType.h"

// ose
#include "OSEGenericGraph.h"

// ue
#include "CoreMinimal.h"

#include "TATUpgradeGraph.generated.h"

class UTATSaveGame;
class UTATUpgradeGraphWidget;

UCLASS()
class TAT_API UTATUpgradeGraphEdge : public UOSEGenericGraphEdge
{
   GENERATED_BODY()

public:
   UTATUpgradeGraphEdge();
};


UCLASS()
class TAT_API UTATUpgradeGraphNode : public UOSEGenericGraphNode
{
   GENERATED_BODY()

public:
   UTATUpgradeGraphNode();

#if WITH_EDITOR
   // From UObject
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
   TObjectPtr<UTATUpgradeType> UpgradeType;

   /// The upgrade's level. Players can only unlock upgrades with levels higher than their current level for that tag.
   UPROPERTY(EditAnywhere, Category = "Output", Meta = (UIMin = 1, ClampMin = 1, UIMax = 10))
   int32 UpgradeLevel = 1;

   /// If greater than zero, the money required to unlock this upgrade
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements", Meta = (UIMin = 0, ClampMin = 0))
   int32 MoneyCost = 0;

   /// All additional currencies required to unlock this upgrade
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
   TArray<FTATUpgradeCurrencyCost> Cost;

   UPROPERTY(EditAnywhere, Category = "Display", Meta = (InlineEditConditionToggle))
   bool UseNameOverride = false;

   UPROPERTY(EditAnywhere, Category = "Display", Meta = (EditCondition = "UseNameOverride"))
   FText NameOverride;

   UPROPERTY(EditAnywhere, Category = "Display", Meta = (InlineEditConditionToggle))
   bool UseDescriptionOverride = false;

   UPROPERTY(EditAnywhere, Category = "Display", Meta = (EditCondition = "UseDescriptionOverride", MultiLine = true))
   FText DescriptionOverride;

   UPROPERTY(EditAnywhere, Category = "Display", Meta = (InlineEditConditionToggle))
   bool UseIconOverride = false;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display", Meta = (EditCondition = "UseIconOverride"))
   TSoftObjectPtr<UPaperSprite> IconOverride;

   /// Should this node be disabled in the UI?
   /// Note that this doesn't prevent the upgrade from being unlocked, it just tells the UI widgets to show it as grayed out and disables the unlock button.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display")
   bool DisabledInUI = false;

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node")
   FText GetUpgradeName() const;

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node")
   FText GetUpgradeDescription() const;

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node")
   TSoftObjectPtr<UPaperSprite> GetUpgradeIcon() const;

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node")
   FGameplayTag GetUpgradeTag() const;

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node")
   int32 GetUpgradeLevel() const;

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node")
   ETATUpgradeProgressionType GetUpgradeProgressionType() const;

#if WITH_EDITOR
   // From UOSEGenericGraphNode
   virtual void GetCustomGraphContextMenuActions(TArray<FOSEGenericGraphNodeCustomAction>& outActions) const override;
   virtual EOSEGenericGraphNodeStyle GetNodeStyle() const override;
   virtual FLinearColor GetBackgroundColor() const override;
   virtual const FSlateBrush* GetNodeIcon() const override;
   virtual FText GetNodeDisplayTitle() const override;
   virtual FText GetNodeDisplaySubtitle() const override;
   virtual FText GetNodeListViewTitle() const override;
   virtual bool IsTitleEditable() const override { return true; }
   virtual FText GetEditableTitle() const override;
   virtual void SetEditableTitle(const FText& newName) override;
   virtual void OnNodeDoubleClicked() override;
   virtual TSharedPtr<SWidget> ConstructNodeBodyWidget() override;
#endif

private:
#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   FSlateBrush _iconBrush;
#endif
};


UCLASS()
class TAT_API UTATUpgradeGraph : public UOSEGenericGraph
{
   GENERATED_BODY()

public:
   UTATUpgradeGraph();

   /// Finds the graph node with the specified upgrade tag and level.
   /// Note that upgrade graphs require each node to have a unique UpgradeTag/UpgradeLevel pair.
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Upgrade Graph")
   UTATUpgradeGraphNode* FindUpgradeNode(FGameplayTag upgradeTag, int32 level) const;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   /// What widget to load for players to unlock upgrades in this graph
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
   TSoftClassPtr<UTATUpgradeGraphWidget> UpgradeWidget;
};
