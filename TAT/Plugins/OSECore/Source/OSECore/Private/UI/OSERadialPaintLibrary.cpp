// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSERadialPaintLibrary.h"

// ue
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Fonts/FontMeasure.h"
#include "Kismet/GameplayStatics.h"
#include "PaperSprite.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSERadialPaintLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogOSERadialPaintLibrary, Log, All);

const FSlateBrush* FOSEFillPaint::GetSlateBrushAndTint(FColor& outTint) const
{
   if (const FLinearColor* linearColor = Fill.TryGet<FLinearColor>())
   {
      outTint = linearColor->ToFColorSRGB();
      static const FName whiteBrushBoxName = FName("GenericWhiteBox");
      const FSlateBrush* genericWhiteBrush = FCoreStyle::Get().GetBrush(whiteBrushBoxName);
      check(genericWhiteBrush != nullptr);
      return genericWhiteBrush;
   }
   if (const FSlateBrush* slateBrush = Fill.TryGet<FSlateBrush>())
   {
      outTint = OSERadialHelpers::GetSlateBrushTintColor(*slateBrush);
      return slateBrush;
   }
   if (const FBrushWithTint* brushWithTint = Fill.TryGet<FBrushWithTint>())
   {
      outTint = brushWithTint->Value;
      return &brushWithTint->Key;
   }
   outTint = FColor::White;
   return nullptr;
}

FColor FOSEFillPaint::GetSolidColor() const
{
   if (const FLinearColor* linearColor = Fill.TryGet<FLinearColor>())
   {
      return linearColor->ToFColorSRGB();
   }
   if (const FSlateBrush* slateBrush = Fill.TryGet<FSlateBrush>())
   {
      return OSERadialHelpers::GetSlateBrushTintColor(*slateBrush);
   }
   if (const FBrushWithTint* brushAndTint = Fill.TryGet<FBrushWithTint>())
   {
      return brushAndTint->Value;
   }
   return FColor::White;
}

void FOSERadialFillImage::UpdateSlateBrush()
{
   if (Image == CachedSlateBrush.GetResourceObject() && (!UseTintColor || TintColor == CachedSlateBrush.TintColor.GetSpecifiedColor()))
   {
      return;
   }

   if (UPaperSprite* paperSprite = Cast<UPaperSprite>(Image))
   {
      CachedSlateBrush = OSERadialHelpers::MakeBrushFromSprite(paperSprite, FVector2D::Zero());
   }
   else if (Image != nullptr)
   {
      CachedSlateBrush = FSlateImageBrush(Image, FVector2D(32.0f, 32.0f), FSlateColor(GetTintColor()));
   }
   else if (UseTintColor && TintColor.A > 0)
   {
      CachedSlateBrush = FSlateColorBrush(TintColor);
   }
   else
   {
      CachedSlateBrush = FSlateNoResource();
   }
}

namespace OSERadialHelpers
{
   FColor GetSlateBrushTintColor(const FSlateBrush& brush)
   {
      return brush.TintColor.GetSpecifiedColor().ToFColorSRGB();
   }

   FSlateBrush MakeBrushFromSprite(UPaperSprite* sprite, FVector2D iconSize)
   {
      if (sprite != nullptr)
      {
         const FSlateAtlasData spriteAtlasData = sprite->GetSlateAtlasData();
         const FVector2D spriteSize = spriteAtlasData.GetSourceDimensions();
         FSlateBrush brush;
         brush.SetResourceObject(sprite);
         if (iconSize.X <= 0)
         {
            iconSize.X = spriteSize.X;
         }
         if (iconSize.Y <= 0)
         {
            iconSize.Y = spriteSize.Y;
         }
         brush.ImageSize = iconSize;
         return brush;
      }
      return FSlateNoResource();
   }

   // Auto calc resolution by assuming ~1 vertex per angle in degrees
   int32 CalcArcResolution(int32 baseResolution, const FFloatInterval& arcAngleDegrees, float radius, int32 maxResolution)
   {
      baseResolution = FMath::Max(0, baseResolution);
      const float angleDiff = FMath::Abs(arcAngleDegrees.Max - arcAngleDegrees.Min);
      // absolute minimum resolution is 3 if the arc is larger than a half circle, otherwise we can get away with a minimum of 1
      // (imagine trying to render a circle with less than three points)
      float minResolution = (angleDiff >= 180.0f) ? 3 : 1;
      if (baseResolution < minResolution)
      {
         // divide a full circle into this many slices - add an extra min resolution point for each slice the arc covers
         constexpr float addPointPerCircleDivision = 16;
         constexpr float angleDiffDivisor = 360.0f / static_cast<float>(addPointPerCircleDivision);
         minResolution += FMath::Clamp(angleDiff / angleDiffDivisor, 1.0f, addPointPerCircleDivision);

         // Circumference range we'll map to the resolution range. This is arbitrary but seems to give reasonable results.
         static const FVector2f circumferenceToResolutionRange{ 50.0f, 6000.0f };

         // Pick a resolution based on the length of the arc
         const float arcCircumference = UOSERadialPaintLibrary::FindArcLengthDegrees(arcAngleDegrees, radius);
         return FMath::CeilToInt32(FMath::GetMappedRangeValueClamped(
            circumferenceToResolutionRange,
            FVector2f(minResolution, static_cast<float>(maxResolution)),
            arcCircumference));
      }
      return baseResolution;
   }

   float UnwindAngleDegrees360(float angleDeg)
   {
      while (angleDeg < 0)
      {
         angleDeg += 360.0f;
      }
      while (angleDeg > 360)
      {
         angleDeg -= 360.0f;
      }
      return angleDeg;
   }

   FFloatInterval NormalizeArcRangeDegrees(float startAngleDeg, float endAngleDeg)
   {
      const float angleDiffDeg = FMath::Clamp(endAngleDeg - startAngleDeg, 0.0f, 360.0f);
      if (angleDiffDeg >= 360)
      {
         return FFloatInterval{ startAngleDeg, startAngleDeg + 360.0f };
      }

      startAngleDeg = UnwindAngleDegrees360(startAngleDeg);
      return FFloatInterval{ startAngleDeg, startAngleDeg + angleDiffDeg };
   }

   /// Gets the bounding box of a radial slice (eg. pizza slice shape)
   /// This is just intended for debugging and will not return an accurate result unless the resolution template param is the same as the slice
   /// (making resolution a parameter instead of a template argument would necessitate adding a heap allocation and this is just for debugging)
   template<int32 Resolution>
   FBox2f GetRadialDiscBoundingBox(const FVector2f& radialCenter, const FFloatInterval& arcAngleDegrees, const FFloatInterval& radius)
   {
      static_assert(Resolution > 0, "Resolution must be at least 1");
      static constexpr int32 numPoints = (Resolution * 2) + 2;
      FVector2f points[numPoints];
      int32 pointIdx = 0;
      for (int32 i = 0; i < Resolution; i++)
      {
         const float normalizedIndexStart = static_cast<float>(i) / static_cast<float>(Resolution);
         const float sliceRadiansStart = FMath::DegreesToRadians(FMath::Lerp(arcAngleDegrees.Min, arcAngleDegrees.Max, normalizedIndexStart));
         check(pointIdx + 2 <= numPoints);
         points[pointIdx++] = PointOnCircleRadians(radialCenter, radius.Min, sliceRadiansStart);
         points[pointIdx++] = PointOnCircleRadians(radialCenter, radius.Max, sliceRadiansStart);
      }
      check(pointIdx == numPoints - 2);
      const float sliceRadiansEnd = FMath::DegreesToRadians(arcAngleDegrees.Max);
      points[pointIdx++] = PointOnCircleRadians(radialCenter, radius.Min, sliceRadiansEnd);
      points[pointIdx++] = PointOnCircleRadians(radialCenter, radius.Max, sliceRadiansEnd);
      check(pointIdx == numPoints);
      return FBox2f(&points[0], numPoints);
   }

