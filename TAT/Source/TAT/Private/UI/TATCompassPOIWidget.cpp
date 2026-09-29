// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/TATCompassPOIWidget.h"

// ue
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCompassPOIWidget)

namespace POIHelpers
{
   static void TrySetVisibility(UWidget* widget, ESlateVisibility visibility)
   {
      // Set visibility may dirty slate, so checking the value first
      check(widget);
      if(widget->GetVisibility() != visibility)
      {
         widget->SetVisibility(visibility);
      }
   }
}

void UTATCompassPOIWidget::SetPOIOffScreenIndicator(bool isLeftOfScreen)
{
   if(isLeftOfScreen)
   {
      POIHelpers::TrySetVisibility(Image_LeftIndicator, ESlateVisibility::SelfHitTestInvisible);
      POIHelpers::TrySetVisibility(Image_RightIndicator, ESlateVisibility::Collapsed);
   }
   else
   {
      POIHelpers::TrySetVisibility(Image_RightIndicator, ESlateVisibility::SelfHitTestInvisible);
      POIHelpers::TrySetVisibility(Image_LeftIndicator, ESlateVisibility::Collapsed);
   }
}

void UTATCompassPOIWidget::ClearPOIOffScreenIndicator()
{
   POIHelpers::TrySetVisibility(Image_LeftIndicator, ESlateVisibility::Collapsed);
   POIHelpers::TrySetVisibility(Image_RightIndicator, ESlateVisibility::Collapsed);
}

void UTATCompassPOIWidget::SetPOIVerticalityIndicator(bool isAbovePlayer)
{
   if(isAbovePlayer)
   {
      POIHelpers::TrySetVisibility(Image_AboveIndicator, ESlateVisibility::SelfHitTestInvisible);
      POIHelpers::TrySetVisibility(Image_BelowIndicator, ESlateVisibility::Collapsed);
   }
   else
   {
      POIHelpers::TrySetVisibility(Image_BelowIndicator, ESlateVisibility::SelfHitTestInvisible);
      POIHelpers::TrySetVisibility(Image_AboveIndicator, ESlateVisibility::Collapsed);
   }
}

void UTATCompassPOIWidget::ClearPOIVerticalityIndicator()
{
   POIHelpers::TrySetVisibility(Image_AboveIndicator, ESlateVisibility::Collapsed);
   POIHelpers::TrySetVisibility(Image_BelowIndicator, ESlateVisibility::Collapsed);
}

void UTATCompassPOIWidget::SetPOIVisible(bool visible)
{
   POIHelpers::TrySetVisibility(this, visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTATCompassPOIWidget::UpdateDistance(float distance)
{
   int distanceInMeters = FMath::RoundToInt32(distance / 100.0f);
   if(distanceInMeters != _cachedDistance)
   {
      _cachedDistance = distanceInMeters;
      Text_Distance->SetText(FText::FormatNamed(DistanceFormat, TEXT("Distance"), distanceInMeters));
   }
}

void UTATCompassPOIWidget::SetDistanceVisible(bool visible)
{
   POIHelpers::TrySetVisibility(Text_Distance, visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
