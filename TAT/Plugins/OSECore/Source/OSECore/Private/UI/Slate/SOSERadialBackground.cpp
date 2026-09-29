// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Slate/SOSERadialBackground.h"
#include "Rendering/DrawElements.h"

static const FOSERadialDiscShape kDefaultRadialDiscShape{};

SLATE_IMPLEMENT_WIDGET(SOSERadialBackground)
void SOSERadialBackground::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "OriginOffset", _originOffsetAttribute, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "RadialBackground", _radialBackgroundAttribute, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Radius", _radiusAttribute, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "ArcRangeDegrees", _arcRangeDegreesAttribute, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "TintColor", _tintColorAttribute, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "BorderColor", _borderColorAttribute, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "RadiusForDesiredSize", _radiusForDesiredSizeAttribute, EInvalidateWidgetReason::Layout);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "ShowDisabledEffect", _showDisabledEffectAttribute, EInvalidateWidgetReason::Paint);
}

SOSERadialBackground::SOSERadialBackground()
   : _originOffsetAttribute(*this, FVector2D(0, 0))
   , _radialBackgroundAttribute(*this, FCoreStyle::Get().GetBrush("WhiteBrush"))
   , _radiusAttribute(*this, FVector2D(100.0f, 400.0f))
   , _arcRangeDegreesAttribute(*this, FVector2D(0.0f, 360.0f))
   , _tintColorAttribute(*this, FLinearColor::White)
   , _borderColorAttribute(*this, FLinearColor::Transparent)
   , _radiusForDesiredSizeAttribute(*this, 0.0f)
   , _showDisabledEffectAttribute(*this, true)
{
}

void SOSERadialBackground::Construct(const SOSERadialBackground::FArguments& args)
{
   _onRadialPaint = args._OnRadialPaint;

   SetClampRadius(args._ClampRadius);
   SetOriginOffset(args._OriginOffset);
   SetRadialBackground(args._RadialBackground);
   SetRadius(args._Radius);
   SetArcRangeDegrees(args._ArcRangeDegrees);
   SetContentScale(args._ContentScale);
   SetColorAndOpacity(args._ColorAndOpacity);
   SetBorderColor(args._BorderColor);
   SetRadiusForDesiredSize(args._RadiusForDesiredSize);
   SetShowEffectWhenDisabled(args._ShowEffectWhenDisabled);
   SetForegroundColor(args._ForegroundColor);

   ChildSlot
   .HAlign(args._HAlign)
   .VAlign(args._VAlign)
   [
      args._Content.Widget
   ];
}

void SOSERadialBackground::SetContent(TSharedRef<SWidget> newContent)
{
   ChildSlot
   [
      newContent
   ];
}

const TSharedRef< SWidget >& SOSERadialBackground::GetContent() const
{
   return ChildSlot.GetWidget();
}

void SOSERadialBackground::ClearContent()
{
   ChildSlot.DetachWidget();
}

int32 SOSERadialBackground::OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const
{
   const FSlateBrush* brushResource = _radialBackgroundAttribute.Get();

   const bool enabled = ShouldBeEnabled(parentEnabled);

   const float renderOpacity = FMath::Clamp(widgetStyle.GetColorAndOpacityTint().A * GetRenderOpacity(), 0.0f, 1.0f);

   // the paint context keeps a _reference_ to the layer id, so it will stay up to date
   FOSERadialPaintContext paintCtx{ layerId, allottedGeometry, myCullingRect, outDrawElements, widgetStyle, parentEnabled, renderOpacity };

   // Apply the ColorAndOpacity attribute to all draw calls performed in this widget
   paintCtx.PushGlobalTintColor(GetColorAndOpacity());

   const FVector2f widgetSize = allottedGeometry.GetLocalSize();
   const FVector2f radialOrigin = (widgetSize / 2.0f) + FVector2f(_originOffsetAttribute.Get());
   const FFloatInterval arcRangeDegrees = OSERadialHelpers::VecToInterval(_arcRangeDegreesAttribute.Get());

   FFloatInterval radius = OSERadialHelpers::VecToInterval(_radiusAttribute.Get());
   if (_clampRadius)
   {
      const float minDimension = FMath::Min(widgetSize.X, widgetSize.Y) / 2.0f;
      if (radius.Max > minDimension)
      {
         radius.Max = minDimension;
      }
      if (radius.Min > minDimension)
      {
         radius.Min = minDimension;
      }
      // Make sure we didn't adjust the max radius to be smaller than the min radius
      if (radius.Min >= radius.Max && radius.Min > 0)
      {
         radius.Min = FMath::Max(0.0f, radius.Max - 1.0f);
      }
   }

   const FLinearColor borderColor = _borderColorAttribute.Get();
   const bool radialVisible = radius.Max > 0 && FMath::Max(0.0f, arcRangeDegrees.Max - arcRangeDegrees.Min) > 0;
   const bool borderVisible = radialVisible && borderColor.A > 0;

   ESlateDrawEffect drawEffect = ESlateDrawEffect::NoPixelSnapping;
   if (GetShowDisabledEffect() && !enabled)
   {
      drawEffect |= ESlateDrawEffect::DisabledEffect;
   }

   if (radialVisible && brushResource != nullptr && brushResource->DrawAs != ESlateBrushDrawType::NoDrawType)
   {
      ESlateDrawEffect discDrawEffect = drawEffect;
      if (_premultipliedAlpha)
      {
         discDrawEffect |= ESlateDrawEffect::PreMultipliedAlpha;
      }
      const FColor tint = (brushResource->GetTint(widgetStyle) * GetTintColor()).ToFColorSRGB();
      paintCtx.DrawDisc(radialOrigin, radius, arcRangeDegrees, *brushResource, _resolution, tint, _uvMode, discDrawEffect);
   }

   // Draw a thin border around the disc.
   // This is useful because disc's are drawn using triangles, which Slate is apparently not capable of antialiasing, but it CAN antialias lines, so this
   // gives us an easy mechanism to cover up ugly aliased edges when needed.
   if (radialVisible && borderColor.A > 0)
   {
      constexpr bool antialias = true;
      paintCtx.DrawArc(radialOrigin, radius.Max, arcRangeDegrees, FOSELineParams{ _borderThickness, borderColor }, _resolution, antialias, drawEffect);
      if (radius.Min > 0)
      {
         paintCtx.DrawArc(radialOrigin, radius.Min, arcRangeDegrees, FOSELineParams{ _borderThickness, borderColor }, _resolution, antialias, drawEffect);
      }
   }

   if (_onRadialPaint.IsBound())
   {
      _onRadialPaint.Execute(paintCtx, radialOrigin, radius);
   }

   paintCtx.PopGlobalTintColor();

   return Super::OnPaint(args, allottedGeometry, myCullingRect, outDrawElements, layerId, widgetStyle, enabled);
}