   inline void ExpandBoundingBoxToIncludePoint(FBox2f& bounds, const FVector2f& pt)
   {
      if (!bounds.bIsValid)
      {
         bounds.Min = pt;
         bounds.Max = pt;
         bounds.bIsValid = true;
         return;
      }
      if (pt.X < bounds.Min.X) bounds.Min.X = pt.X;
      if (pt.X > bounds.Max.X) bounds.Max.X = pt.X;
      if (pt.Y < bounds.Min.Y) bounds.Min.Y = pt.Y;
      if (pt.Y > bounds.Max.Y) bounds.Max.Y = pt.Y;
   }

   FORCEINLINE FVector2f RemapUV(const FBox2f& boundingBox, const FVector2f& uvRange, const FVector2f& pt)
   {
      return FVector2f{
         FMath::GetMappedRangeValueUnclamped(FVector2f(boundingBox.Min.X, boundingBox.Max.X), uvRange, pt.X),
         FMath::GetMappedRangeValueUnclamped(FVector2f(boundingBox.Min.Y, boundingBox.Max.Y), uvRange, pt.Y)
      };
   }

} // namespace OSERadialHelpers

//---------------------------------------------------------------------------------------
// FOSELineParams
//---------------------------------------------------------------------------------------

// static
FOSELineParams FOSELineParams::Lerp(const FOSELineParams& a, const FOSELineParams& b, float alpha, EOSEColorBlendMode colorBlendMode)
{
   return FOSELineParams(
      FMath::Lerp(a.Thickness, b.Thickness, alpha),
      UOSERadialPaintLibrary::ColorLerp(a.Color, b.Color, alpha, colorBlendMode));
}

//---------------------------------------------------------------------------------------
// FOSERadialSlice
//---------------------------------------------------------------------------------------

FBox2f FOSERadialSlice::GetBoundingBox() const
{
   return (Background.GetResolution() <= 8)
      ? OSERadialHelpers::GetRadialDiscBoundingBox<8>(FVector2f(Background.Origin), Background.ArcRangeDegrees, GetBackgroundRadius())
      : OSERadialHelpers::GetRadialDiscBoundingBox<32>(FVector2f(Background.Origin), Background.ArcRangeDegrees, GetBackgroundRadius());
}

//---------------------------------------------------------------------------------------
// FOSERadialPaintContext
//---------------------------------------------------------------------------------------

FOSEScopedUMGPaintContext::FOSEScopedUMGPaintContext(FOSERadialPaintContext& radialContext)
   : _radialContext(radialContext)
   , _umgContext(radialContext.ToUMGPaintContext())
{
}

FOSEScopedUMGPaintContext::~FOSEScopedUMGPaintContext()
{
   _radialContext.SetLayerIdFromUMGPaintContext(_umgContext);
}

//---------------------------------------------------------------------------------------
// FOSERadialPaintContext
//---------------------------------------------------------------------------------------

FOSERadialPaintContext::FOSERadialPaintContext(int32& maxLayer, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, const FWidgetStyle& widgetStyle, bool parentEnabled, float renderOpacity)
   : _maxLayer(maxLayer)
   , _allottedGeometry(allottedGeometry)
   , _cullingRect(cullingRect)
   , _outDrawElements(outDrawElements)
   , _widgetStyle(widgetStyle)
   , _parentEnabled(parentEnabled)
   , _renderOpacity(renderOpacity)
{
}

FOSERadialPaintContext::FOSERadialPaintContext(FPaintContext& umgOnPaintContext)
   : _maxLayer(umgOnPaintContext.MaxLayer)
   , _allottedGeometry(umgOnPaintContext.AllottedGeometry)
   , _cullingRect(umgOnPaintContext.MyCullingRect)
   , _outDrawElements(umgOnPaintContext.OutDrawElements)
   , _widgetStyle(umgOnPaintContext.WidgetStyle)
   , _parentEnabled(umgOnPaintContext.bParentEnabled)
{
}

FLinearColor FOSERadialPaintContext::_ApplyGlobalColorEffects(const FLinearColor& color) const
{
   FLinearColor result = color;
   if (_globalTintColorStack.Num() > 0)
   {
      result *= _globalTintColorStack.Last();
   }
   result.A *= _renderOpacity;
   return result;
}

FColor FOSERadialPaintContext::_ApplyGlobalColorEffects(FColor color) const
{
   auto multiplyColorChannel = [](uint8 channelValue, float multiplier) -> uint8
   {
      const float newValue = (static_cast<float>(channelValue) / 255.0f) * multiplier;
      return static_cast<uint8>(FMath::Clamp(static_cast<int32>(newValue * 255.0f), 0, 255));
   };

   float alphaMultiplier = _renderOpacity;

   if (_globalTintColorStack.Num() > 0)
   {
      const FLinearColor& tintColor = _globalTintColorStack.Last();
      color.R = multiplyColorChannel(color.R, tintColor.R);
      color.G = multiplyColorChannel(color.G, tintColor.G);
      color.B = multiplyColorChannel(color.B, tintColor.B);
      alphaMultiplier *= tintColor.A;
   }
   color.A = multiplyColorChannel(color.A, alphaMultiplier);
   return color;
}

FPaintContext FOSERadialPaintContext::ToUMGPaintContext()
{
   return FPaintContext{ _allottedGeometry, _cullingRect, _outDrawElements, _maxLayer, _widgetStyle, _parentEnabled };
}

void FOSERadialPaintContext::SetLayerIdFromUMGPaintContext(const FPaintContext& umgPaintContext)
{
   _maxLayer = FMath::Max(_maxLayer, umgPaintContext.MaxLayer + 1);
}

void FOSERadialPaintContext::DrawLines(TArray<FVector2f> points, const FOSELineParams& params, bool antialias, ESlateDrawEffect drawEffect)
{
   ensure(points.Num() >= 2);
   FSlateDrawElement::MakeLines(
      _outDrawElements,
      _maxLayer++,
      _allottedGeometry.ToPaintGeometry(),
      MoveTemp(points),
      drawEffect,
      _ApplyGlobalColorEffects(params.Color),
      antialias,
      params.Thickness);
}

