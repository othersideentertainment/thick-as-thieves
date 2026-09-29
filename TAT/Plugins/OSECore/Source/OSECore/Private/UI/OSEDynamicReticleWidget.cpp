// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSEDynamicReticleWidget.h"

// ue
#include "Blueprint/UserWidget.h"

// ose
#include "Math/OSEMathFunctionLibrary.h"
#include "UI/Slate/SOSERadialBackground.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEDynamicReticleWidget)

DEFINE_LOG_CATEGORY_STATIC(LogOSEDynamicReticleWidget, Log, All);

void FOSEDynamicReticleProgressBar::Paint(FOSERadialPaintContext& ctx, const FVector2f& reticleOrigin, float baseReticleRadius, TOptional<float> valueOverride,
   bool isDesignView, TOptional<FName> indicatorName, ESlateDrawEffect drawEffect) const
{
   float normalizedValue = 0.0f;
   if (valueOverride)
   {
      normalizedValue = FMath::GetMappedRangeValueClamped(FVector2f(ValueRange.Min, ValueRange.Max), FVector2f(0.0f, 1.0f), *valueOverride);
   }
   else if (isDesignView)
   {
      normalizedValue = FMath::Clamp(PreviewPercent, 0.0f, 1.0f);
   }
   else if (CurrentValue)
   {
      normalizedValue = FMath::GetMappedRangeValueClamped(FVector2f(ValueRange.Min, ValueRange.Max), FVector2f(0.0f, 1.0f), *CurrentValue);
   }
   else
   {
      // If we don't have a progress bar value, don't draw it at all
      return;
   }

   const float radiusOffset = RadiusIsRelativeToReticle ? baseReticleRadius : 0.0f;

   ctx.DrawRadialProgressBar(ProgressBar, normalizedValue, reticleOrigin, radiusOffset, drawEffect);

   if (isDesignView && indicatorName.IsSet())
   {
      const FFloatInterval barRadius = ProgressBar.GetRadius(radiusOffset);
      const float indicatorCenterRadius = FMath::Lerp(barRadius.Min, barRadius.Max, 0.5f);

      float endAngleDeg = ProgressBar.ArcAngleDegrees;
      if (ProgressBar.ProgressMode == EOSERadialProgressBarMode::Forward)
      {
         endAngleDeg += ProgressBar.ArcSizeDegrees;
      }
      else if (ProgressBar.ProgressMode == EOSERadialProgressBarMode::Backward)
      {
         endAngleDeg -= ProgressBar.ArcSizeDegrees;
      }

      const float indicatorCenterAngleDeg = UOSERadialPaintLibrary::LerpArcAngleDegrees(FFloatInterval{ ProgressBar.ArcAngleDegrees, endAngleDeg }, normalizedValue * 0.5f);
      const FVector2f indicatorCenterPoint = FVector2f(UOSERadialPaintLibrary::FindPointOnCircle(FVector2D(reticleOrigin), indicatorCenterRadius, indicatorCenterAngleDeg)) - FVector2f(0, 5.0f);
      ctx.DrawText(indicatorCenterPoint, indicatorName->ToString());
   }
}

UOSEDynamicReticleWidget::UOSEDynamicReticleWidget()
   : Super()
{
}

TSharedRef<SWidget> UOSEDynamicReticleWidget::RebuildWidget()
{
   _widget = SNew(SOSERadialBackground)
      .OnRadialPaint_UObject(this, &UOSEDynamicReticleWidget::_OnRadialPaint);

   if (GetChildrenCount() > 0)
   {
      UPanelSlot* slot = GetContentSlot();
      check(slot != nullptr && slot->Content);
      _widget->SetContent(slot->Content->TakeWidget());
   }

   return _widget.ToSharedRef();
}

