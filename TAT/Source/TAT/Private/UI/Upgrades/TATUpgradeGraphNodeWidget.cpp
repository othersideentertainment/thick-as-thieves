// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Upgrades/TATUpgradeGraphNodeWidget.h"

// tat
#include "Upgrades/TATUpgradeGraph.h"

// ose
#include "UI/OSERadialPaintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeGraphNodeWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATUpgradeGraphNodeWidget, Log, All);

void UTATUpgradeGraphNodeWidget::NativeConstruct()
{
   Super::NativeConstruct();
   SetUpgradeNode(UpgradeNode);
}

void UTATUpgradeGraphNodeWidget::NativeDestruct()
{
   Super::NativeDestruct();
}

void UTATUpgradeGraphNodeWidget::NativeTick(const FGeometry& myGeometry, float deltaTime)
{
   Super::NativeTick(myGeometry, deltaTime);
   // Unfortunately we need to do this in tick because there are some contexts in which we just don't have access to the widget bounds otherwise
   _slateRect = FSlateRect(myGeometry.LocalToAbsolute(FVector2f::Zero()), myGeometry.LocalToAbsolute(myGeometry.GetLocalSize()));
}

int32 UTATUpgradeGraphNodeWidget::NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const
{
   layerId = Super::NativePaint(args, allottedGeometry, myCullingRect, outDrawElements, layerId, widgetStyle, parentEnabled);
   if (IsDesignTime())
   {
      const_cast<UTATUpgradeGraphNodeWidget*>(this)->_slateRect = FSlateRect(
         allottedGeometry.LocalToAbsolute(FVector2f::Zero()),
         allottedGeometry.LocalToAbsolute(allottedGeometry.GetLocalSize()));
   }
   return layerId;
}

FReply UTATUpgradeGraphNodeWidget::NativeOnFocusReceived(const FGeometry& geometry, const FFocusEvent& focusEvent)
{
   OnUpgradeNodeWidgetFocused.Broadcast(this);
   return Super::NativeOnFocusReceived(geometry, focusEvent);
}

void UTATUpgradeGraphNodeWidget::SetUpgradeNode(const FOSEGenericGraphNodeHandle& upgradeNodeHandle)
{
   UpgradeNode = upgradeNodeHandle;
   UTATUpgradeGraphNode* upgradeNode = UpgradeNode.GetNode<UTATUpgradeGraphNode>();
   OnSetUpgradeNode(upgradeNode);
}

void UTATUpgradeGraphNodeWidget::SetNodeSelected(bool newSelected)
{
   if (newSelected != _isSelected)
   {
      _isSelected = newSelected;
      OnSelectionStateChanged(_isSelected);
   }
}