void FOSERadialPaintContext::DrawText(FVector2f textPosition, const FString& text, int32 fontSize, FName fontName, const FLinearColor& color, bool centered, bool dropShadow, ESlateDrawEffect drawEffect)
{
   if (fontName == NAME_None)
   {
      fontName = "Regular";
   }
   const FSlateFontInfo fontInfo = FCoreStyle::GetDefaultFontStyle(fontName, fontSize);
   const TSharedRef<FSlateFontMeasure> fontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
   const FVector2f textSize = fontMeasureService->Measure(text, fontInfo);
   if (centered)
   {
      textPosition.X -= textSize.X * 0.5f;
   }

   if (dropShadow)
   {
      FSlateDrawElement::MakeText(
         _outDrawElements,
         _maxLayer++,
         _allottedGeometry.ToPaintGeometry(textSize, FSlateLayoutTransform(textPosition + FVector2f(2.0f, 2.0f))),
         text,
         fontInfo,
         drawEffect,
         _ApplyGlobalColorEffects(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f)));
   }

   FSlateDrawElement::MakeText(
      _outDrawElements,
      _maxLayer++,
      _allottedGeometry.ToPaintGeometry(textSize, FSlateLayoutTransform(textPosition)),
      text,
      fontInfo,
      drawEffect,
      _ApplyGlobalColorEffects(color));
}

void FOSERadialPaintContext::DrawBoundingBox(const FVector2f& origin, const FVector2f& size, const FOSELineParams& line, ESlateDrawEffect drawEffect)
{
   if (size.IsNearlyZero() || !line.IsVisible())
   {
      return;
   }
   TArray<FVector2f> points = {
      origin,
      origin + FVector2f(size.X, 0.0f),
      origin + FVector2f(size.X, size.Y),
      origin + FVector2f(0.0f, size.Y),
      origin,
   };
   constexpr bool antialias = false;
   DrawLines(MoveTemp(points), line, antialias, drawEffect);
}

void FOSERadialPaintContext::DrawTriangle(const FVector2f& a, const FVector2f& b, const FVector2f& c, const FOSEFillPaint& fillStyle, float rotateUVsAngleDeg, TArrayView<FVector2f> uvs, ESlateDrawEffect drawEffect)
{
   ensureMsgf(uvs.Num() == 0 || uvs.Num() == 3, TEXT("FOSERadialPaintContext::DrawTriangle expected exactly 0 or 3 UVs, got %d"), uvs.Num());

   // No need to compute bounds if we have explicitly assigned UVs
   const bool autoGenerateUVs = uvs.Num() != 3;

   // Try pulling the tint color from the slate brush if we didn't get an explicit tint color value
   FColor tintColor;
   const FSlateBrush* brush = fillStyle.GetSlateBrushAndTint(tintColor);
   if (!ensure(brush != nullptr))
   {
      return;
   }
   tintColor = _ApplyGlobalColorEffects(tintColor);

   // UVs are temporary - we need the transformed window-space geometry to generate the UVs, so we'll do that after setting up the triangle and index data
   const FSlateRenderTransform& transform = _allottedGeometry.GetAccumulatedRenderTransform();
   TArray<FSlateVertex> vertices;
   vertices.SetNum(3);

   vertices[0] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, a, FVector2f::ZeroVector, tintColor);
   vertices[1] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, b, FVector2f::ZeroVector, tintColor);
   vertices[2] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, c, FVector2f::ZeroVector, tintColor);

   // We only need to compute UVs if our fill style isn't a solid color
   if (!fillStyle.IsSolidColor())
   {
      FBox2f triangleBoundsWindowSpace{ ForceInit };
      if (autoGenerateUVs)
      {
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(triangleBoundsWindowSpace, vertices[0].Position);
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(triangleBoundsWindowSpace, vertices[1].Position);
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(triangleBoundsWindowSpace, vertices[2].Position);

         // If we're not stretching UVs to fit, we may need to expand and crop the bounds to maintain the aspect ratio
         if (!fillStyle.StretchUVsToFit)
         {
            const FVector2f boundsCenter = triangleBoundsWindowSpace.GetCenter();
            const FVector2f boundsSizeHalf = triangleBoundsWindowSpace.GetSize() * 0.5f;

            // If the bounding box isn't a square, crop it on the larger dimension to make it one
            if (boundsSizeHalf.X < boundsSizeHalf.Y)
            {
               triangleBoundsWindowSpace = FBox2f{
                  boundsCenter - FVector2f(boundsSizeHalf.Y),
                  boundsCenter + FVector2f(boundsSizeHalf.Y),
               };
            }
            else if (boundsSizeHalf.X > boundsSizeHalf.Y)
            {
               triangleBoundsWindowSpace = FBox2f{
                  boundsCenter - FVector2f(boundsSizeHalf.X),
                  boundsCenter + FVector2f(boundsSizeHalf.X),
               };
            }
         }
      }

      // Set up UVs
      for (int32 i = 0; i < vertices.Num(); i++)
      {
         FSlateVertex& vert = vertices[i];

         // Generate or retrieve the base UVs
         FVector2f uv = FVector2f::ZeroVector;
         if (autoGenerateUVs)
         {
            uv = OSERadialHelpers::RemapUV(triangleBoundsWindowSpace, FVector2f(0.0f, 1.0f), vert.Position);
         }
         else
         {
            uv = uvs[i];
         }

         // Rotate UVs if specified
         if (!FMath::IsNearlyZero(rotateUVsAngleDeg, 0.001f))
         {
            // Before rotating, translate the UV coordinates so that (0.0, 0.0) is the center instead of (0.5, 0.5)
            const FVector2f offset = FVector2f(0.5f, 0.5f);
            uv = (uv - offset).GetRotated(rotateUVsAngleDeg) + offset;
         }

         vert.TexCoords[0] = uv.X;
         vert.TexCoords[1] = uv.Y;
         vert.MaterialTexCoords.X = vert.TexCoords[0];
         vert.MaterialTexCoords.Y = vert.TexCoords[1];
      }
   }

   const TArray<SlateIndex> indices = { 0, 1, 2 };
   FSlateDrawElement::MakeCustomVerts(
      _outDrawElements,
      _maxLayer++,
      brush->GetRenderingResource(),
      vertices,
      indices,
      nullptr,
      0,
      0,
      drawEffect);
}

void FOSERadialPaintContext::DrawRect(const FVector2f& origin, const FVector2f& size, const FSlateBrush& brush, const TOptional<FColor>& tint, ESlateDrawEffect drawEffect)
{
   FSlateDrawElement::MakeBox(
      _outDrawElements,
      _maxLayer++,
      _allottedGeometry.ToPaintGeometry(size, FSlateLayoutTransform(origin)),
      &brush,
      drawEffect,
      _ApplyGlobalColorEffects(tint ? tint.GetValue() : OSERadialHelpers::GetSlateBrushTintColor(brush)));
}

void FOSERadialPaintContext::DrawLineFromOriginAndAngle(const FVector2f& origin, float angleDegrees, const FFloatInterval& radius, const FOSELineParams& params, bool antialias, ESlateDrawEffect drawEffect)
{
   DrawLine(
      OSERadialHelpers::PointOnCircleDegrees(origin, radius.Min, angleDegrees),
      OSERadialHelpers::PointOnCircleDegrees(origin, radius.Max, angleDegrees),
      params,
      antialias,
      drawEffect);
}