void UOSEDynamicReticleWidget::SynchronizeProperties()
{
   Super::SynchronizeProperties();

   if (!_widget.IsValid())
   {
      return;
   }

   _widget->SetHAlign(HorizontalAlign);
   _widget->SetVAlign(VerticalAlign);
   _widget->SetRadiusForDesiredSize(_CalcReticleRadiusForWidgetDesiredSize());
   _widget->SetClampRadius(ClampReticleRadiusToWidgetSize);
   _widget->SetOriginOffset(ReticleOriginOffset);
   _widget->SetRadius(_CalcDiscRadiusWithDiscOutlineThickness(DiscOutlineThickness, DiscRadius));
   _widget->SetColorAndOpacity(ReticleTintColor);

   if (ShowDisc)
   {
      _widget->SetArcRangeDegrees(GetDiscArcRangeDegrees());
      _widget->SetUVMode(DiscFillImageUVMode);
      _widget->SetResolution(DiscResolution);
      _widget->SetPremultipliedAlpha(DiscFillImagePremultipliedAlpha);
      _widget->SetRadialBackground(&DiscFillImage.GetSlateBrush());
      _widget->SetTintColor(DiscTintColor);
      _widget->SetBorderColor(GetDiscBorderColor());
      _widget->SetBorderThickness(DiscBorderThickness);
   }
   else
   {
      static const FSlateBrush emptyBrush = FSlateNoResource();
      _widget->SetRadialBackground(&emptyBrush);
      _widget->SetArcRangeDegrees(FFloatInterval{ 0, 0 });
      _widget->SetBorderColor(FLinearColor::Transparent);
   }
}

void UOSEDynamicReticleWidget::RemoveFromParent()
{
   _DisableReticleTick();

   Super::RemoveFromParent();
}

#if WITH_EDITOR
const FText UOSEDynamicReticleWidget::GetPaletteCategory()
{
   return FText::FromString(TEXT("OSE"));
}
#endif

void UOSEDynamicReticleWidget::ReleaseSlateResources(bool releaseChildren)
{
   Super::ReleaseSlateResources(releaseChildren);
   _widget.Reset();
}

UClass* UOSEDynamicReticleWidget::GetSlotClass() const
{
   return UPanelSlot::StaticClass();
}

void UOSEDynamicReticleWidget::OnSlotAdded(UPanelSlot* slot)
{
   if (_widget.IsValid())
   {
      if (slot != nullptr && slot->Content)
      {
         _widget->SetContent(slot->Content->TakeWidget());
      }
      else
      {
         _widget->SetContent(SNullWidget::NullWidget);
      }
   }
}

void UOSEDynamicReticleWidget::OnSlotRemoved(UPanelSlot* slot)
{
   if (_widget.IsValid())
   {
      _widget->SetContent(SNullWidget::NullWidget);
   }
}

