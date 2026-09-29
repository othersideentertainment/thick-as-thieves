// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Tutorial/TATHUDHighlightWidget.h"

// ue
#include "Blueprint/WidgetLayoutLibrary.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHUDHighlightWidget)

void UTATHUDHighlightWidget::NativeConstruct()
{
   Super::NativeConstruct();

   _PositionToTarget();
}

void UTATHUDHighlightWidget::NativeTick(const FGeometry& myGeometry, float inDeltaTime)
{
   _PositionToTarget();
   
   Super::NativeTick(myGeometry, inDeltaTime);
}

void UTATHUDHighlightWidget::SetTarget(UWidget* target)
{
   _targetWidget = target;
}

void UTATHUDHighlightWidget::_PositionToTarget()
{
   if(!IsValid(_targetWidget))
   {
      return;
   }

   // TODO: Hide initially before first positioning
   //
   // On tick, just update its viewport position to match that of the target,
   // accounting for transforms
   //
   // NOTE: GetCachedGeometry may return the geometry from the previous frame,
   //       however this is not likely to be a big deal as long as it isn't moving
   //       much.
   const FGeometry viewportGeometry = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this);
   const FGeometry& targetGeometry = _targetWidget->GetCachedGeometry();
   const FSlateRect boundingRect = targetGeometry.GetRenderBoundingRect();

   const float viewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

   SetPositionInViewport(viewportGeometry.AbsoluteToLocal(boundingRect.GetCenter2f()), false);
   SetDesiredSizeInViewport(boundingRect.GetSize() / viewportScale);
   // Force alignment to be centered, so that it aligns with the positioning
   SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}
