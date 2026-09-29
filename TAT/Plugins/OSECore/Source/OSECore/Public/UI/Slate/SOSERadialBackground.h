// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "UI/OSERadialPaintLibrary.h"

// ue
#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Layout/Margin.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

class FPaintArgs;
class FSlateWindowElementList;

DECLARE_DELEGATE_ThreeParams(FOSEOnRadialBackgroundPaint, FOSERadialPaintContext&, const FVector2f&, const FFloatInterval&)

namespace OSERadialHelpers
{
   FORCEINLINE FFloatInterval VecToInterval(const FVector2D& vec)
   {
      return FFloatInterval{ static_cast<float>(vec.X), static_cast<float>(vec.Y) };
   }
}

/// A radial background is a widget that displays a circle (or part of a circle) and can draw slices of a circle on top of that.
/// Intended for use by radial wheels.
class OSECORE_API SOSERadialBackground : public SCompoundWidget
{
   SLATE_DECLARE_WIDGET(SOSERadialBackground, SCompoundWidget)

public:

   SLATE_BEGIN_ARGS(SOSERadialBackground)
      : _Content()
      , _HAlign(HAlign_Fill)
      , _VAlign(VAlign_Fill)
      , _ClampRadius(true)
      , _RadialBackground()
      , _UVMode(EOSERadialDiscUVMode::Curved)
      , _Resolution(0)
      , _PremultipliedAlpha(false)
      , _ContentScale(FVector2D(1, 1))
      , _RadiusForDesiredSize(0.0f)
      , _TintColor(FLinearColor::White)
      , _BorderColor(FLinearColor::Transparent)
      , _ColorAndOpacity(FLinearColor::White)
      , _ForegroundColor(FSlateColor::UseForeground())
      , _ShowEffectWhenDisabled(true)
      {}

      SLATE_DEFAULT_SLOT(FArguments, Content)

      SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
      SLATE_ARGUMENT(EVerticalAlignment, VAlign)
      SLATE_ARGUMENT(bool, ClampRadius)

      SLATE_EVENT(FOSEOnRadialBackgroundPaint, OnRadialPaint)

      SLATE_ATTRIBUTE(FVector2D, OriginOffset)

      SLATE_ATTRIBUTE(FVector2D, Radius)

      SLATE_ATTRIBUTE(FVector2D, ArcRangeDegrees)

      SLATE_ATTRIBUTE(const FSlateBrush*, RadialBackground)

      SLATE_ARGUMENT(EOSERadialDiscUVMode, UVMode)
      SLATE_ARGUMENT(int32, Resolution)
      SLATE_ARGUMENT(bool, PremultipliedAlpha)

      SLATE_ATTRIBUTE(FVector2D, ContentScale)

      SLATE_ATTRIBUTE(float, RadiusForDesiredSize)

      SLATE_ATTRIBUTE(FLinearColor, TintColor)
      SLATE_ATTRIBUTE(FLinearColor, BorderColor)

      /// ColorAndOpacity is the color and opacity of content in the radial
      SLATE_ATTRIBUTE(FLinearColor, ColorAndOpacity)

