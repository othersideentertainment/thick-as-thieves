// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "TATWidgetComponent.generated.h"

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class TAT_API UTATWidgetComponent : public UWidgetComponent
{
   GENERATED_BODY()
   
public:
   // If enabled, the widget component's ShouldDrawWidget method will always return true.
   // This is useful for drawing the widget component's render target onto another surface.
   // If updateTickSettings is true, this will also set the widget component's tick mode and offscreen-tick settings appropriately.
   UFUNCTION(BlueprintCallable, Category = "TAT Widget Component")
   void SetAlwaysRedrawWidget(bool alwaysRedraw, bool updateTickSettings = true);

   // Checks if the widget will always be redraw regardless of visibility.
   UFUNCTION(BlueprintPure, Category = "TAT Widget Component")
   bool GetAlwaysRedrawWidget() const { return _alwaysRedrawWidget; }

   // Don't redraw the widget whenever this component is not visible on screen.
   // You can pass null in a subsequent call to disable.
   UFUNCTION(BlueprintCallable, Category = "TAT Widget Component")
   void SetOnlyRedrawWhenTargetComponentIsVisible(UPrimitiveComponent* newLinkedComponent);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATOnWidgetComponentRenderTargetChanged, UTextureRenderTarget2D*, renderTarget);

   // Event fired when the widget component's render target changes.
   // This is intended for keeping a dynamic material in sync with the widget.
   UPROPERTY(BlueprintAssignable, Category = "TAT Widget Component")
   FTATOnWidgetComponentRenderTargetChanged OnRenderTargetChanged;

   // From UWidgetComponent
   virtual void UpdateRenderTarget(FIntPoint DesiredRenderTargetSize) override;

   // If enabled, always checks RedrawTime even if this component is not being rendered
   // (eg. if this component is hidden but we're using its render target on another component)
   UPROPERTY(EditDefaultsOnly, Category = "TAT Widget Component")
   bool AlwaysCheckRedrawInterval = true;

protected:
   // From UWidgetComponent
   virtual bool ShouldDrawWidget() const override;

   // When non-null, only draw the widget when this component is visible.
   // Primarily intended for cases where this component is the one the widget's render target is being rendered to.
   TWeakObjectPtr<UPrimitiveComponent> _linkedComponent;

   // If enabled, ShouldDrawWidget always returns true
   bool _alwaysRedrawWidget = false;

   // Last game time in seconds the widget was redrawn
   double _lastDrawTime = 0;

private:
   // cached render target - used by OnRenderTargetChanged to detect changes to fire events
   TWeakObjectPtr<UTextureRenderTarget2D> _prevRenderTarget;

};