void FOSERadialPaintContext::DrawLineWithCullingRect(const FVector2f& lineA, const FVector2f& lineB, const FOSELineParams& lineParams, const FSlateRect& cullingRect, bool antialias, ESlateDrawEffect drawEffect)
{
   const bool aInside = cullingRect.ContainsPoint(lineA);
   const bool bInside = cullingRect.ContainsPoint(lineB);
   if (aInside && bInside)
   {
      DrawLine(lineA, lineB, lineParams, antialias, drawEffect);
      return;
   }

   UOSERadialPaintLibrary::FIntersectionArray2f intersections;
   if (!UOSERadialPaintLibrary::FindLineSegmentRectEdgeIntersections(lineA, lineB, cullingRect, intersections))
   {
      return;
   }

   if (aInside && !bInside)
   {
      ensure(intersections.Num() == 1);
      DrawLine(lineA, intersections[0], lineParams, antialias, drawEffect);
      return;
   }

   if (!aInside && bInside)
   {
      ensure(intersections.Num() == 1);
      DrawLine(intersections[0], lineB, lineParams, antialias, drawEffect);
      return;
   }

   if (intersections.Num() >= 2)
   {
      DrawLine(intersections[0], intersections[1], lineParams, antialias, drawEffect);
   }
}

void FOSERadialPaintContext::DrawCrosshair(const FVector2f& origin, int32 numSpokes, const FOSELineParams& style, const FFloatInterval& radius, float rotationDegrees, bool antialias, ESlateDrawEffect drawEffect)
{
   if (!style.IsVisible() || FMath::IsNearlyZero(radius.Size()))
   {
      return;
   }
   numSpokes = FMath::Max(1, numSpokes);
   const float angleInterval = 360.0f / static_cast<float>(numSpokes);
   float angle = rotationDegrees;
   for (int32 i = 0; i < numSpokes; i++)
   {
      DrawLineFromOriginAndAngle(origin, angle, radius, style, antialias, drawEffect);
      angle += angleInterval;
   }
}

void FOSERadialPaintContext::DrawCrosshair(const FOSECrosshairShape& crosshairShape, const FVector2f& origin, float reticleRadius, bool antialias, ESlateDrawEffect drawEffect)
{
   DrawCrosshair(
      origin + FVector2f(crosshairShape.OriginOffset),
      crosshairShape.Spokes,
      crosshairShape.Style,
      crosshairShape.GetCrosshairRadius(reticleRadius),
      crosshairShape.RotationDegrees,
      antialias,
      drawEffect);
}

void FOSERadialPaintContext::DrawRadialProgressBar(const FOSERadialProgressBarShape& progressBar, float normalizedValue, const FVector2f& origin, float radiusOffset, ESlateDrawEffect drawEffect)
{
   normalizedValue = FMath::Clamp(normalizedValue, 0.0f, 1.0f);

   ESlateDrawEffect fillDrawEffect = drawEffect;
   if (progressBar.FillImagePremultipliedAlpha)
   {
      fillDrawEffect |= ESlateDrawEffect::PreMultipliedAlpha;
   }

   const float normalizedValueForBorders = progressBar.FitBordersToProgressLevel ? normalizedValue : 1.0f;

   const FFloatInterval indicatorRadius{
      progressBar.Radius + radiusOffset,
      progressBar.Radius + progressBar.Thickness + radiusOffset,
   };

   // Arc/bool pair type.
   // This is needed so that when we precompute the progress bar arcs, the drawing logic can tell if it's a reversed bar or not.
   struct FArcRange
   {
      FFloatInterval Arc;
      bool IsReversed = false;
   };

   // Precompute the progress bar arcs we want to draw.
   // For forward and backward styles there is only one arc, but for middle-out we draw two of them.
   // This is done in a lambda to make it easy to compute this twice if requested (once for the current progress level, once for the full progress bar)
   TArray<FArcRange, TInlineAllocator<2>> arcRanges;
   auto calcArcRanges = [&progressBar, &arcRanges](float arcSizeDeg)
   {
      auto makeArcRange = [](float startDeg, float sizeDeg, bool reverse) -> FArcRange
      {
         const FFloatInterval arc = reverse
            ? OSERadialHelpers::NormalizeArcRangeDegrees(startDeg - sizeDeg, startDeg)
            : OSERadialHelpers::NormalizeArcRangeDegrees(startDeg, startDeg + sizeDeg);
         return FArcRange{ arc, reverse };
      };

      arcRanges.Reset();
      switch (progressBar.ProgressMode)
      {
      case EOSERadialProgressBarMode::Forward:
         arcRanges.Add(makeArcRange(progressBar.ArcAngleDegrees, arcSizeDeg, false));
         break;
      case EOSERadialProgressBarMode::Backward:
         arcRanges.Add(makeArcRange(progressBar.ArcAngleDegrees, arcSizeDeg, true));
         break;
      case EOSERadialProgressBarMode::MiddleOut:
         arcRanges.Add(makeArcRange(progressBar.ArcAngleDegrees, arcSizeDeg / 2.0f, false));
         arcRanges.Add(makeArcRange(progressBar.ArcAngleDegrees, arcSizeDeg / 2.0f, true));
         break;
      default:
         checkNoEntry();
         break;
      }
   };

   int32 arcResolution = progressBar.Resolution;
   if (arcResolution > 1 && arcRanges.Num() > 1)
   {
      arcResolution /= arcRanges.Num();
   }

   // Compute the arc ranges for the progress bar at the current value
   calcArcRanges(progressBar.ArcSizeDegrees * normalizedValue);

   if (progressBar.FillImage.IsVisible())
   {
      const FSlateBrush& fillBrush = progressBar.FillImage.GetSlateBrush();
      for (const FArcRange& arcRange : arcRanges)
      {
         DrawDisc(origin, indicatorRadius, arcRange.Arc, fillBrush, arcResolution, NullOpt, progressBar.UVMode, fillDrawEffect);
      }
   }

   // If we want to fill out borders to show the whole progress bar, recalculate the arc ranges to show the whole progress range
   if (!progressBar.FitBordersToProgressLevel && (progressBar.InnerBorder || progressBar.OuterBorder || progressBar.IndicatorStartCap || progressBar.IndicatorEndCap))
   {
      calcArcRanges(progressBar.ArcSizeDegrees * normalizedValueForBorders);
   }

   constexpr bool antialias = true;

   if (progressBar.InnerBorder)
   {
      for (const FArcRange& arcRange : arcRanges)
      {
         DrawArc(origin, indicatorRadius.Min, arcRange.Arc, progressBar.InnerBorder, arcResolution, antialias, drawEffect);
      }
   }

   if (progressBar.OuterBorder)
   {
      for (const FArcRange& arcRange : arcRanges)
      {
         DrawArc(origin, indicatorRadius.Max, arcRange.Arc, progressBar.OuterBorder, arcResolution, antialias, drawEffect);
      }
   }

   if (progressBar.IndicatorStartCap)
   {
      // Only draw one start cap regardless of how many arc ranges we have.
      // All progress bar styles have the same starting angle.
      DrawLineFromOriginAndAngle(origin, progressBar.ArcAngleDegrees, indicatorRadius, progressBar.IndicatorStartCap, antialias, drawEffect);
   }

   // Logic for determining if the start and end caps would draw in the same exact location.
   auto startAndEndCapsOverlap = [&arcRanges](float normalizedProgress) -> bool
   {
      if (normalizedProgress < 1.0f)
      {
         return false;
      }
      float arcSizeTotal = 0.0f;
      for (const FArcRange& arcRange : arcRanges)
      {
         arcSizeTotal += arcRange.Arc.Size();
      }
      return arcSizeTotal >= 360.0f;
   };

   if (progressBar.IndicatorEndCap && !startAndEndCapsOverlap(normalizedValueForBorders))
   {
      for (const FArcRange& arcRange : arcRanges)
      {
         DrawLineFromOriginAndAngle(origin, arcRange.IsReversed ? arcRange.Arc.Min : arcRange.Arc.Max, indicatorRadius, progressBar.IndicatorEndCap, antialias, drawEffect);
      }
   }
}