void UOSEDynamicReticleWidget::_OnRadialPaint(FOSERadialPaintContext& paintCtx, const FVector2f& radialOrigin, const FFloatInterval& radius)
{
   if (!_widget.IsValid())
   {
      return;
   }

   const FFloatInterval radialArcRangeDeg = _widget->GetArcRangeDegreesAsInterval();
   const int32 radialResolution = _widget->GetResolution();
   const float baseRadius = FMath::Max(ReticleRadius, radius.Max);

   for (const FOSERadialDiscShape& extraDisc : DiscShapes)
   {
      paintCtx.DrawDisc(extraDisc, radialOrigin);
   }

   for (const FOSERadialArcShape& extraArc : ArcShapes)
   {
      paintCtx.DrawArc(extraArc, radialOrigin);
   }

   for (const FOSERadialLineShape& extraLine : LineShapes)
   {
      paintCtx.DrawLineFromOriginAndAngle(extraLine, radialOrigin);
   }

   if (ShowArc)
   {
      paintCtx.DrawArc(
         radialOrigin + FVector2f(ArcOriginOffset),
         ArcRadius + (ArcRadiusIsRelativeToReticleRadius ? ReticleRadius : 0.0f),
         FFloatInterval{ ArcRangeStartDegrees, ArcRangeEndDegrees },
         FOSELineParams{ ArcThickness, ArcColor },
         ArcResolution);
   }

   if (ShowCrosshair)
   {
      FFloatInterval crosshairRadius{ CrosshairCenterSpacing, CrosshairCenterSpacing + CrosshairLength };
      if (CrosshairCenterSpacingIsRelativeToReticleRadius)
      {
         crosshairRadius += ReticleRadius;
      }
      paintCtx.DrawCrosshair(radialOrigin + FVector2f(CrosshairOriginOffset), CrosshairSpokes, FOSELineParams{ CrosshairThickness, CrosshairColor }, crosshairRadius, CrosshairRotationDegrees);
   }

   for (const FOSECrosshairShape& crosshair : CrosshairShapes)
   {
      paintCtx.DrawCrosshair(crosshair, radialOrigin, ReticleRadius);
   }

   for (const auto& pair : RadialProgressIndicators)
   {
      const TOptional<FName> showIndicatorName = ShowProgressIndicatorNamesInDesignView ? TOptional<FName>(pair.Key) : NullOpt;
      pair.Value.Paint(paintCtx, radialOrigin, ReticleRadius, NullOpt, IsDesignTime(), showIndicatorName);
   }

   // Switching to a blueprint-friendly paint context
   {
      FOSEScopedUMGPaintContext umgPaintContext{ paintCtx };

      // Hook for drawing from native subclasses and blueprints
      _OnPaintDynamicReticle(*umgPaintContext, FVector2D(radialOrigin));

      // Update tracked actors if we have any
      for (FTrackedActor& trackedActor : _trackedActors)
      {
         if (trackedActor.NeedsWidgetPositionUpdate)
         {
            AActor* actor = trackedActor.Actor.Get();
            if (actor == nullptr)
            {
               trackedActor.NeedsWidgetPositionUpdate = false;
               continue;
            }

            // Call GetActorLocation, allowing for a blueprint override to find the location to show on-screen for an actor
            const FVector trackedActorLocation = OverrideGetTrackedActorLocation.IsBound()
               ? OverrideGetTrackedActorLocation.Execute(actor)
               : actor->GetActorLocation();

            trackedActor.HaveValidWidgetPosition = UOSERadialPaintLibrary::TransformWorldPositionToPaintSpace(
               GetOwningPlayer(), *umgPaintContext, trackedActorLocation, trackedActor.LastWidgetPosition);
            trackedActor.NeedsWidgetPositionUpdate = false;

            // Broadcast the updated position
            _OnPaintTrackedActor(*umgPaintContext, FVector2D(radialOrigin), actor, trackedActor.HaveValidWidgetPosition, trackedActor.LastWidgetPosition);
         }
      }
   }
}

bool UOSEDynamicReticleWidget::_Tick(float deltaSeconds)
{
   int32 numValidActors = 0;
   for (FTrackedActor& trackedActor : _trackedActors)
   {
      if (AActor* actor = trackedActor.Actor.Get())
      {
         ++numValidActors;
         trackedActor.NeedsWidgetPositionUpdate = true;
      }
   }

   if (numValidActors > 0 && _widget)
   {
      // Trigger a repaint - we can't accurately convert the world position to a widget-space position outside of a paint callback
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }

   if (numValidActors == 0)
   {
      _trackedActors.Reset();
      _tickTimerHandle.Reset();
      return false;
   }

   // return true to continue ticking
   return true;
}

