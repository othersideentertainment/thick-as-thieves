// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "UI/TATUserWidget.h"

// ose
#include "UI/OSERadialPaintLibrary.h"
#include "OSEGenericGraph.h"

#include "TATUpgradeGraphWidget.generated.h"

class UPaperSprite;
class UTATUpgradeGraph;
class UTATUpgradeGraphNode;
class UTATUpgradeGraphNodeWidget;
class UWidgetTree;
class IWidgetCompilerLog;

/// Widget for displaying and editing upgrade graphs
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATUpgradeGraphWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual int32 NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements,
      int32 layerId, const FWidgetStyle& inWidgetStyle, bool parentEnabled) const override;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUpgradeSelectionEvent, UTATUpgradeGraphWidget*, graphWidget, UTATUpgradeGraphNodeWidget*, nodeWidget);
   UPROPERTY(BlueprintAssignable)
   FUpgradeSelectionEvent OnUpgradeNodeSelected;

protected:
   UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
   TObjectPtr<UPanelWidget> UpgradeEntryContainer;

   UFUNCTION(BlueprintNativeEvent, Category = "Upgrade Graph Widget")
   bool ShouldAutoCreateWidgetForNode(UTATUpgradeGraphNode* node) const;
   virtual bool ShouldAutoCreateWidgetForNode_Implementation(UTATUpgradeGraphNode* node) const;

   UFUNCTION(BlueprintNativeEvent, Category = "Upgrade Graph Widget")
   void OnAutoCreatedWidgetForUpgradeNode(UTATUpgradeGraphNode* node, UTATUpgradeGraphNodeWidget* nodeWidget);
   virtual void OnAutoCreatedWidgetForUpgradeNode_Implementation(UTATUpgradeGraphNode* node, UTATUpgradeGraphNodeWidget* nodeWidget);

   UFUNCTION(BlueprintNativeEvent, Category = "Upgrade Graph Widget")
   void OnUpgradeNodeWidgetSelected(UTATUpgradeGraphNodeWidget* nodeWidget);
   virtual void OnUpgradeNodeWidgetSelected_Implementation(UTATUpgradeGraphNodeWidget* nodeWidget) {}

   UFUNCTION(BlueprintCallable, Category = "Upgrade Graph Widget")
   void SelectFirstUpgradeNodeWidget();

   UFUNCTION(BlueprintCallable, Category = "Upgrade Graph Widget")
   void RefreshAllUpgradeNodeWidgets();

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Widget")
   UTATUpgradeGraphNodeWidget* GetWidgetForNode(UTATUpgradeGraphNode* node) const;

   UFUNCTION(BlueprintCallable, Category = "Upgrade Graph Widget")
   void SetUpgradeNodeSelected(UTATUpgradeGraphNode* node) { SetUpgradeNodeWidgetSelected(GetWidgetForNode(node)); }

   UFUNCTION(BlueprintCallable, Category = "Upgrade Graph Widget")
   void SetUpgradeNodeWidgetSelected(UTATUpgradeGraphNodeWidget* nodeWidget);

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Upgrade Graph Widget", Meta = (ExposeOnSpawn))
   TObjectPtr<UTATUpgradeGraph> UpgradeGraph;

   /// If enabled, any nodes that have not been manually added to the container will be auto-created
   /// Note that enabling this requires UpgradeEntryContainer to be bound to a valid container widget (newly created widgets will be added there)
   /// See also: ShouldAutoCreateWidgetForNode and OnAutoCreatedWidgetForUpgradeNode
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Upgrade Graph Widget")
   bool AutoCreateWidgetsForMissingNodes = false;

   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Upgrade Graph Widget", Meta = (EditCondition = "AutoCreateWidgetsForMissingNodes"))
   TSubclassOf<UTATUpgradeGraphNodeWidget> UpgradeNodeWidgetClass;

   /// How to traverse the graph when adding missing nodes
   UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Upgrade Graph Widget", Meta = (EditCondition = "AutoCreateWidgetsForMissingNodes"))
   EOSEGenericGraphSearchMode AutoCreateTraverseOrder = EOSEGenericGraphSearchMode::DepthFirstSearch;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrows")
   bool DrawArrowsForDependentNodes = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrows", Meta = (EditCondition = "DrawArrowsForDependentNodes"))
   FOSEArrowStyle ArrowStyle;

   /// How far into the node widget the arrow should draw (line overlap amount)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrows", Meta = (EditCondition = "DrawArrowsForDependentNodes"))
   float ArrowLineOffsetFromNodeWidget = 35.0f;

public:
   void ForEachGraphNodeWidget(TFunctionRef<void(UTATUpgradeGraphNodeWidget*)> callback) const;

#if WITH_EDITOR
   // from UUserWidget
   virtual void ValidateCompiledWidgetTree(const UWidgetTree& blueprintWidgetTree, IWidgetCompilerLog& compileLog) const override;
#endif // WITH_EDITOR

private:
   UFUNCTION()
   void _OnUpgradeNodeWidgetFocused(UTATUpgradeGraphNodeWidget* nodeWidget);

   void _SetSelectedNodeWidget(UTATUpgradeGraphNodeWidget* nodeWidget);

   TMap<TWeakObjectPtr<UTATUpgradeGraphNode>, TWeakObjectPtr<UTATUpgradeGraphNodeWidget>> _nodeWidgetMap;
};