void FOSERadialPaintContext::DrawArrow(const FVector2f& lineA, const FVector2f& lineB, const FOSELineParams& lineParams, float arrowheadSize, const TOptional<FOSEFillPaint>& arrowheadFill,
   float arrowheadAngle, const TOptional<FOSELineParams>& arrowheadBorder, const TOptional<FSlateRect>& cullingRect, bool antialias, ESlateDrawEffect drawEffect)
{
   auto drawArrowLineSegment = [this, &cullingRect, antialias, drawEffect](const FVector2f& a, const FVector2f& b, const FOSELineParams& lineStyle)
   {
      if ((b - a).SquaredLength() <= 0.1f)
      {
         return;
      }
      if (cullingRect)
      {
         DrawLineWithCullingRect(a, b, lineStyle, *cullingRect, antialias, drawEffect);
      }
      else
      {
         DrawLine(a, b, lineStyle, antialias, drawEffect);
      }
   };

   // If we don't have an arrowhead for some reason, just draw a line and we're done
   if (arrowheadAngle <= 0 || arrowheadSize <= 0)
   {
      drawArrowLineSegment(lineA, lineB, lineParams);
      return;
   }

   const FVector2f dir = (lineB - lineA).GetSafeNormal();
   const FVector2f arrowheadPointA = lineB - (dir.GetRotated(+arrowheadAngle) * arrowheadSize);
   const FVector2f arrowheadPointB = lineB - (dir.GetRotated(-arrowheadAngle) * arrowheadSize);

   // If we're using a filled arrowhead, only draw the line up to the point the arrowhead starts (instead of to the tip of the arrowhead)
   if (arrowheadFill)
   {
      FVector2f arrowheadLineIntersection = FVector2f::ZeroVector;
      if (UOSERadialPaintLibrary::FindLineSegmentIntersection(lineA, lineB, arrowheadPointA, arrowheadPointB, arrowheadLineIntersection))
      {
         drawArrowLineSegment(lineA, arrowheadLineIntersection, lineParams);
      }
      else
      {
         // If we didn't find an intersection, the arrowhead is probably too large and the line won't fit, so just don't draw it
      }
   }
   else
   {
      drawArrowLineSegment(lineA, lineB, lineParams);
   }

   if (arrowheadFill)
   {
      // No need to rotate the UVs because we're explicitly passing UVs for each vertex instead of projecting them
      constexpr float rotateUVAngleDeg = 0.0f;

      static constexpr int32_t numBaseUVs = 3;
      FVector2f triangleUVs[numBaseUVs] = {
         FVector2f(0.0f, 0.0f),
         FVector2f(0.0f, 1.0f),
         FVector2f(1.0f, 0.5f),
      };
      DrawTriangle(arrowheadPointA, arrowheadPointB, lineB, *arrowheadFill, rotateUVAngleDeg, TArrayView<FVector2f>(triangleUVs, numBaseUVs), drawEffect);
   }
   else
   {
      drawArrowLineSegment(lineB, arrowheadPointA, lineParams);
      drawArrowLineSegment(lineB, arrowheadPointB, lineParams);
   }

   if (arrowheadBorder)
   {
      drawArrowLineSegment(lineB, arrowheadPointA, *arrowheadBorder);
      drawArrowLineSegment(lineB, arrowheadPointB, *arrowheadBorder);
      drawArrowLineSegment(arrowheadPointA, arrowheadPointB, *arrowheadBorder);
   }
}

void FOSERadialPaintContext::DrawArrow(const FVector2f& lineA, const FVector2f& lineB, const FOSEArrowStyle& style, const TOptional<FSlateRect>& cullingRect, bool antialias, ESlateDrawEffect drawEffect)
{
   DrawArrow(lineA, lineB, style.Line, style.ArrowheadSize, style.GetArrowheadFillStyle(), style.ArrowheadAngle, style.GetArrowheadBorder(), cullingRect, antialias, drawEffect);
}

void FOSERadialPaintContext::DrawArc(const FVector2f& origin, float radius, FFloatInterval arcAngleDegrees, const FOSELineParams& params, int32 resolution, bool antialias, ESlateDrawEffect drawEffect)
{
   if (!params.IsVisible())
   {
      return;
   }
   if (arcAngleDegrees.Min > arcAngleDegrees.Max)
   {
      std::swap(arcAngleDegrees.Min, arcAngleDegrees.Max);
   }
   if (arcAngleDegrees.Max - arcAngleDegrees.Min < UE_KINDA_SMALL_NUMBER)
   {
      return;
   }
   resolution = OSERadialHelpers::CalcArcResolution(resolution, arcAngleDegrees, radius);
   check(resolution > 0);
   TArray<FVector2f> points;
   points.SetNumUninitialized(resolution + 1);
   for (int32 i = 0; i < points.Num(); i++)
   {
      points[i] = OSERadialHelpers::PointOnCircleDegrees(origin, radius, FMath::Lerp(arcAngleDegrees.Min, arcAngleDegrees.Max, static_cast<float>(i) / static_cast<float>(resolution)));
   }
   DrawLines(MoveTemp(points), params, antialias, drawEffect);
}

