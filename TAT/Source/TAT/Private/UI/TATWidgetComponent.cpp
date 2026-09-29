// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATWidgetComponent.h"
#include "Engine/TextureRenderTarget2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWidgetComponent)

void UTATWidgetComponent::SetAlwaysRedrawWidget(bool alwaysRedraw, bool updateTickSettings)
{
   _alwaysRedrawWidget = alwaysRedraw;
   if (updateTickSettings)
   {
      SetTickMode(alwaysRedraw ? ETickMode::Enabled : ETickMode::Automatic);
      SetTickWhenOffscreen(alwaysRedraw);
   }
}

void UTATWidgetComponent::SetOnlyRedrawWhenTargetComponentIsVisible(UPrimitiveComponent* newLinkedComponent)
{
   _linkedComponent = newLinkedComponent;
}

void UTATWidgetComponent::UpdateRenderTarget(FIntPoint DesiredRenderTargetSize)
{
   Super::UpdateRenderTarget(DesiredRenderTargetSize);

   // UpdateRenderTarget is called on every draw so we can use this to detect how long it's been since the last update
   _lastDrawTime = GetWorld()->TimeSeconds;

   // Detect changes to RenderTarget and broadcast an event when it changes
   UTextureRenderTarget2D* curRenderTarget = RenderTarget.Get();
   if (_prevRenderTarget.Get() != curRenderTarget)
   {
      OnRenderTargetChanged.Broadcast(curRenderTarget);
      _prevRenderTarget = curRenderTarget;
   }
}

bool UTATWidgetComponent::ShouldDrawWidget() const
{
   if (_alwaysRedrawWidget || bRedrawRequested)
   {
      return true;
   }

   // We have a linked component and it's not currently being rendered - don't redraw the widget
   UPrimitiveComponent* linkedComp = _linkedComponent.Get();
   if (linkedComp != nullptr && (!linkedComp->IsVisible() || GetWorld()->TimeSeconds - linkedComp->GetLastRenderTimeOnScreen() >= 0.2f))
   {
      return false;
   }

   // This looks redundant to what Super::ShouldDrawWidget does, but there's one important difference:
   // In UWidgetComponent, ShouldDrawWidget only checks its redraw interval if this widget component is visible, and we want to support
   // scenarios where the component is NOT visible while using the render target to draw the widget onto some other mesh.
   if (AlwaysCheckRedrawInterval)
   {
      return GetWorld()->TimeSeconds - _lastDrawTime >= FMath::Max(RedrawTime, GetWorld()->DeltaTimeSeconds + UE_KINDA_SMALL_NUMBER);
   }

   return Super::ShouldDrawWidget();
}
