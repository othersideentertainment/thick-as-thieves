// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATLayoutWidget.h"

// common
#include "CommonActivatableWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLayoutWidget)

UE_DEFINE_GAMEPLAY_TAG(Tag_UI_Layer_Game, "UI.Layer.Game");
UE_DEFINE_GAMEPLAY_TAG(Tag_UI_Layer_Menu, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG(Tag_UI_Layer_Modal, "UI.Layer.Modal");

void UTATLayoutWidget::_RegisterLayer(FGameplayTag layer, UCommonActivatableWidgetContainerBase* widget)
{
   if (IsDesignTime())
   {
      return;
   }

   _layers.Add(layer, widget);
}

bool UTATLayoutWidget::PopWidgetsByClass(FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass) const
{
   TObjectPtr<UCommonActivatableWidgetContainerBase> layerWidget = _layers.FindRef(layer);
   if (!layerWidget)
   {
      return false;
   }

   bool successfullyRemovedWidgets = false;

   // Only collect widgets we actually intend to remove
   TArray<UCommonActivatableWidget*, TInlineAllocator<2>> widgetsToRemove;
   for (UCommonActivatableWidget* widget : layerWidget->GetWidgetList())
   {
      if (widget && widget->IsA(widgetClass))
      {
         widgetsToRemove.Add(widget);
      }
   }

   for (UCommonActivatableWidget* widget : widgetsToRemove)
   {
      layerWidget->RemoveWidget(*widget);
      successfullyRemovedWidgets = true;
   }

   return successfullyRemovedWidgets;
}

bool UTATLayoutWidget::IsAnyWidgetActive() const
{
   for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& layerPair : _layers)
   {
      if(layerPair.Value->GetActiveWidget() != nullptr)
      {
         return true;
      }
   }

   return false;
}

UCommonActivatableWidget* UTATLayoutWidget::GetActiveWidgetOfLayer(FGameplayTag layer) const
{
   for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& layerPair : _layers)
   {
      if(layerPair.Key == layer)
      {
         return layerPair.Value->GetActiveWidget();
      }
   }

   return nullptr;
}

bool UTATLayoutWidget::PopWidget(UCommonActivatableWidget* widget) const
{
   if (!widget)
   {
      return false;
   }

   for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& layerPair : _layers)
   {
      const TArray<UCommonActivatableWidget*>& layerWidgets = layerPair.Value->GetWidgetList();
      if (layerWidgets.Contains(widget))
      {
         layerPair.Value->RemoveWidget(*widget);
         return true;
      }
   }

   return false;
}