void FOSERadialPaintContext::DrawDisc(const FVector2f& origin, const FFloatInterval& radius, const FFloatInterval& arcAngleDegrees, const FSlateBrush& brush, int32 resolution, const TOptional<FColor>& tint, EOSERadialDiscUVMode uvMode, ESlateDrawEffect drawEffect)
{
   resolution = OSERadialHelpers::CalcArcResolution(resolution, arcAngleDegrees, radius.Max);
   check(resolution > 0);

   TArray<FSlateVertex> vertices;
   TArray<SlateIndex> indices;
   vertices.Reserve(resolution * 4);
   indices.Reserve(resolution * 6);

   // Try pulling the tint color from the slate brush if we didn't get an explicit tint color value
   const FColor tintColor = _ApplyGlobalColorEffects(tint ? tint.GetValue() : OSERadialHelpers::GetSlateBrushTintColor(brush));

   const FSlateRenderTransform& transform = _allottedGeometry.GetAccumulatedRenderTransform();

   // We can pre-compute the bounding box needed for a disc origin uv projection
   FBox2f discFullRadialBoundsDrawSpace{ ForceInit };
   if (uvMode == EOSERadialDiscUVMode::ProjectFullRadial)
   {
      discFullRadialBoundsDrawSpace = FBox2f(
         origin - FVector2f(radius.Max, radius.Max),
         origin + FVector2f(radius.Max, radius.Max));
   }

   // We can't pre-compute a bounding box projection because we don't know the points yet.
   // Since we can't apply the UVs until after we generate the geometry, and the geometry has to be in window-space, we'll compute the bounds
   // in window-space so we can project the UVs without needing to un-transforming all the vertices.
   FBox2f discBoundsWindowSpace{ ForceInit };
   const bool needDiscBoundingBoxInWindowSpace = uvMode == EOSERadialDiscUVMode::ProjectBoundingBoxStretch || uvMode == EOSERadialDiscUVMode::ProjectBoundingBoxCenter;

   //TODO: Optimize this by having each segment share connected vertices
   for (int32 i = 0; i < resolution; i++)
   {
      const float normalizedIndexStart = static_cast<float>(i) / static_cast<float>(resolution);
      const float normalizedIndexEnd = static_cast<float>(i + 1) / static_cast<float>(resolution);
      const float sliceStartDegrees = FMath::Lerp(arcAngleDegrees.Min, arcAngleDegrees.Max, normalizedIndexStart);
      const float sliceEndDegrees = FMath::Lerp(arcAngleDegrees.Min, arcAngleDegrees.Max, normalizedIndexEnd);

      const FVector2f pt0 = OSERadialHelpers::PointOnCircleDegrees(origin, radius.Min, sliceStartDegrees);
      const FVector2f pt1 = OSERadialHelpers::PointOnCircleDegrees(origin, radius.Max, sliceStartDegrees);
      const FVector2f pt2 = OSERadialHelpers::PointOnCircleDegrees(origin, radius.Max, sliceEndDegrees);
      const FVector2f pt3 = OSERadialHelpers::PointOnCircleDegrees(origin, radius.Min, sliceEndDegrees);

      // The curved UV mode is super cheap to generate UVs for (we literally already have the values), so we'll just use that as the default
      FVector2f uv0{ normalizedIndexStart, 0.0f };
      FVector2f uv1{ normalizedIndexStart, 1.0f };
      FVector2f uv2{ normalizedIndexEnd, 1.0f };
      FVector2f uv3{ normalizedIndexEnd, 0.0f };

      if (uvMode == EOSERadialDiscUVMode::CurvedFullCircle)
      {
         // Just like the curved UV mode, but interpolated around the whole circle instead of just the disc's arc range
         const float normalizedCircleStart = FMath::GetMappedRangeValueClamped(FVector2f(0.0f, 360.0f), FVector2f(0.0f, 1.0f), sliceStartDegrees);
         const float normalizedCircleEnd = FMath::GetMappedRangeValueClamped(FVector2f(0.0f, 360.0f), FVector2f(0.0f, 1.0f), sliceEndDegrees);
         uv0.X = normalizedCircleStart;
         uv1.X = normalizedCircleStart;
         uv2.X = normalizedCircleEnd;
         uv3.X = normalizedCircleEnd;
      }
      else if (uvMode == EOSERadialDiscUVMode::ProjectFullRadial)
      {
         uv0 = OSERadialHelpers::RemapUV(discFullRadialBoundsDrawSpace, FVector2f(0.0f, 1.0f), pt0);
         uv1 = OSERadialHelpers::RemapUV(discFullRadialBoundsDrawSpace, FVector2f(0.0f, 1.0f), pt1);
         uv2 = OSERadialHelpers::RemapUV(discFullRadialBoundsDrawSpace, FVector2f(0.0f, 1.0f), pt2);
         uv3 = OSERadialHelpers::RemapUV(discFullRadialBoundsDrawSpace, FVector2f(0.0f, 1.0f), pt3);
      }

      const int32 idx0 = vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, pt0, uv0, tintColor));
      const int32 idx1 = vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, pt1, uv1, tintColor));
      const int32 idx2 = vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, pt2, uv2, tintColor));
      const int32 idx3 = vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(transform, pt3, uv3, tintColor));

      if (needDiscBoundingBoxInWindowSpace)
      {
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(discBoundsWindowSpace, vertices[idx0].Position);
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(discBoundsWindowSpace, vertices[idx1].Position);
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(discBoundsWindowSpace, vertices[idx2].Position);
         OSERadialHelpers::ExpandBoundingBoxToIncludePoint(discBoundsWindowSpace, vertices[idx3].Position);
      }

      indices.Add(idx0);
      indices.Add(idx1);
      indices.Add(idx2);
      indices.Add(idx0);
      indices.Add(idx2);
      indices.Add(idx3);
   }

   // For local bounds UVs, we need to do a second pass to project the UVs because we didn't have the complete bounding box until after the geometry was generated
   if (uvMode == EOSERadialDiscUVMode::ProjectBoundingBoxStretch)
   {
      for (FSlateVertex& vert : vertices)
      {
         const FVector2f uv = OSERadialHelpers::RemapUV(discBoundsWindowSpace, FVector2f(0.0f, 1.0f), vert.Position);
         vert.TexCoords[0] = uv.X;
         vert.TexCoords[1] = uv.Y;
         vert.MaterialTexCoords.X = vert.TexCoords[0];
         vert.MaterialTexCoords.Y = vert.TexCoords[1];
      }
   }
   else if (uvMode == EOSERadialDiscUVMode::ProjectBoundingBoxCenter)
   {
      const FVector2f boundsCenter = discBoundsWindowSpace.GetCenter();
      const FVector2f boundsSizeHalf = discBoundsWindowSpace.GetSize() * 0.5f;

      // If the bounding box isn't a square, crop it on the larger dimension to make it one
      if (boundsSizeHalf.X < boundsSizeHalf.Y)
      {
         discBoundsWindowSpace = FBox2f{
            boundsCenter - FVector2f(boundsSizeHalf.Y),
            boundsCenter + FVector2f(boundsSizeHalf.Y),
         };
      }
      else if (boundsSizeHalf.X > boundsSizeHalf.Y)
      {
         discBoundsWindowSpace = FBox2f{
            boundsCenter - FVector2f(boundsSizeHalf.X),
            boundsCenter + FVector2f(boundsSizeHalf.X),
         };
      }

      for (FSlateVertex& vert : vertices)
      {
         const FVector2f uv = OSERadialHelpers::RemapUV(discBoundsWindowSpace, FVector2f(0.0f, 1.0f), vert.Position);
         vert.TexCoords[0] = uv.X;
         vert.TexCoords[1] = uv.Y;
         vert.MaterialTexCoords.X = vert.TexCoords[0];
         vert.MaterialTexCoords.Y = vert.TexCoords[1];
      }
   }

   FSlateDrawElement::MakeCustomVerts(
      _outDrawElements,
      _maxLayer++,
      brush.GetRenderingResource(),
      vertices,
      indices,
      nullptr,
      0,
      0,
      drawEffect);
}