      /// The foreground color of text and some glyphs that appear as the radial's content.
      SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)

      /// Whether or not to show the disabled effect when this radial is disabled
      SLATE_ATTRIBUTE(bool, ShowEffectWhenDisabled)

   SLATE_END_ARGS()

   SOSERadialBackground();
   void Construct(const FArguments& args);

   virtual void SetContent(TSharedRef<SWidget> newContent);
   const TSharedRef<SWidget>& GetContent() const;
   void ClearContent();

   /// See HAlign argument
   void SetHAlign(EHorizontalAlignment newHAlign);

   /// See VAlign argument
   void SetVAlign(EVerticalAlignment newVAlign);

   FVector2D GetOriginOffset() const { return _originOffsetAttribute.Get(); }
   void SetOriginOffset(TAttribute<FVector2D> newOriginOffset);

   /// Get the image to draw for this border.
   const FSlateBrush* GetRadialBackground() const { return _radialBackgroundAttribute.Get(); }
   /// Set the image to draw for this border.
   void SetRadialBackground(TAttribute<const FSlateBrush*> newRadialBackground);

   FVector2D GetRadius() const { return _radiusAttribute.Get(); }
   FFloatInterval GetRadiusAsInterval() const { return OSERadialHelpers::VecToInterval(_radiusAttribute.Get()); }
   void SetRadius(TAttribute<FVector2D> newRadius);
   void SetRadius(const FFloatInterval& newRadius) { SetRadius(FVector2D(newRadius.Min, newRadius.Max)); }

   FVector2D GetArcRangeDegrees() const { return _arcRangeDegreesAttribute.Get(); }
   FFloatInterval GetArcRangeDegreesAsInterval() const { return OSERadialHelpers::VecToInterval(_arcRangeDegreesAttribute.Get()); }
   void SetArcRangeDegrees(TAttribute<FVector2D> newArcRangeDegrees);
   void SetArcRangeDegrees(const FFloatInterval& newArcRangeDegrees) { SetArcRangeDegrees(FVector2D(newArcRangeDegrees.Min, newArcRangeDegrees.Max)); }

   bool GetClampRadius() const { return _clampRadius; }
   void SetClampRadius(bool newClampRadius);

   EOSERadialDiscUVMode GetUVMode() const { return _uvMode; }
   void SetUVMode(EOSERadialDiscUVMode newUVMode);

   int32 GetResolution() const { return _resolution; }
   void SetResolution(int32 newResolution);

   bool GetPremultipliedAlpha() const { return _premultipliedAlpha; }
   void SetPremultipliedAlpha(bool newPremultipliedAlpha);

   FLinearColor GetTintColor() const { return _tintColorAttribute.Get(); }
   void SetTintColor(TAttribute<FLinearColor> newTintColor);

   FLinearColor GetBorderColor() const { return _borderColorAttribute.Get(); }
   void SetBorderColor(TAttribute<FLinearColor> newBorderColor);

   float GetBorderThickness() const { return _borderThickness; }
   void SetBorderThickness(float newBorderThickness);

   /// Get the desired size scale multiplier
   float GetRadiusForDesiredSize() const { return _radiusForDesiredSizeAttribute.Get(); }
   /// Set the desired size scale multiplier
   void SetRadiusForDesiredSize(TAttribute<float> newRadiusForDesiredSize);

   /// Get whether or not to show the disabled effect when this border is disabled
   bool GetShowDisabledEffect() const { return _showDisabledEffectAttribute.Get(); }
   /// Set whether or not to show the disabled effect when this radial is disabled
   void SetShowEffectWhenDisabled(TAttribute<bool> newShowEffectWhenDisabled);

public:
   // From SWidget
   virtual int32 OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& myCullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const override;
protected:
   virtual FVector2D ComputeDesiredSize(float layoutScaleMultiplier) const override;

   TSlateAttributeRef<FVector2D> GetOriginOffsetAttribute() const { return TSlateAttributeRef<FVector2D>(SharedThis(this), _originOffsetAttribute); }
   TSlateAttributeRef<const FSlateBrush*> GetRadialBackgroundAttribute() const { return TSlateAttributeRef<const FSlateBrush*>(SharedThis(this), _radialBackgroundAttribute); }
   TSlateAttributeRef<FVector2D> GetRadiusAttribute() const { return TSlateAttributeRef<FVector2D>(SharedThis(this), _radiusAttribute); }
   TSlateAttributeRef<FVector2D> GetArcRangeDegreesAttribute() const { return TSlateAttributeRef<FVector2D>(SharedThis(this), _arcRangeDegreesAttribute); }
   TSlateAttributeRef<FLinearColor> GetTintColorAttribute() const { return TSlateAttributeRef<FLinearColor>(SharedThis(this), _tintColorAttribute); }
   TSlateAttributeRef<FLinearColor> GetBorderColorAttribute() const { return TSlateAttributeRef<FLinearColor>(SharedThis(this), _borderColorAttribute); }
   TSlateAttributeRef<float> GetRadiusForDesiredSizeAttribute() const { return TSlateAttributeRef<float>(SharedThis(this), _radiusForDesiredSizeAttribute); }
   TSlateAttributeRef<bool> GetShowDisabledEffectAttribute() const { return TSlateAttributeRef<bool>(SharedThis(this), _showDisabledEffectAttribute); }

private:
   FOSEOnRadialBackgroundPaint _onRadialPaint;

   bool _clampRadius = true;
   EOSERadialDiscUVMode _uvMode = EOSERadialDiscUVMode::Curved;
   int32 _resolution = 0;
   float _borderThickness = 1.0f;
   bool _premultipliedAlpha = false;

   TSlateAttribute<FVector2D> _originOffsetAttribute;
   TSlateAttribute<const FSlateBrush*> _radialBackgroundAttribute;
   TSlateAttribute<FVector2D> _radiusAttribute;
   TSlateAttribute<FVector2D> _arcRangeDegreesAttribute;
   TSlateAttribute<FLinearColor> _tintColorAttribute;
   TSlateAttribute<FLinearColor> _borderColorAttribute;
   TSlateAttribute<float> _radiusForDesiredSizeAttribute;
   TSlateAttribute<bool> _showDisabledEffectAttribute;
};