void UOSEDynamicReticleWidget::SetReticleRadius(float newReticleRadius)
{
   const float origReticleRadius = ReticleRadius;
   ReticleRadius = FMath::Max(newReticleRadius, 0.0f);
   if (FMath::IsNearlyEqual(ReticleRadius, origReticleRadius))
   {
      return;
   }
   if (_widget)
   {
      if (ShowDisc)
      {
         _widget->SetRadius(_CalcDiscRadiusWithDiscOutlineThickness(DiscOutlineThickness, DiscRadius));
      }
      _widget->SetRadiusForDesiredSize(_CalcReticleRadiusForWidgetDesiredSize());
      _widget->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
   }
}
void UOSEDynamicReticleWidget::SetReticleTintColor(FLinearColor newReticleTintColor)
{
   ReticleTintColor = newReticleTintColor;
   if (_widget)
   {
      _widget->SetColorAndOpacity(newReticleTintColor);
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}

void UOSEDynamicReticleWidget::SetArcRadius(float newArcRadius)
{
   ArcRadius = newArcRadius;
   if (_widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
   }
}
void UOSEDynamicReticleWidget::SetArcColor(FLinearColor newArcColor)
{
   ArcColor = newArcColor;
   if (_widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetArcThickness(float newArcThickness)
{
   ArcThickness = newArcThickness;
   if (_widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetArcRangeStartDegrees(float newArcRangeStartDegrees)
{
   ArcRangeStartDegrees = newArcRangeStartDegrees;
   if (_widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetArcRangeEndDegrees(float newArcRangeEndDegrees)
{
   ArcRangeEndDegrees = newArcRangeEndDegrees;
   if (_widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetArcOriginOffset(FVector2D newArcOriginOffset)
{
   ArcOriginOffset = newArcOriginOffset;
   if (_widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}

void UOSEDynamicReticleWidget::SetCrosshairCenterSpacing(float newCrosshairCenterSpacing)
{
   CrosshairCenterSpacing = newCrosshairCenterSpacing;
   if (ShowCrosshair && _widget)
   {
      _widget->SetRadiusForDesiredSize(_CalcReticleRadiusForWidgetDesiredSize());
      _widget->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
   }
}
void UOSEDynamicReticleWidget::SetCrosshairLength(float newCrosshairLength)
{
   CrosshairLength = newCrosshairLength;
   if (ShowCrosshair && _widget)
   {
      _widget->SetRadiusForDesiredSize(_CalcReticleRadiusForWidgetDesiredSize());
      _widget->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
   }
}
void UOSEDynamicReticleWidget::SetCrosshairThickness(float newCrosshairThickness)
{
   CrosshairThickness = newCrosshairThickness;
   if (ShowCrosshair && _widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetCrosshairColor(FLinearColor newCrosshairColor)
{
   CrosshairColor = newCrosshairColor;
   if (ShowCrosshair && _widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetCrosshairRotationDegrees(float newCrosshairRotationDegrees)
{
   CrosshairRotationDegrees = newCrosshairRotationDegrees;
   if (ShowCrosshair && _widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetCrosshairOriginOffset(FVector2D newCrosshairOriginOffset)
{
   CrosshairOriginOffset = newCrosshairOriginOffset;
   if (ShowCrosshair && _widget)
   {
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}

void UOSEDynamicReticleWidget::SetDiscRadius(float newDiscRadius)
{
   DiscRadius = newDiscRadius;
   if (_widget)
   {
      _widget->SetRadius(_CalcDiscRadiusWithDiscOutlineThickness(DiscOutlineThickness, newDiscRadius));
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetDiscOutlineThickness(float newDiscOutlineThickness)
{
   DiscOutlineThickness = newDiscOutlineThickness;
   if (_widget)
   {
      _widget->SetRadius(_CalcDiscRadiusWithDiscOutlineThickness(DiscOutlineThickness, DiscRadius, &DiscOutlineThickness));
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetDiscTintColor(FLinearColor newDiscTintColor)
{
   DiscTintColor = newDiscTintColor;
   if (_widget)
   {
      _widget->SetTintColor(DiscTintColor);
      if (DiscBorderMode == EOSEDynamicReticleDiscBorderMode::UseDiscTintColor)
      {
         _widget->SetBorderColor(DiscTintColor);
      }
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetDiscBorderColor(FLinearColor newDiscBorderColor)
{
   DiscBorderColor = newDiscBorderColor;

   // If we're setting the disc border color directly, automatically change the border mode to use it
   DiscBorderMode = EOSEDynamicReticleDiscBorderMode::UseDiscBorderColor;

   if (_widget)
   {
      _widget->SetBorderColor(DiscBorderColor);
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}

void UOSEDynamicReticleWidget::SetDiscBorderThickness(float newDiscBorderThickness)
{
   DiscBorderThickness = newDiscBorderThickness;

   if (_widget)
   {
      _widget->SetBorderThickness(DiscBorderThickness);
   }
}

void UOSEDynamicReticleWidget::SetDiscArcRangeStartDegrees(float newDiscArcRangeStartDegrees)
{
   DiscArcRangeStartDegrees = newDiscArcRangeStartDegrees;
   if (_widget)
   {
      _widget->SetArcRangeDegrees(GetDiscArcRangeDegrees());
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}
void UOSEDynamicReticleWidget::SetDiscArcRangeEndDegrees(float newDiscArcRangeEndDegrees)
{
   DiscArcRangeEndDegrees = newDiscArcRangeEndDegrees;
   if (_widget)
   {
      _widget->SetArcRangeDegrees(GetDiscArcRangeDegrees());
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}

void UOSEDynamicReticleWidget::SetReticleOriginOffset(FVector2D newReticleOriginOffset)
{
   ReticleOriginOffset = newReticleOriginOffset;
   if (_widget)
   {
      _widget->SetOriginOffset(newReticleOriginOffset);
      _widget->Invalidate(EInvalidateWidgetReason::Paint);
   }
}


float UOSEDynamicReticleWidget::GetReticleRadius() const
{
   if (ClampReticleRadiusToWidgetSize)
   {
      const float desiredWidth = GetDesiredSize().X;
      if (desiredWidth > 0)
      {
         return FMath::Min(ReticleRadius, desiredWidth / 2.0f);
      }
   }
   return ReticleRadius;
}

FLinearColor UOSEDynamicReticleWidget::GetDiscBorderColor() const
{
   if (DiscBorderMode == EOSEDynamicReticleDiscBorderMode::UseDiscBorderColor)
   {
      return DiscBorderColor;
   }

   if (DiscBorderMode == EOSEDynamicReticleDiscBorderMode::UseDiscTintColor)
   {
      return DiscTintColor;
   }

   if (DiscBorderMode == EOSEDynamicReticleDiscBorderMode::UseDiscImageTintColor && DiscFillImage.UseTintColor)
   {
      return DiscFillImage.TintColor;
   }

   return FLinearColor::Transparent;
}

FFloatInterval UOSEDynamicReticleWidget::GetDiscArcRangeDegrees() const
{
   return OSERadialHelpers::NormalizeArcRangeDegrees(DiscArcRangeStartDegrees, DiscArcRangeEndDegrees);
}

bool UOSEDynamicReticleWidget::GetProgressIndicatorValue(FName indicatorName, float& value) const
{
   const FOSEDynamicReticleProgressBar* indicator = RadialProgressIndicators.Find(indicatorName);
   if (indicator != nullptr && indicator->CurrentValue.IsSet())
   {
      value = *indicator->CurrentValue;
      return true;
   }
   value = 0.0f;
   return false;
}

bool UOSEDynamicReticleWidget::SetProgressIndicatorValue(FName indicatorName, float newValue)
{
   if (FOSEDynamicReticleProgressBar* indicator = RadialProgressIndicators.Find(indicatorName))
   {
      indicator->CurrentValue = newValue;
      return true;
   }
   return false;
}

bool UOSEDynamicReticleWidget::ClearProgressIndicatorValue(FName indicatorName)
{
   if (FOSEDynamicReticleProgressBar* indicator = RadialProgressIndicators.Find(indicatorName))
   {
      indicator->CurrentValue.Reset();
      return true;
   }
   return false;
}

void UOSEDynamicReticleWidget::_OnPaintDynamicReticle(FPaintContext& context, const FVector2D& radialOrigin)
{
   OnPaintDynamicReticle.ExecuteIfBound(context, radialOrigin);
}

void UOSEDynamicReticleWidget::_OnPaintTrackedActor(FPaintContext& context, const FVector2D& radialOrigin, AActor* actor, bool positionIsValid, const FVector2D& newActorPosition)
{
   if (OnPaintTrackedActor.IsBound())
   {
      OnPaintTrackedActor.Execute(context, radialOrigin, actor, positionIsValid, newActorPosition);
   }
}

void UOSEDynamicReticleWidget::_EnableReticleTick()
{
   if (_tickTimerHandle.IsValid())
   {
      return;
   }
   _tickTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UOSEDynamicReticleWidget::_Tick));
}

void UOSEDynamicReticleWidget::_DisableReticleTick()
{
   if (_tickTimerHandle.IsValid())
   {
      FTSTicker::GetCoreTicker().RemoveTicker(_tickTimerHandle);
      _tickTimerHandle.Reset();
   }
}

FFloatInterval UOSEDynamicReticleWidget::_CalcDiscRadiusWithDiscOutlineThickness(float outlineThickness, float outerRadius, float* outAdjustedOutlineThickness) const
{
   FFloatInterval result{ 0.0f, outerRadius + (DiscRadiusIsRelativeToReticleRadius ? ReticleRadius : 0.0f) };
   if (IsOutline)
   {
      result.Min = FMath::Max(0.0f, result.Max - outlineThickness);
   }
   return result;
}

float UOSEDynamicReticleWidget::_CalcReticleRadiusForWidgetDesiredSize() const
{
   float radiusForDesiredSize = ReticleRadius;

   if (ShowArc)
   {
      radiusForDesiredSize = FMath::Max(radiusForDesiredSize, ArcRadius + (ArcRadiusIsRelativeToReticleRadius ? ReticleRadius : 0.0f));
   }

   if (ShowCrosshair)
   {
      const float maxCrosshairRadius = CrosshairCenterSpacing + CrosshairLength + (CrosshairCenterSpacingIsRelativeToReticleRadius ? ReticleRadius : 0.0f);
      radiusForDesiredSize = FMath::Max(radiusForDesiredSize, maxCrosshairRadius);
   }

   if (ShowDisc)
   {
      radiusForDesiredSize = FMath::Max(radiusForDesiredSize, DiscRadius + (DiscRadiusIsRelativeToReticleRadius ? ReticleRadius : 0.0f));
   }

   for (const auto& pair : RadialProgressIndicators)
   {
      const FOSEDynamicReticleProgressBar& indicator = pair.Value;
      if (indicator.AffectsWidgetDesiredSize)
      {
         radiusForDesiredSize = FMath::Max(radiusForDesiredSize, indicator.ProgressBar.GetRadius(ReticleRadius).Max);
      }
   }

   if (DecorativeCrosshairShapesAffectDesiredSize)
   {
      for (const FOSECrosshairShape& crosshair : CrosshairShapes)
      {
         radiusForDesiredSize = FMath::Max(radiusForDesiredSize, crosshair.GetCrosshairRadius(ReticleRadius).Max);
      }
   }

   if (DecorativeDiscShapesAffectDesiredSize)
   {
      for (const FOSERadialDiscShape& disc : DiscShapes)
      {
         radiusForDesiredSize = FMath::Max(radiusForDesiredSize, disc.Radius.Max);
      }
   }

   if (DecorativeArcShapesAffectDesiredSize)
   {
      for (const FOSERadialArcShape& arc : ArcShapes)
      {
         radiusForDesiredSize = FMath::Max(radiusForDesiredSize, arc.Radius);
      }
   }

   if (DecorativeLineShapesAffectDesiredSize)
   {
      for (const FOSERadialLineShape& line : LineShapes)
      {
         radiusForDesiredSize = FMath::Max(radiusForDesiredSize, line.Radius.Max);
      }
   }

   return radiusForDesiredSize;
}

void UOSEDynamicReticleWidget::AddTrackedActor(AActor* actor)
{
   if (actor == nullptr)
   {
      return;
   }

   for (const FTrackedActor& trackedActor : _trackedActors)
   {
      if (trackedActor.Actor.Get() == actor)
      {
         return;
      }
   }

   FTrackedActor newTrackedActor;
   newTrackedActor.Actor = actor;
   _trackedActors.Add(newTrackedActor);

   _EnableReticleTick();
}

void UOSEDynamicReticleWidget::RemoveTrackedActor(AActor* actorToRemove)
{
   int32 index = INDEX_NONE;
   for (int32 i = 0; i < _trackedActors.Num(); i++)
   {
      if (_trackedActors[i].Actor.Get() == actorToRemove)
      {
         index = i;
         break;
      }
   }

   if (index != INDEX_NONE)
   {
      _trackedActors.RemoveAtSwap(index);
   }

   if (_trackedActors.Num() == 0)
   {
      _DisableReticleTick();
   }
}

void UOSEDynamicReticleWidget::ClearTrackedActors()
{
   if (_trackedActors.Num() > 0)
   {
      _trackedActors.Reset();
      _DisableReticleTick();
   }
}

FTimerHandle UOSEDynamicReticleWidget::StartProgressIndicatorTimerWithCallback(FName progressIndicatorName, float duration, const FProgressIndicatorCallback& callback, bool resetFirst, TOptional<FVector2D> progressValueRange)
{
   if (duration <= 0 || !RadialProgressIndicators.Contains(progressIndicatorName))
   {
      return FTimerHandle{};
   }

   if (resetFirst)
   {
      FOSEDynamicReticleProgressBar& progressBar = RadialProgressIndicators[progressIndicatorName];
      progressBar.CurrentValue = progressBar.ValueRange.Min;
   }

   struct FTimerState
   {
      FTimerHandle Handle;
      TWeakObjectPtr<UWorld> World;
      TWeakObjectPtr<UOSEDynamicReticleWidget> Widget;
      FProgressIndicatorCallback Callback;

      void CancelTimer() { if (UWorld* world = World.Get()) { world->GetTimerManager().ClearTimer(Handle); } }
   };

   TSharedPtr<FTimerState> timerState = MakeShared<FTimerState>();
   timerState->World = GetWorld();
   timerState->Widget = this;
   timerState->Callback = callback;

   const double timerStartTime = GetWorld()->GetTimeSeconds();

   constexpr float interval = 1.0f / 30.0f;
   constexpr bool looping = true;
   GetWorld()->GetTimerManager().SetTimer(timerState->Handle, [timerState, progressIndicatorName, timerStartTime, duration, progressValueRange]()
   {
      check(timerState.IsValid());

      UOSEDynamicReticleWidget* self = timerState->Widget.Get();
      if (self == nullptr)
      {
         timerState->CancelTimer();
         return;
      }

      const double elapsedSeconds = FMath::Clamp(self->GetWorld()->GetTimeSeconds() - timerStartTime, 0.0, duration);
      const bool isTimerComplete = elapsedSeconds >= duration;

      // Update the progress bar value
      if (FOSEDynamicReticleProgressBar* progressBar = self->RadialProgressIndicators.Find(progressIndicatorName))
      {
         const FVector2D outputValueRange = progressValueRange.Get(FVector2D(progressBar->ValueRange.Min, progressBar->ValueRange.Max));
         progressBar->CurrentValue = FMath::GetMappedRangeValueClamped(FVector2D(0.0, duration), outputValueRange, elapsedSeconds);
         if (self->_widget)
         {
            self->_widget->Invalidate(EInvalidateWidgetReason::Paint);
         }
      }

      // Fire the callback if we have one
      if (timerState->Callback)
      {
         timerState->Callback(self, elapsedSeconds, isTimerComplete);
      }

      // Cancel the timer if we've hit the elapsed time
      if (isTimerComplete)
      {
         timerState->CancelTimer();
      }
   }, interval, looping);

   return timerState->Handle;
}

FTimerHandle UOSEDynamicReticleWidget::StartProgressIndicatorTimer(FName progressIndicatorName, float duration, bool resetFirst, bool useProgressValueRange, FVector2D progressValueRange)
{
   return StartProgressIndicatorTimerWithCallback(progressIndicatorName, duration, nullptr, resetFirst, useProgressValueRange ? TOptional<FVector2D>(progressValueRange) : NullOpt);
}

FTimerHandle UOSEDynamicReticleWidget::StartProgressIndicatorTimerWithDelegate(FName progressIndicatorName, float duration, FProgressIndicatorTimerEvent onProgressUpdated,
   bool resetFirst, bool useProgressValueRange, FVector2D progressValueRange)
{
   return StartProgressIndicatorTimerWithCallback(progressIndicatorName, duration,
      [onProgressUpdated](UOSEDynamicReticleWidget* self, double elapsedSeconds, bool isComplete)
      {
         if (onProgressUpdated.IsBound())
         {
            onProgressUpdated.Execute(elapsedSeconds, isComplete);
         }
      }, resetFirst, useProgressValueRange ? TOptional<FVector2D>(progressValueRange) : NullOpt);
}