void FOSERadialPaintContext::DrawSlice(const FOSERadialSlice& slice, const FVector2f& originOffset, ESlateDrawEffect drawEffect)
{
   if (slice.IsHidden)
   {
      return;
   }

   // If the slice is marked as disabled, use the slate "disabled" effect (it'll render as grayed out)
   if (slice.IsDisabled)
   {
      drawEffect |= ESlateDrawEffect::DisabledEffect;
   }

   const FVector2f sliceOrigin = originOffset + FVector2f(slice.Background.Origin);
   const FFloatInterval backgroundRadius = slice.GetBackgroundRadius();
   constexpr bool antialias = true;

   // Item background brush
   if (slice.UseHoverBackgroundBrush)
   {
      // Crossfade between the hover brush and the regular background brush
      const float hoverBackgroundAlpha = FMath::Clamp(slice.HoverLerpAlpha, 0.0f, 1.0f);
      const float normalBackgroundAlpha = 1.0f - hoverBackgroundAlpha;

      auto byteToFloat = [](uint8 v) -> float { return static_cast<float>(v) / 255.0f; };
      auto floatToByte = [](float v) -> uint8 { return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt32(v * 255.0f), 0, 255)); };

      if (normalBackgroundAlpha > 0)
      {
         FColor tint = OSERadialHelpers::GetSlateBrushTintColor(slice.Background.Brush);
         tint.A = floatToByte(byteToFloat(tint.A) * normalBackgroundAlpha);
         DrawDisc(
            sliceOrigin,
            backgroundRadius,
            slice.Background.ArcRangeDegrees,
            slice.Background.Brush,
            slice.Background.Resolution,
            tint,
            slice.Background.UVMode,
            drawEffect);
      }

      if (hoverBackgroundAlpha > 0)
      {
         FColor tint = OSERadialHelpers::GetSlateBrushTintColor(slice.HoverBackgroundBrush);
         tint.A = floatToByte(byteToFloat(tint.A) * hoverBackgroundAlpha);
         DrawDisc(
            sliceOrigin,
            backgroundRadius,
            slice.Background.ArcRangeDegrees,
            slice.HoverBackgroundBrush,
            slice.Background.Resolution,
            tint,
            slice.Background.UVMode,  // TODO: A HoverBackgroundBrushUVMode property might be a good idea
            drawEffect);
      }
   }
   else
   {
      DrawDisc(
         sliceOrigin,
         backgroundRadius,
         slice.Background.ArcRangeDegrees,
         slice.Background.Brush,
         slice.Background.Resolution,
         NullOpt,
         slice.Background.UVMode,
         drawEffect);
   }

   // Item side borders
   if (const FOSELineParams borderSide = slice.GetBorderSide())
   {
      const float borderSideOffsetDeg = slice.GetBorderSideOffsetDegrees();
      DrawLineFromOriginAndAngle(sliceOrigin, slice.Background.ArcRangeDegrees.Min - borderSideOffsetDeg, backgroundRadius, borderSide, antialias, drawEffect);
      DrawLineFromOriginAndAngle(sliceOrigin, slice.Background.ArcRangeDegrees.Max + borderSideOffsetDeg, backgroundRadius, borderSide, antialias, drawEffect);
   }

   // Item inner border
   if (backgroundRadius.Min > 0)
   {
      if (const FOSELineParams borderInner = slice.GetBorderInner())
      {
         const float borderInnerOffset = slice.GetBorderInnerOffsetRadius();
         DrawArc(sliceOrigin, backgroundRadius.Min - borderInnerOffset, slice.Background.ArcRangeDegrees, borderInner, slice.Background.Resolution, antialias, drawEffect);
      }
   }

   // Item outer border
   if (backgroundRadius.Max > 0)
   {
      if (const FOSELineParams borderOuter = slice.GetBorderOuter())
      {
         const float borderOuterOffset = slice.GetBorderOuterOffsetRadius();
         DrawArc(sliceOrigin, backgroundRadius.Max + borderOuterOffset, slice.Background.ArcRangeDegrees, borderOuter, slice.Background.Resolution, antialias, drawEffect);
      }
   }
}

//---------------------------------------------------------------------------------------
// UOSERadialPaintLibrary
//---------------------------------------------------------------------------------------

// static
bool UOSERadialPaintLibrary::AngleContainedInRadialArc(float angleDegrees, FFloatInterval arcAngleDegrees)
{
   auto angleGreaterOrEqual = [](float a, float b) { FMath::WindRelativeAnglesDegrees(a, b); return a >= b; };
   auto angleLessOrEqual = [](float a, float b) { FMath::WindRelativeAnglesDegrees(a, b); return a <= b; };
   const float angleMidpoint = FMath::Lerp(arcAngleDegrees.Min, arcAngleDegrees.Max, 0.5f);
   const float halfArcLength = (arcAngleDegrees.Max - arcAngleDegrees.Min) * 0.5f;
   angleDegrees = FMath::UnwindDegrees(angleDegrees);
   return angleGreaterOrEqual(angleDegrees, angleMidpoint - halfArcLength) && angleLessOrEqual(angleDegrees, angleMidpoint + halfArcLength);
}

// static
float UOSERadialPaintLibrary::FindAngleDegreesBetweenNormals2D(const FVector2D& from, const FVector2D& to)
{
   const float angle = FMath::Acos(FVector2D::DotProduct(from, to));
   return FMath::RadiansToDegrees(FVector2D::CrossProduct(from, to) >= 0 ? angle : -angle);
}

// static
bool UOSERadialPaintLibrary::FindLineSegmentIntersection(const FVector2f& a0, const FVector2f& a1, const FVector2f& b0, const FVector2f& b1, FVector2f& outIntersection)
{
   // Simple wrapper around FMath::SegmentIntersection2D that takes 2D vectors
   FVector intersect = FVector::ZeroVector;
   const bool result = FMath::SegmentIntersection2D(
      FVector(a0.X, a0.Y, 0),
      FVector(a1.X, a1.Y, 0),
      FVector(b0.X, b0.Y, 0),
      FVector(b1.X, b1.Y, 0),
      intersect);
   if (result)
   {
      outIntersection = FVector2f(intersect.X, intersect.Y);
   }
   return result;
}

