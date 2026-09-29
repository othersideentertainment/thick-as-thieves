// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <CommonUserWidget.h>
#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>
#include <Widgets/CommonActivatableWidgetContainer.h>

#include "TATLayoutWidget.generated.h"

class UCommonActivatableWidget;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_UI_Layer_Game);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_UI_Layer_Menu);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_UI_Layer_Modal);

UCLASS(MinimalAPI, Abstract, meta=(DisableNativeTick))
class UTATLayoutWidget : public UCommonUserWidget
{
   GENERATED_BODY()

public:
   template<typename T = UCommonActivatableWidget>
   T* PushWidget(FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass) const;

   template<typename T = UCommonActivatableWidget>
   T* PushWidget(FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass, TFunctionRef<void(T& widget)> initFunc) const;

   bool PopWidget(UCommonActivatableWidget* widget) const;

   bool PopWidgetsByClass(FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass) const;

   // The following functions are exposed for the purposes of tutorial instrumentation, and may not be fast
   // NOTE: current caller assumes that any active widget is modal/screen-like, which is true currently,
   //       but may not definitely remain so
   bool IsAnyWidgetActive() const;

   UFUNCTION(BlueprintCallable)
   UCommonActivatableWidget* GetActiveWidgetOfLayer(UPARAM(meta=(Categories = "UI.Layer")) FGameplayTag layer) const;

private:
   UFUNCTION(BlueprintCallable)
   void _RegisterLayer(UPARAM(meta=(Categories = "UI.Layer")) FGameplayTag layer, UCommonActivatableWidgetContainerBase* widget);

   UPROPERTY(Transient)
   TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> _layers;
};

template<typename T>
T* UTATLayoutWidget::PushWidget(FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass) const
{
   TObjectPtr<UCommonActivatableWidgetContainerBase> layerWidget = _layers.FindRef(layer);
   return layerWidget ? layerWidget->AddWidget<T>(widgetClass) : nullptr;
}

template<typename T>
T* UTATLayoutWidget::PushWidget(FGameplayTag layer, TSubclassOf<UCommonActivatableWidget> widgetClass, TFunctionRef<void(T& Widget)> initFunc) const
{
   TObjectPtr<UCommonActivatableWidgetContainerBase> layerWidget = _layers.FindRef(layer);
   return layerWidget ? layerWidget->AddWidget<T>(widgetClass, initFunc) : nullptr;
}
