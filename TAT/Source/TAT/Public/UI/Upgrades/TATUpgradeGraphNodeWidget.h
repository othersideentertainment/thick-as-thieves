// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "UI/TATUserWidget.h"

// ose
#include "OSEGenericGraphNodeHandle.h"

#include "TATUpgradeGraphNodeWidget.generated.h"

class UTATUpgradeGraph;
class UTATUpgradeGraphNode;
class UPaperSprite;

/// Widget for a single upgrade in an upgrade graph
UCLASS()
class TAT_API UTATUpgradeGraphNodeWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;
   virtual void NativeTick(const FGeometry& myGeometry, float deltaTime) override;
   virtual int32 NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const override;
   virtual FReply NativeOnFocusReceived(const FGeometry& geometry, const FFocusEvent& focusEvent) override;

   UFUNCTION(BlueprintNativeEvent, Category = "Upgrade Graph Node Widget")
   void OnSetUpgradeNode(UTATUpgradeGraphNode* upgradeNode);
   virtual void OnSetUpgradeNode_Implementation(UTATUpgradeGraphNode* upgradeNode) {}

   UFUNCTION(BlueprintNativeEvent, Category = "Upgrade Graph Node Widget")
   void OnSelectionStateChanged(bool newSelected);
   virtual void OnSelectionStateChanged_Implementation(bool newSelected) {}

   /// Update the state of the upgrade node (eg. if we just unlocked the upgrade)
   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Upgrade Graph Node Widget")
   void RefreshUpgradeState();
   virtual void RefreshUpgradeState_Implementation() {}

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Upgrade Graph Node Widget", Meta = (GraphType = "/Script/TAT.TATUpgradeGraph", ExposeOnSpawn))
   FOSEGenericGraphNodeHandle UpgradeNode;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUpgradeNodeWidgetFocusEvent, UTATUpgradeGraphNodeWidget*, nodeWidget);
   UPROPERTY(BlueprintAssignable)
   FUpgradeNodeWidgetFocusEvent OnUpgradeNodeWidgetFocused;

   UFUNCTION(BlueprintCallable, Category = "Upgrade Graph Node Widget")
   void SetUpgradeNode(const FOSEGenericGraphNodeHandle& upgradeNodeHandle);

   UFUNCTION(BlueprintPure, Category = "Upgrade Graph Node Widget")
   bool IsNodeSelected() const { return _isSelected; }

   UFUNCTION(BlueprintCallable, Category = "Upgrade Graph Node Widget")
   void SetNodeSelected(bool newSelected);

   FSlateRect GetSlateRect() const { return _slateRect; }

private:
   bool _isSelected = false;
   FSlateRect _slateRect;

};