// static
bool UOSERadialPaintLibrary::FindLineSegmentRectEdgeIntersections(const FVector2f& lineStart, const FVector2f& lineEnd, const FSlateRect& rect, FIntersectionArray2f& outIntersections)
{
   outIntersections.Reset();

   // Check to see if the line segment overlaps any rect edges - if so we technically have infinite intersections.
   // Since this function is primarily for culling purposes, just return no intersections for these cases.
   if ((lineStart.X == lineEnd.X && (lineStart.X <= rect.Left || lineStart.X >= rect.Right))
      || (lineStart.Y == lineEnd.Y && (lineStart.Y <= rect.Top || lineStart.Y >= rect.Bottom)))
   {
      return false;
   }

   // If both the start and end points are inside the rect, there can't be any part of the line that intersects the sides
   if (rect.ContainsPoint(lineStart) && rect.ContainsPoint(lineEnd))
   {
      return false;
   }

   FVector2f intersection;

   // Look for an intersection between the line and all four edges of the rect
   if (FindLineSegmentIntersection(lineStart, lineEnd, rect.GetTopLeft2f(), rect.GetTopRight2f(), intersection))
   {
      outIntersections.Add(intersection);
   }
   if (FindLineSegmentIntersection(lineStart, lineEnd, rect.GetTopRight(), rect.GetBottomRight2f(), intersection))
   {
      outIntersections.Add(intersection);
   }
   if (FindLineSegmentIntersection(lineStart, lineEnd, rect.GetBottomRight2f(), rect.GetBottomLeft2f(), intersection))
   {
      outIntersections.Add(intersection);
   }
   if (FindLineSegmentIntersection(lineStart, lineEnd, rect.GetBottomLeft2f(), rect.GetTopLeft2f(), intersection))
   {
      outIntersections.Add(intersection);
   }

   return outIntersections.Num() > 0;
}

// static
FLinearColor UOSERadialPaintLibrary::ColorLerp(const FLinearColor& base, const FLinearColor& top, float alpha, EOSEColorBlendMode blendMode)
{
   if (alpha <= 0)
   {
      return base;
   }
   if (alpha >= 1)
   {
      return top;
   }
   switch (blendMode)
   {
   case EOSEColorBlendMode::Linear:
      return FLinearColor(
         FMath::Lerp(base.R, top.R, alpha),
         FMath::Lerp(base.G, top.G, alpha),
         FMath::Lerp(base.B, top.B, alpha),
         FMath::Lerp(base.A, top.A, alpha));
   case EOSEColorBlendMode::Overlay:
      return LerpColorUsingOverlayBlend(base, top, alpha);
   case EOSEColorBlendMode::HSV:
      return FLinearColor::LerpUsingHSV(base, top, alpha);
   default:
      checkNoEntry();
      break;
   }
   return base;
}

// static
FLinearColor UOSERadialPaintLibrary::ColorBlendOverlay(const FLinearColor& base, const FLinearColor& top, float resultAlphaChannel)
{
   // See also: https://en.wikipedia.org/wiki/Blend_modes
   auto overlay = [](float v0, float v1) -> float
   {
      v0 = FMath::Clamp(v0, 0.0f, 1.0f);
      v1 = FMath::Clamp(v1, 0.0f, 1.0f);
      return (v1 < 0.5f) ? (2.0f * v0 * v1) : (1.0f - 2.0f * (1.0f - v0) * (1.0f - v1));
   };
   return FLinearColor(
      overlay(base.R, top.R),
      overlay(base.G, top.G),
      overlay(base.B, top.B),
      resultAlphaChannel);
}

// static
FLinearColor UOSERadialPaintLibrary::LerpColorUsingOverlayBlend(const FLinearColor& base, const FLinearColor& top, float alpha, float midpoint)
{
   const FLinearColor intermediateColor = ColorBlendOverlay(base, top);
   if (alpha <= midpoint)
   {
      const float lerpAlpha = FMath::GetMappedRangeValueClamped(FVector2f(0.0f, midpoint), FVector2f(0.0f, 1.0f), alpha);
      return FLinearColor(
         FMath::Lerp(base.R, intermediateColor.R, lerpAlpha),
         FMath::Lerp(base.G, intermediateColor.G, lerpAlpha),
         FMath::Lerp(base.B, intermediateColor.B, lerpAlpha),
         FMath::Lerp(base.A, top.A, alpha));
   }
   else
   {
      const float lerpAlpha = FMath::GetMappedRangeValueClamped(FVector2f(midpoint, 1.0f), FVector2f(0.0f, 1.0f), alpha);
      return FLinearColor(
         FMath::Lerp(intermediateColor.R, top.R, lerpAlpha),
         FMath::Lerp(intermediateColor.G, top.G, lerpAlpha),
         FMath::Lerp(intermediateColor.B, top.B, lerpAlpha),
         FMath::Lerp(base.A, top.A, alpha));
   }
}

// static
FVector2D UOSERadialPaintLibrary::GetPaintContextScreenSpaceOrigin(FPaintContext& context)
{
   return context.AllottedGeometry.GetAbsolutePosition();
}

// static
FVector2D UOSERadialPaintLibrary::GetPaintContextLocalSize(FPaintContext& context)
{
   return context.AllottedGeometry.GetLocalSize();
}

// static
bool UOSERadialPaintLibrary::TransformWorldPositionToPaintSpace(APlayerController* playerController, UPARAM(ref) FPaintContext& context, FVector worldPosition, FVector2D& position)
{
   if (playerController != nullptr)
   {
      FVector2D screenSpaceCoord = FVector2D::Zero();
      if (UGameplayStatics::ProjectWorldToScreen(playerController, worldPosition, screenSpaceCoord))
      {
         position = TransformScreenSpaceToPaintSpace(playerController, context, screenSpaceCoord);
         return true;
      }
   }
   position = FVector2D::Zero();
   return false;
}

// static
FVector2D UOSERadialPaintLibrary::TransformScreenSpaceToPaintSpace(UObject* contextObject, UPARAM(ref) FPaintContext& context, FVector2D screenSpaceCoord)
{
   FVector2D absWidgetCoord = FVector2D::Zero();
   constexpr bool removeWindowPosition = true;
   USlateBlueprintLibrary::ScreenToWidgetAbsolute(contextObject, screenSpaceCoord, absWidgetCoord, removeWindowPosition);
   return context.AllottedGeometry.AbsoluteToLocal(absWidgetCoord);
}

// static
void UOSERadialPaintLibrary::Radial_DrawLineCustom(FPaintContext& context, const TArray<FVector2D>& points, FOSELineParams params)
{
   if (points.Num() < 2 || !params.IsVisible())
   {
      return;
   }
   TArray<FVector2f> pts;
   pts.SetNumUninitialized(points.Num());
   for (int32 i = 0; i < points.Num(); i++)
   {
      pts[i].X = static_cast<float>(points[i].X);
      pts[i].Y = static_cast<float>(points[i].Y);
   }
   FOSERadialPaintContext(context).DrawLines(MoveTemp(pts), params);
}

// static
void UOSERadialPaintLibrary::Radial_DrawDisc(FPaintContext& context, FVector2D origin, FFloatInterval radius, FFloatInterval arcAngleDegrees, const FSlateBrush& brush, int32 resolution, bool applyTint, FLinearColor tintColor, EOSERadialDiscUVMode uvMode, bool drawDisabled)
{
   const TOptional<FColor> tint = applyTint ? TOptional(tintColor.ToFColorSRGB()) : NullOpt;
   const ESlateDrawEffect drawEffect = drawDisabled ? ESlateDrawEffect::DisabledEffect : ESlateDrawEffect::None;
   FOSERadialPaintContext(context).DrawDisc(FVector2f(origin), radius, arcAngleDegrees, brush, resolution, tint, uvMode, drawEffect);
}