FVector2D SOSERadialBackground::ComputeDesiredSize(float layoutScaleMultiplier) const
{
   // If we have an overridden radius for the desired size calculation, use that instead of the background radius
   float radius = _radiusForDesiredSizeAttribute.Get();
   if (radius <= 0)
   {
      radius = _radiusAttribute.Get().Y;
   }
   return FVector2D(radius * 2.0f);
}

void SOSERadialBackground::SetTintColor(TAttribute<FLinearColor> newTintColor)
{
   _tintColorAttribute.Assign(*this, newTintColor);
}

void SOSERadialBackground::SetBorderColor(TAttribute<FLinearColor> newBorderColor)
{
   _borderColorAttribute.Assign(*this, newBorderColor);
}

void SOSERadialBackground::SetBorderThickness(float newBorderThickness)
{
   _borderThickness = newBorderThickness;
}

void SOSERadialBackground::SetRadiusForDesiredSize(TAttribute<float> newRadiusForDesiredSize)
{
   _radiusForDesiredSizeAttribute.Assign(*this, newRadiusForDesiredSize);
}

void SOSERadialBackground::SetHAlign(EHorizontalAlignment newHAlign)
{
   ChildSlot.SetHorizontalAlignment(newHAlign);
}

void SOSERadialBackground::SetVAlign(EVerticalAlignment newVAlign)
{
   ChildSlot.SetVerticalAlignment(newVAlign);
}

void SOSERadialBackground::SetShowEffectWhenDisabled(TAttribute<bool> newShowEffectWhenDisabled)
{
   _showDisabledEffectAttribute.Assign(*this, newShowEffectWhenDisabled);
}

void SOSERadialBackground::SetOriginOffset(TAttribute<FVector2D> newOriginOffset)
{
   _originOffsetAttribute.Assign(*this, newOriginOffset);
}

void SOSERadialBackground::SetRadialBackground(TAttribute<const FSlateBrush*> newRadialBackground)
{
   _radialBackgroundAttribute.Assign(*this, newRadialBackground);
}

void SOSERadialBackground::SetRadius(TAttribute<FVector2D> newRadius)
{
   _radiusAttribute.Assign(*this, newRadius);
}

void SOSERadialBackground::SetArcRangeDegrees(TAttribute<FVector2D> newArcRangeDegrees)
{
   _arcRangeDegreesAttribute.Assign(*this, newArcRangeDegrees);
}

void SOSERadialBackground::SetClampRadius(bool newClampRadius)
{
   _clampRadius = newClampRadius;
   Invalidate(EInvalidateWidgetReason::Paint);
}

void SOSERadialBackground::SetUVMode(EOSERadialDiscUVMode newUVMode)
{
   _uvMode = newUVMode;
   Invalidate(EInvalidateWidgetReason::Paint);
}

void SOSERadialBackground::SetResolution(int32 newResolution)
{
   _resolution = newResolution;
   Invalidate(EInvalidateWidgetReason::Paint);
}

void SOSERadialBackground::SetPremultipliedAlpha(bool newPremultipliedAlpha)
{
   _premultipliedAlpha = newPremultipliedAlpha;
   Invalidate(EInvalidateWidgetReason::Paint);
}
