// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Blueprint/UserWidget.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OSERadialPaintLibrary.generated.h"

class UPaperSprite;
class FOSERadialPaintContext;

UENUM(BlueprintType)
enum class EOSEColorBlendMode : uint8
{
   Linear,
   HSV,
   Overlay,
   MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EOSERadialDiscUVMode : uint8
{
   Curved UMETA(ToolTip="Maps the UVs 1:1 with the disc shape"),
   CurvedFullCircle UMETA(DisplayName="Curved (360 deg)", ToolTip="Maps the UVs to the disc shape using a full 360 degree arc"),
   ProjectFullRadial UMETA(DisplayName="Project Onto Full Radial", ToolTip="Projects UVs onto the disc using the origin and max radius as the bounding box (eg. acts like the arc range is always 0..360)"),
   ProjectBoundingBoxStretch UMETA(DisplayName="Project Onto Bounding Box (Stretched)", ToolTip="Projects UVs using the disc's exact bounding box, stretching it to fill the area"),
   ProjectBoundingBoxCenter UMETA(DisplayName="Project Onto Bounding Box (Centered)", ToolTip="Projects UVs using the disc's exact bounding box, maintaining the texture's aspect ratio"),
   MAX UMETA(Hidden)
};

/// Fill style variant type that can represent a solid color, slate brush, or a slate brush and associated tint color.
struct OSECORE_API FOSEFillPaint
{
   using FBrushWithTint = TPair<FSlateBrush, FColor>;
   using FFillVariant = TVariant<FLinearColor, FSlateBrush, FBrushWithTint>;

   /// The fill color, material, or texture
   FFillVariant Fill;

   /// For slate brushes with textures, stretch the generated UVs to cover all vertices?
   /// If false, maintains a square aspect ratio with a crop factor.
   bool StretchUVsToFit = false;

   FOSEFillPaint() { Fill.Set<FLinearColor>(FLinearColor::White); }
   FOSEFillPaint(const FLinearColor& color) { Fill.Set<FLinearColor>(color); }
   FOSEFillPaint(const FSlateBrush& brush, bool stretchUVsToFit = false) : StretchUVsToFit(stretchUVsToFit) { Fill.Set<FSlateBrush>(brush); }
   FOSEFillPaint(const FSlateBrush& brush, const FColor& tint, bool stretchUVsToFit = false) : StretchUVsToFit(stretchUVsToFit) { Fill.Set<FBrushWithTint>({ brush, tint }); }

   FOSEFillPaint(const FOSEFillPaint&) = default;
   FOSEFillPaint& operator=(const FOSEFillPaint&) = default;

   // NB. This can return a pointer to the brush contained in the fill style, so if you need the lifetime of the return value to last past the current
   // function scope, you should make a copy of the slate brush.
   const FSlateBrush* GetSlateBrushAndTint(FColor& outTint) const;

   FORCEINLINE bool IsSolidColor() const { return Fill.IsType<FLinearColor>(); }

   FColor GetSolidColor() const;
};

/// Wrapper around FSlateBrush that provides a much nicer blueprints UI because it doesn't expose a bunch of unused fields.
USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialFillImage
{
   GENERATED_BODY()

   //TODO: Support paper sprites as images. They'll show up in the Image dropdown if you add "/Script/Engine.SlateTextureAtlasInterface" to AllowedClasses, but they don't render correctly.

   /// Texture to use as the fill image
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Fill Image", meta=(DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture,/Script/Engine.MaterialInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
   TObjectPtr<UObject> Image;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Fill Image", Meta = (InlineEditConditionToggle))
   bool UseTintColor = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Fill Image", Meta = (EditCondition = "UseTintColor"))
   FLinearColor TintColor = FLinearColor::White;

   UPROPERTY(Transient)
   FSlateBrush CachedSlateBrush;

   void UpdateSlateBrush();

   FLinearColor GetTintColor() const
   {
      return UseTintColor ? TintColor : FLinearColor::White;
   }

   bool IsVisible() const
   {
      return Image != nullptr || (UseTintColor && TintColor.A > 0);
   }

   const FSlateBrush& GetSlateBrush() const
   {
      const_cast<FOSERadialFillImage*>(this)->UpdateSlateBrush();
      return CachedSlateBrush;
   }
};

namespace OSERadialHelpers
{
   template<typename VecT>
   FORCEINLINE VecT PointOnCircleRadians(const VecT& circleOrigin, float circleRadius, float angleRadians)
   {
      return VecT{
         circleOrigin.X + (FMath::Cos(angleRadians) * circleRadius),
         circleOrigin.Y + (FMath::Sin(angleRadians) * circleRadius),
      };
   }

   template<typename VecT>
   FORCEINLINE VecT PointOnCircleDegrees(const VecT& circleOrigin, float circleRadius, float angleDegrees)
   {
      return PointOnCircleRadians<VecT>(circleOrigin, circleRadius, FMath::DegreesToRadians(angleDegrees));
   }

   /// Extracts the tint color from a slate brush
   OSECORE_API FColor GetSlateBrushTintColor(const FSlateBrush& brush);

   /// Constructs a slate brush from a paper sprite object
   OSECORE_API FSlateBrush MakeBrushFromSprite(UPaperSprite* sprite, FVector2D iconSize);

   /// Auto calc resolution by assuming ~1 vertex per angle in degrees
   OSECORE_API int32 CalcArcResolution(int32 baseResolution, const FFloatInterval& arcAngleDegrees, float radius, int32 maxResolution = 300);

   /// Linear interpolation for float intervals
   FORCEINLINE FFloatInterval LerpFloatInterval(const FFloatInterval& a, const FFloatInterval& b, float alpha)
   {
      return FFloatInterval(
         FMath::Lerp(a.Min, b.Min, alpha),
         FMath::Lerp(a.Max, b.Max, alpha));
   }

   OSECORE_API float UnwindAngleDegrees360(float angleDeg);

   OSECORE_API FFloatInterval NormalizeArcRangeDegrees(float startAngleDeg, float endAngleDeg);
}

/// Line style data
USTRUCT(BlueprintType)
struct OSECORE_API FOSELineParams
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line", meta = (ClampMin = "0", UIMin = "0"))
   float Thickness = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Line")
   FLinearColor Color = FLinearColor::White;

   FOSELineParams() = default;
   explicit FOSELineParams(float thickness, const FLinearColor& color = FLinearColor::White) : Thickness(thickness), Color(color) {}

   FORCEINLINE bool IsVisible() const { return Thickness > 0 && Color.A > 0; }
   FORCEINLINE explicit operator bool() const { return IsVisible(); }

   /// Linear interpolation of line params
   static FOSELineParams Lerp(const FOSELineParams& a, const FOSELineParams& b, float alpha, EOSEColorBlendMode colorBlendMode);
};

UENUM(BlueprintType)
enum class EOSEArrowheadStyle : uint8
{
   Lines,
   SolidColor,
   Brush,
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEArrowStyle
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape")
   FOSELineParams Line = FOSELineParams{ 2.0f, FLinearColor::White };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape")
   EOSEArrowheadStyle ArrowheadStyle = EOSEArrowheadStyle::Lines;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (EditCondition = "ArrowheadStyle == EOSEArrowheadStyle::SolidColor", EditConditionHides))
   FLinearColor ArrowheadColor = FLinearColor::White;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (EditCondition = "ArrowheadStyle == EOSEArrowheadStyle::Brush", EditConditionHides))
   FSlateBrush ArrowheadBrush;

   /// When auto-generating triangle UVs, stretches UV coordinates to fit the bounding box of the triangle
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (EditCondition = "ArrowheadStyle == EOSEArrowheadStyle::Brush", EditConditionHides))
   bool ArrowheadBrushStretchUVsToFit = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (InlineEditConditionToggle))
   bool UseArrowheadBorder = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (EditCondition = "UseArrowheadBorder"))
   FOSELineParams ArrowheadBorder = FOSELineParams{ 1.0f, FLinearColor::Black };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (UIMin = 0, ClampMin = 0))
   float ArrowheadSize = 40.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Arrow Shape", Meta = (UIMin = 0, ClampMin = 0, UIMax = 90, ClampMax = 90, ForceUnits = "degrees"))
   float ArrowheadAngle = 30.0f;

   TOptional<FOSELineParams> GetArrowheadBorder() const
   {
      if (UseArrowheadBorder)
      {
         return ArrowheadBorder;
      }
      return NullOpt;
   }

   TOptional<FOSEFillPaint> GetArrowheadFillStyle() const
   {
      if (ArrowheadStyle == EOSEArrowheadStyle::SolidColor)
      {
         return FOSEFillPaint(ArrowheadColor);
      }
      if (ArrowheadStyle == EOSEArrowheadStyle::Brush)
      {
         return FOSEFillPaint(ArrowheadBrush, ArrowheadBrushStretchUVsToFit);
      }
      return NullOpt;
   }
};

/// Defines a line as an origin, angle, and radius start and end (eg. bicycle spokes)
USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialLineShape
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Line Shape")
   FVector2D Origin = FVector2D::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Line Shape")
   FOSELineParams Line{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Line Shape", meta = (ClampMin = "0", UIMin = "0", ClampMax = "360", UIMax = "360", ForceUnits="degrees"))
   float AngleDegrees = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Line Shape")
   FFloatInterval Radius{ 100.0f, 400.0f };
};

/// Defines a crosshair that can be drawn in a UMG or slate widget
USTRUCT(BlueprintType)
struct OSECORE_API FOSECrosshairShape
{
   GENERATED_BODY()

   /// Number of lines in the crosshair
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape", Meta = (UIMin = 1, ClampMin = 1, UIMax = 16))
   int32 Spokes = 4;

   /// How long each spoke should be
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape", Meta = (UIMin = 0, ClampMin = 0))
   float Length = 15.0f;

   /// Spoke color and thickness
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape")
   FOSELineParams Style = FOSELineParams{ 1.0f, FLinearColor::White };

   /// Rotation for all spokes in this crosshair
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape", Meta = (UIMin = 0, UIMax = 360, ForceUnits="degrees"))
   float RotationDegrees = 0.0f;

   /// Spacing between the reticle and the start of the line.
   /// This is either from the center point of the reticle, or from the outer edge of the reticle if CenterOffsetIsRelativeToReticleRadius is enabled.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape")
   float CenterOffset = 0.0f;

   /// If enabled, adds the reticle's radius to the center offset
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape")
   bool CenterOffsetIsRelativeToReticleRadius = false;

   /// Translation for all spokes from the origin of the reticle
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crosshair Shape", AdvancedDisplay)
   FVector2D OriginOffset = FVector2D::Zero();

   /// Used by widgets to calculate desired size changes when AffectsWidgetDesiredSize is true
   FFloatInterval GetCrosshairRadius(float reticleRadius = 0.0f) const
   {
      FFloatInterval result{ CenterOffset, CenterOffset + Length };
      if (CenterOffsetIsRelativeToReticleRadius)
      {
         result += reticleRadius;
      }
      return result;
   }
};

/// How a dynamic reticle progress indicator expands
UENUM(BlueprintType)
enum class EOSERadialProgressBarMode : uint8
{
   Forward = 0,
   Backward,
   MiddleOut,
};

/// Defines a radial progress bar that can be drawn in a UMG or slate widget
USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialProgressBarShape
{
   GENERATED_BODY()

   /// What direction should the bar expand when the progress value is increasing?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape")
   EOSERadialProgressBarMode ProgressMode = EOSERadialProgressBarMode::Forward;

   /// The starting/base angle of the progress indicator in degrees.
   /// For EOSERadialProgressBarMode::MiddleOut, this is the center point. For forward and backward modes, this is the starting point.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape", Meta = (ClampMin = "0", UIMin = "0", ClampMax = "360", UIMax = "360", ForceUnits="degrees"))
   float ArcAngleDegrees = 0.0f;

   /// The "width" of the progress indicator in degrees
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape", Meta = (ClampMin = "0", UIMin = "0", ClampMax = "360", UIMax = "360", ForceUnits="degrees"))
   float ArcSizeDegrees = 360.0f;

   /// Radius of the progress indicator.
   /// This is either an absolute value, or relative to the radius of the reticle if RelativeToReticleRadius is enabled.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape")
   float Radius = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape", Meta = (UIMin = 0, ClampMin = 0))
   float Thickness = 32.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Progress Bar Shape")
   FOSERadialFillImage FillImage;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Progress Bar Shape")
   EOSERadialDiscUVMode UVMode = EOSERadialDiscUVMode::ProjectFullRadial;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape|Borders")
   FOSELineParams InnerBorder{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape|Borders")
   FOSELineParams OuterBorder{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape|Borders")
   FOSELineParams IndicatorStartCap;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape|Borders")
   FOSELineParams IndicatorEndCap;

   /// If enabled, borders are drawn to fit the current progress level.
   /// Otherwise, borders are always drawn at the maximum size and only the fill color/image is used to show progress.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape|Borders")
   bool FitBordersToProgressLevel = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape", AdvancedDisplay, Meta = (UIMin = 0, ClampMin = 0, UIMax = 360))
   int32 Resolution = 0;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape", AdvancedDisplay)
   bool FillImagePremultipliedAlpha = false;

   FFloatInterval GetRadius(float radiusOffset = 0.0f) const
   {
      FFloatInterval result{
         Radius + radiusOffset,
         Radius + Thickness + radiusOffset,
      };
      if (InnerBorder)
      {
         result += InnerBorder.Thickness;
      }
      if (OuterBorder)
      {
         result += OuterBorder.Thickness;
      }
      return result;
   }
};

/// Defines a line in the shape of an arc, which which can be a circle or section of a circle
/// This is similar to FOSERadialDiscShape but is not filled
USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialArcShape
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Circle Shape")
   FVector2D Origin = FVector2D::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Circle Shape")
   FOSELineParams Line{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Circle Shape", meta = (ClampMin = "0", UIMin = "0"))
   float Radius = 100.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Circle Shape", meta = (UIMin = "0", UIMax = "360", ForceUnits="degrees"))
   FFloatInterval ArcAngleDegrees{ 0.0f, 360.0f };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay, Category = "Radial Circle Shape", meta = (ClampMin = "0", UIMin = "0", UIMax = "360"))
   int32 Resolution = 0;

   FORCEINLINE int32 GetResolution() const
   {
      return OSERadialHelpers::CalcArcResolution(Resolution, ArcAngleDegrees, Radius);
   }

   FORCEINLINE bool IsVisible() const
   {
      return Radius > 0 && Line.IsVisible();
   }
};

/// Defines the a radial "disc" shape, which is a filled circle or slice of a circle (eg. the shape of a slice of pizza)
USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialDiscShape
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Disc Shape")
   FVector2D Origin = FVector2D::ZeroVector;

   /// Inner and outer radius of the disc. If the min value is greater than zero, renders as a disc with a hole cut out of it's center.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Disc Shape", meta = (ClampMin = "0", UIMin = "0"))
   FFloatInterval Radius{ 100.0f, 400.0f };

   /// Arc range of the disc, in degrees. A value of 0..360 renders as a circle, and a value of 0..180 renders as a half-circle.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Disc Shape", meta = (UIMin = "0", UIMax = "360", ForceUnits="degrees"))
   FFloatInterval ArcRangeDegrees{ 0.0f, 360.0f };

   /// Brush to render the disc with.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Disc Shape")
   FSlateBrush Brush;

   /// How to generate UVs for the disc
   UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay, Category = "Radial Disc Shape")
   EOSERadialDiscUVMode UVMode = EOSERadialDiscUVMode::Curved;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay, Category = "Radial Disc Shape")
   bool PremultipliedAlpha = false;

   /// Number of vertices to use around the circle. If zero, this will be automatically computed based on the size and arc range of the disc.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay, Category = "Radial Disc Shape", meta = (ClampMin = "0", UIMin = "0", UIMax = "360"))
   int32 Resolution = 0;

   FOSERadialDiscShape() = default;
   FOSERadialDiscShape(const FFloatInterval& radius, const FFloatInterval& arcRangeDegrees)
      : Radius(radius)
      , ArcRangeDegrees(arcRangeDegrees)
   {
   }

   FORCEINLINE int32 GetResolution() const
   {
      return OSERadialHelpers::CalcArcResolution(Resolution, ArcRangeDegrees, Radius.Max);
   }

   FORCEINLINE bool IsCircle() const
   {
      return ArcRangeDegrees.Size() >= 360.0f;
   }

   FORCEINLINE bool IsVisible() const
   {
      return Radius.Max > 0 && FMath::Max(0.0f, ArcRangeDegrees.Max - ArcRangeDegrees.Min) > 0;
   }
};

/// Defines a radial slice, which is similar to FOSERadialDiscShape but includes borders and functionality intended for interactive use-cases
USTRUCT(BlueprintType)
struct OSECORE_API FOSERadialSlice
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   FOSERadialDiscShape Background{ FFloatInterval(100.0f, 400.0f), FFloatInterval(0.0f, 90.0f) };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   bool IsDisabled = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   bool IsHidden = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   FOSELineParams BorderSide{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (ClampMin = "-180", UIMin = "-20", ClampMax = "180", UIMax = "20", ForceUnits="degrees"))
   float BorderSideOffsetDegrees = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   FOSELineParams BorderInner;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   float BorderInnerOffsetRadius = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   FOSELineParams BorderOuter;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   float BorderOuterOffsetRadius = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBackgroundRadius = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBackgroundRadius"))
   FFloatInterval HoverBackgroundRadius{ 100.0f, 400.0f };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBackgroundBrush = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBackgroundBrush"))
   FSlateBrush HoverBackgroundBrush;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBorderSide = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBorderSide"))
   FOSELineParams HoverBorderSide{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBorderSideOffsetDegrees = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBorderSideOffsetDegrees", ClampMin = "-180", UIMin = "-20", ClampMax = "180", UIMax = "20", ForceUnits="degrees"))
   float HoverBorderSideOffsetDegrees = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBorderInner = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBorderInner"))
   FOSELineParams HoverBorderInner{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBorderInnerOffsetRadius = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBorderInnerOffsetRadius"))
   float HoverBorderInnerOffsetRadius = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBorderOuter = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBorderOuter"))
   FOSELineParams HoverBorderOuter{ 1.0f, FLinearColor::Gray };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (InlineEditConditionToggle))
   bool UseHoverBorderOuterOffsetRadius = false;
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (EditCondition = "UseHoverBorderOuterOffsetRadius"))
   float HoverBorderOuterOffsetRadius = 0.0f;

   /// The color blend mode to use when interpolating border colors on hover
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice")
   EOSEColorBlendMode HoverColorBlendMode = EOSEColorBlendMode::Linear;

   /// Lerp alpha value used when an item is blending between hover and non-hover states
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Slice", meta = (UIMin = "0.0", UIMax = "1.0"))
   float HoverLerpAlpha = 0.0f;

   // These helper functions take hover into account

   FFloatInterval GetBackgroundRadius() const { return UseHoverBackgroundRadius ? OSERadialHelpers::LerpFloatInterval(Background.Radius, HoverBackgroundRadius, HoverLerpAlpha) : Background.Radius; }

   FOSELineParams GetBorderSide() const { return UseHoverBorderSide ? FOSELineParams::Lerp(BorderSide, HoverBorderSide, HoverLerpAlpha, HoverColorBlendMode) : BorderSide; }
   float GetBorderSideOffsetDegrees() const { return UseHoverBorderSideOffsetDegrees ? FMath::Lerp(BorderSideOffsetDegrees, HoverBorderSideOffsetDegrees, HoverLerpAlpha) : BorderSideOffsetDegrees; }

   FOSELineParams GetBorderInner() const { return UseHoverBorderInner ? FOSELineParams::Lerp(BorderInner, HoverBorderInner, HoverLerpAlpha, HoverColorBlendMode) : BorderInner; }
   float GetBorderInnerOffsetRadius() const { return UseHoverBorderInnerOffsetRadius ? FMath::Lerp(BorderInnerOffsetRadius, HoverBorderInnerOffsetRadius, HoverLerpAlpha) : BorderInnerOffsetRadius; }

   FOSELineParams GetBorderOuter() const { return UseHoverBorderOuter ? FOSELineParams::Lerp(BorderOuter, HoverBorderOuter, HoverLerpAlpha, HoverColorBlendMode) : BorderOuter; }
   float GetBorderOuterOffsetRadius() const { return UseHoverBorderOuterOffsetRadius ? FMath::Lerp(BorderOuterOffsetRadius, HoverBorderOuterOffsetRadius, HoverLerpAlpha) : BorderOuterOffsetRadius; }

   /// Debugging helper function for getting the (rough) bounding box of the slice.
   /// This is not intended to be the exact bounding box.
   FBox2f GetBoundingBox() const;
};


/// Allows temporarily converting a FOSERadialPaintContext to an FPaintContext (which serves essentially the same purpose but is exposed to blueprints)
///
/// IMPORTANT: Do *not* do any painting with the FOSERadialPaintContext while this UMG context is on the stack.
class OSECORE_API FOSEScopedUMGPaintContext
{
   FOSERadialPaintContext& _radialContext;
   FPaintContext _umgContext;

public:
   explicit FOSEScopedUMGPaintContext(FOSERadialPaintContext& radialContext);
   ~FOSEScopedUMGPaintContext();

   UE_NONCOPYABLE(FOSEScopedUMGPaintContext);

   FPaintContext& operator*() { return _umgContext; }
   FPaintContext& Get() { return _umgContext; }
};


///
/// Radial paint context, for drawing radial shapes in SWidget::OnPaint, UUserWidget::NativePaint (native-only) or UUserWidget::OnPaint (blueprints-only).
/// Note that when implementing UUserWidget::NativePaint or SWidget::OnPaint, the function must return the topmost layer id after drawing is complete.
/// The function has the current topmost layer id as a parameter, so you generally want to structure your paint function like so:
///
/// int32 UMyWidget::NativePaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const
/// {
///   layerId = Super::NativePaint(args, allottedGeometry, cullingRect, outDrawElements, layerId, widgetStyle, parentEnabled);
///   // Note that the FOSERadialPaintContext constructor takes a _reference_ to the layerId variable, so it will modify it in-place when needed and you can simply return it when done.
///   FOSERadialPaintContext paintCtx(layerId, allottedGeometry, cullingRect, outDrawElements, widgetStyle, parentEnabled);
///   // ... draw stuff with paintCtx ...
///   return layerId;
/// }
///
class OSECORE_API FOSERadialPaintContext
{
   friend class FOSEScopedUMGPaintContext;

   int32& _maxLayer;
   const FGeometry& _allottedGeometry;
   const FSlateRect& _cullingRect;
   FSlateWindowElementList& _outDrawElements;
   const FWidgetStyle& _widgetStyle;
   bool _parentEnabled = false;
   float _renderOpacity = 1.0f;
   TArray<FLinearColor, TInlineAllocator<4>> _globalTintColorStack;

   FLinearColor _ApplyGlobalColorEffects(const FLinearColor& color) const;
   FColor _ApplyGlobalColorEffects(FColor color) const;

public:
   /// Constructor intended for use by UserWidget NativePaint callbacks
   FOSERadialPaintContext(int32& maxLayer, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, const FWidgetStyle& widgetStyle, bool parentEnabled, float renderOpacity = 1.0f);

   /// Constructor intended for use by UMG OnPaint callbacks
   explicit FOSERadialPaintContext(FPaintContext& umgOnPaintContext);

private:
   /// Creates a new UMG paint context that can be passed to blueprints.
   /// Note that if you use this, the FPaintContext will increment its own layer id on draw calls, and this FOSERadialPaintContext will not be aware of it.
   /// You will need to have your paint function return the FPaintContext's MaxLayer field instead of relying on this FOSERadialPaintContext to increment it
   /// for you.
   ///
   /// DO NOT interleave FOSERadialPaintContext draw calls with FPaintContext draw calls! Use one or the other (or one followed by the other).
   ///
   /// If this ends up being a source of bugs in the future, it might be a good idea to add a bool to FOSERadialPaintContext indicating that a
   /// UMG FPaintContext is active. That would allow us to add asserts to drawing functions (specifically, anywhere that _maxLayer is incremented).
   ///
   /// (This method is intended to be used from FOSEScopedUMGPaintContext)
   FPaintContext ToUMGPaintContext();

   /// Updates our max layer id to match the max layer painted by a UMG FPaintContext.
   /// This is useful after using ToUMGPaintContext() to pass the context to a blueprint callback.
   /// (This method is intended to be used from FOSEScopedUMGPaintContext)
   void SetLayerIdFromUMGPaintContext(const FPaintContext& umgPaintContext);

public:
   FORCEINLINE int32 GetMaxLayerId() const { return _maxLayer; }

   FORCEINLINE const FGeometry& GetAllottedGeometry() const { return _allottedGeometry; }
   FORCEINLINE FVector2f GetLocalSize() const { return _allottedGeometry.GetLocalSize(); }

   FORCEINLINE void SetRenderOpacity(float newRenderOpacity) { _renderOpacity = FMath::Clamp(newRenderOpacity, 0.0f, 1.0f); }

   /// Sets a tint color that will be applied to all subsequent draw calls
   FORCEINLINE void PushGlobalTintColor(const FLinearColor& tintColor) { _globalTintColorStack.Add(tintColor); }

   /// Unsets a global tint color that was assigned via PushGlobalTintColor
   FORCEINLINE void PopGlobalTintColor() { check(_globalTintColorStack.Num() > 0); _globalTintColorStack.RemoveAt(_globalTintColorStack.Num() - 1); }

   /// Draws a line with a variable number of points. Each additional point will extend the line to that position.
   /// Note that if you want a closed shape you must include the first point as the last point in the array (eg. drawing a square requires 5 points).
   /// NB. You *must* pass in at least two points
   void DrawLines(TArray<FVector2f> points, const FOSELineParams& params, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws a line between two points
   void DrawLine(const FVector2f& a, const FVector2f& b, const FOSELineParams& params, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None)
   {
      DrawLines({ a, b }, params, antialias, drawEffect);
   }

   /// Draw a line defined by a start and end radius around a center point and angle
   void DrawLineFromOriginAndAngle(const FVector2f& origin, float angleDegrees, const FFloatInterval& radius, const FOSELineParams& params, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draw a line defined by a start and end radius around a center point and angle
   void DrawLineFromOriginAndAngle(const FOSERadialLineShape& line, const FVector2f& originOffset = FVector2f::ZeroVector, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None)
   {
      DrawLineFromOriginAndAngle(FVector2f(line.Origin) + originOffset, line.AngleDegrees, line.Radius, line.Line, antialias, drawEffect);
   }

   /// Draws just the part of a line segment that is overlapping with a culling rect (draws nothing at all if no part of the line is intersecting)
   /// Mostly useful for use in a scrollbox, where you don't want to paint areas outside the viewable area.
   void DrawLineWithCullingRect(const FVector2f& lineA, const FVector2f& lineB, const FOSELineParams& lineParams, const FSlateRect& cullingRect, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws a number of lines radially around a center point
   void DrawCrosshair(const FVector2f& origin, int32 numSpokes, const FOSELineParams& style, const FFloatInterval& radius, float rotationDegrees = 0.0f, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws a number of lines radially around a center point
   void DrawCrosshair(const FOSECrosshairShape& crosshairShape, const FVector2f& origin, float reticleRadius = 0.0f, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws a curved progress bar
   void DrawRadialProgressBar(const FOSERadialProgressBarShape& progressBar, float normalizedValue, const FVector2f& origin, float radiusOffset = 0.0f, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws an arrow from lineA pointing towards lineB
   /// If cullingRect is specified, only the part of the arrow inside the culling rect is drawn (mostly useful for use in a scrollbox to avoid painting outside the viewable area).
   void DrawArrow(const FVector2f& lineA, const FVector2f& lineB, const FOSELineParams& lineParams, float arrowheadSize, const TOptional<FOSEFillPaint>& arrowheadFill = NullOpt,
      float arrowheadAngle = 20.0f, const TOptional<FOSELineParams>& arrowheadBorder = NullOpt, const TOptional<FSlateRect>& cullingRect = NullOpt, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws an arrow from lineA pointing towards lineB (version taking an FOSEArrowStyle to simplify exposing style parameters in blueprints)
   /// If cullingRect is specified, only the part of the arrow inside the culling rect is drawn (mostly useful for use in a scrollbox to avoid painting outside the viewable area).
   void DrawArrow(const FVector2f& lineA, const FVector2f& lineB, const FOSEArrowStyle& style, const TOptional<FSlateRect>& cullingRect = NullOpt, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws text (mostly intended for debugging purposes)
   void DrawText(FVector2f textPosition, const FString& text, int32 fontSize = 14, FName fontName = NAME_None, const FLinearColor& color = FLinearColor::White, bool centered = true, bool dropShadow = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws a bounding box as an origin (top-left) and size (primarily intended for debugging)
   void DrawBoundingBox(const FVector2f& origin, const FVector2f& size, const FOSELineParams& line = FOSELineParams{ 1.0f, FLinearColor::White }, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draw a filled triangle.
   /// Passing in explicit UVs is optional. If you do pass them in, the array must have exactly three elements.
   /// If you don't specify UVs, they will be generated automatically by projecting a rect onto the triangle.
   void DrawTriangle(const FVector2f& a, const FVector2f& b, const FVector2f& c, const FOSEFillPaint& fillStyle, float rotateUVsAngleDeg = 0.0f, TArrayView<FVector2f> uvs = {}, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draw a filled rect
   void DrawRect(const FVector2f& origin, const FVector2f& size, const FSlateBrush& brush, const TOptional<FColor>& tint = NullOpt, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draw a segment of a circle
   void DrawArc(const FVector2f& origin, float radius, FFloatInterval arcAngleDegrees, const FOSELineParams& params, int32 resolution = 0, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draw a segment of a circle
   void DrawArc(const FOSERadialArcShape& circle, const FVector2f& originOffset = FVector2f::ZeroVector, ESlateDrawEffect drawEffect = ESlateDrawEffect::None)
   {
      constexpr bool antialias = true;
      DrawArc(FVector2f(circle.Origin) + originOffset, circle.Radius, circle.ArcAngleDegrees, circle.Line, circle.Resolution, antialias, drawEffect);
   }

   /// Draw a full circle
   void DrawCircle(const FVector2f& origin, float radius, const FOSELineParams& params, int32 resolution = 0, bool antialias = true, ESlateDrawEffect drawEffect = ESlateDrawEffect::None)
   {
      if (resolution > 0)
      {
         // a resolution of zero means automatically compute a reasonable resolution, but for a circle, a resolution of 1 or 2 makes no sense
         resolution = FMath::Max(3, resolution);
      }
      DrawArc(origin, radius, { 0.0f, 360.0f }, params, resolution, antialias, drawEffect);
   }

   /// Draws a filled circle or segment of a circle
   /// If a valid uvBounds is passed in, the disc's UV map will be projected onto it. If null, it will be curved around the disc.
   void DrawDisc(
      const FVector2f& origin,
      const FFloatInterval& radius,
      const FFloatInterval& arcAngleDegrees,
      const FSlateBrush& brush,
      int32 resolution = 0,
      const TOptional<FColor>& tint = NullOpt,
      EOSERadialDiscUVMode uvMode = EOSERadialDiscUVMode::Curved,
      ESlateDrawEffect drawEffect = ESlateDrawEffect::None);

   /// Draws a filled circle or segment of a circle
   void DrawDisc(const FOSERadialDiscShape& disc, const FVector2f& originOffset, const TOptional<FColor>& tint = NullOpt, ESlateDrawEffect drawEffect = ESlateDrawEffect::None)
   {
      if (disc.PremultipliedAlpha)
      {
         drawEffect |= ESlateDrawEffect::PreMultipliedAlpha;
      }
      DrawDisc(FVector2f(disc.Origin) + originOffset, disc.Radius, disc.ArcRangeDegrees, disc.Brush, disc.Resolution, tint, disc.UVMode, drawEffect);
   }

   /// Draws a radial slice, taking into account hover state
   void DrawSlice(const FOSERadialSlice& slice, const FVector2f& originOffset = FVector2f::ZeroVector, ESlateDrawEffect drawEffect = ESlateDrawEffect::None);
};

/// Blueprint-exposed helper functions for radial painting (and some general-purpose 2D math helpers)
UCLASS(BlueprintType)
class OSECORE_API UOSERadialPaintLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   using FIntersectionArray2f = TArray<FVector2f, TInlineAllocator<4>>;

   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static FORCEINLINE FVector2D FindPointOnCircle(FVector2D origin, float radius, float angleDegrees)
   {
      return OSERadialHelpers::PointOnCircleDegrees<FVector2D>(origin, radius, angleDegrees);
   }

   /// Ensures an angle in degrees is between +/- 180 degrees by unwinding
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static FORCEINLINE float UnwindAngleDegrees(float angleDegrees)
   {
      return FMath::UnwindDegrees(angleDegrees);
   }

   /// Linear interpolation of an arc in degrees, taking winding into account
   UFUNCTION(BlueprintPure, Category = "Math|OSE", DisplayName = "Lerp (Arc Angle in Degrees)")
   static FORCEINLINE float LerpArcAngleDegrees(FFloatInterval arcAngleDegrees, float alpha)
   {
      while (arcAngleDegrees.Min < 0 || arcAngleDegrees.Max < 0)
      {
         arcAngleDegrees.Min += 360.0f;
         arcAngleDegrees.Max += 360.0f;
      }
      return FMath::UnwindDegrees(FMath::Lerp(arcAngleDegrees.Min, arcAngleDegrees.Max, alpha));
   }

   /// Length of an arc around a circle (degrees)
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static FORCEINLINE float FindArcLengthDegrees(FFloatInterval arcAngleDegrees, float radius)
   {
      return 2.0f * UE_PI * radius * (FMath::Abs(arcAngleDegrees.Max - arcAngleDegrees.Min) / 360.0f);
   }

   /// Returns the delta angle in an arc (return value will be between -180.0 and +180.0)
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static FORCEINLINE float FindAngleDeltaDegrees(FFloatInterval arcAngleDegrees)
   {
      return FMath::FindDeltaAngleDegrees(arcAngleDegrees.Min, arcAngleDegrees.Max);
   }

   /// Given an arc (a segment of a circle as a start and end angle in degrees), checks if a given angle is inside that arc
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static bool AngleContainedInRadialArc(float angleDegrees, FFloatInterval arcAngleDegrees);

   /// Returns the angle in degrees between two 2D direction vectors
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static float FindAngleDegreesBetweenNormals2D(const FVector2D& from, const FVector2D& to);

   /// Finds the intersection between two line segments (if any)
   static bool FindLineSegmentIntersection(const FVector2f& a0, const FVector2f& a1, const FVector2f& b0, const FVector2f& b1, FVector2f& outIntersection);

   /// Returns the intersection between a 2d line segment and the edges of a 2d AABB (if any)
   static bool FindLineSegmentRectEdgeIntersections(const FVector2f& lineStart, const FVector2f& lineEnd, const FSlateRect& rect, FIntersectionArray2f& outIntersections);

   /// Interpolate between two colors with a selectable blend mode
   static FLinearColor ColorLerp(const FLinearColor& base, const FLinearColor& top, float alpha, EOSEColorBlendMode blendMode);

   /// Interpolate between two colors with a selectable blend mode
   UFUNCTION(BlueprintPure, Category = "Math|OSE", DisplayName = "Lerp (LinearColor, Custom Blend Mode)")
   static FORCEINLINE FLinearColor BP_ColorLerp(FLinearColor base, FLinearColor top, float alpha, EOSEColorBlendMode blendMode)
   {
      return ColorLerp(base, top, alpha, blendMode);
   }

   /// Simulates the photoshop "Overlay" color blending mode.
   /// Note that this only blends the R, G, and B channels - the resulting color's alpha channel will simply use resultAlphaChannel for its alpha component.
   UFUNCTION(BlueprintPure, Category = "Math|OSE")
   static FLinearColor ColorBlendOverlay(const FLinearColor& base, const FLinearColor& top, float resultAlphaChannel = 1.0f);

   /// Linearly interpolates from one color to another by using an intermediate "overlay" (as in the photoshop blend mode) blended color as the interpolation halfway point
   /// (Normally, an overlay blend will result in a mixed color, but by using it as the halfway point for interpolation we can get nice color blending
   /// results without the color shifting behavior that occurs when using FLinearColor::LerpUsingHSV)
   static FLinearColor LerpColorUsingOverlayBlend(const FLinearColor& base, const FLinearColor& top, float alpha, float midpoint = 0.75f);

   /// Linear interpolation for float intervals
   UFUNCTION(BlueprintPure, Category = "Math|OSE", DisplayName = "Lerp (Float Interval)")
   static FORCEINLINE FFloatInterval LerpFloatInterval(FFloatInterval a, FFloatInterval b, float alpha)
   {
      return OSERadialHelpers::LerpFloatInterval(a, b, alpha);
   }

   /// Gets the screen-space position of the paint context's top-left point
   UFUNCTION(BlueprintPure, Category = "Radial Paint Library")
   static FVector2D GetPaintContextScreenSpaceOrigin(UPARAM(ref) FPaintContext& context);

   /// Gets the size of a paint context in local space (intended to be used for painting coordinates)
   UFUNCTION(BlueprintPure, Category = "Radial Paint Library")
   static FVector2D GetPaintContextLocalSize(UPARAM(ref) FPaintContext& context);

   /// Converts a world-space coordinate to paint-space
   UFUNCTION(BlueprintPure, Category = "Radial Paint Library")
   static bool TransformWorldPositionToPaintSpace(APlayerController* playerController, UPARAM(ref) FPaintContext& context, FVector worldPosition, FVector2D& position);

   /// Converts a screen-space coordinate to paint-space. Useful for getting the paint-space coordinates of a world location.
   UFUNCTION(BlueprintPure, Category = "Radial Paint Library", Meta = (WorldContext = "contextObject"))
   static FVector2D TransformScreenSpaceToPaintSpace(UObject* contextObject, UPARAM(ref) FPaintContext& context, FVector2D screenSpaceCoord);

   /// Draws a line between two points
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Line")
   static void Radial_DrawLine(UPARAM(ref) FPaintContext& context, FVector2D a, FVector2D b, const FOSELineParams& params)
   {
      FOSERadialPaintContext(context).DrawLine(FVector2f(a), FVector2f(b), params);
   }

   /// Draw a line defined by a start and end radius around a center point and angle
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Line (From Origin and Angle)")
   static void Radial_DrawLineFromOriginAndAngle(UPARAM(ref) FPaintContext& context, FVector2D origin, float angleDegrees, const FFloatInterval& radius, const FOSELineParams& params)
   {
      FOSERadialPaintContext(context).DrawLineFromOriginAndAngle(FVector2f(origin), angleDegrees, radius, params);
   }

   /// Draws a line with a variable number of points. Each additional point will extend the line to that position.
   /// Note that if you want a closed shape you must include the first point as the last point in the array (eg. drawing a square requires 5 points).
   /// NB. You *must* pass in at least two points, otherwise nothing will be rendered.
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Line (Custom)")
   static void Radial_DrawLineCustom(UPARAM(ref) FPaintContext& context, const TArray<FVector2D>& points, FOSELineParams params);

   /// Draws a crosshair
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Crosshair")
   static void Radial_DrawCrosshair(UPARAM(ref) FPaintContext& context, FVector2D origin, int32 numSpokes, FOSELineParams style, FFloatInterval radius, float rotationDegrees)
   {
      FOSERadialPaintContext(context).DrawCrosshair(FVector2f(origin), numSpokes, style, radius, rotationDegrees);
   }

   /// Draws a crosshair shape
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Crosshair (Shape)")
   static void Radial_DrawCrosshairShape(UPARAM(ref) FPaintContext& context, FVector2D origin, float reticleRadius, const FOSECrosshairShape& crosshairShape)
   {
      FOSERadialPaintContext(context).DrawCrosshair(crosshairShape, FVector2f(origin), reticleRadius);
   }

   /// Draws a curved progress bar shape
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Radial Progress Bar (Shape)")
   static void Radial_DrawRadialProgressBar(UPARAM(ref) FPaintContext& context, const FOSERadialProgressBarShape& progressBar, float normalizedValue, FVector2D origin, float radiusOffset = 0.0f)
   {
      FOSERadialPaintContext(context).DrawRadialProgressBar(progressBar, normalizedValue, FVector2f(origin), radiusOffset);
   }

   /// Draws text
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Debug Text")
   static void Radial_DrawText(UPARAM(ref) FPaintContext& context, FVector2D textPosition, FString text, int32 fontSize = 14, FName fontName = NAME_None, FLinearColor color = FLinearColor::White, bool centered = true, bool dropShadow = true)
   {
      FOSERadialPaintContext(context).DrawText(FVector2f(textPosition), text, fontSize, fontName, color, centered, dropShadow);
   }

   /// Draws the outline of a rectangle as a center point and size
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Rect (Outline)")
   static void Radial_DrawRectOutline(UPARAM(ref) FPaintContext& context, FVector2D center, FVector2D size, FOSELineParams line)
   {
      FOSERadialPaintContext(context).DrawBoundingBox(FVector2f(center - (size * 0.5f)), FVector2f(size), line);
   }

   /// Draws a filled rectangle as a center point and size
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Rect (Filled)")
   static void Radial_DrawRectFilled(UPARAM(ref) FPaintContext& context, FVector2D center, FVector2D size, const FSlateBrush& brush, bool applyTint = false, FLinearColor tintColor = FLinearColor::White)
   {
      const TOptional<FColor> tint = applyTint ? TOptional(tintColor.ToFColorSRGB()) : NullOpt;
      FOSERadialPaintContext(context).DrawRect(FVector2f(center - (size * 0.5f)), FVector2f(size), brush, tint);
   }

   /// Draw a segment of a circle
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Arc Line")
   static void Radial_DrawArc(UPARAM(ref) FPaintContext& context, FVector2D origin, float radius, FFloatInterval arcAngleDegrees, FOSELineParams line, int32 resolution = 0)
   {
      FOSERadialPaintContext(context).DrawArc(FVector2f(origin), radius, arcAngleDegrees, line, resolution);
   }

   /// Draw a segment of a circle
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Arc Line (Shape)")
   static void Radial_DrawArcShape(UPARAM(ref) FPaintContext& context, const FOSERadialArcShape& circle, FVector2D originOffset = FVector2D::ZeroVector)
   {
      FOSERadialPaintContext(context).DrawArc(circle, FVector2f(originOffset));
   }

   /// Draw a circle
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Circle")
   static void Radial_DrawCircle(UPARAM(ref) FPaintContext& context, FVector2D origin, float radius, FOSELineParams line, int32 resolution = 0)
   {
      FOSERadialPaintContext(context).DrawCircle(FVector2f(origin), radius, line, resolution);
   }

   /// Draws a filled circle or segment of a circle
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Disc")
   static void Radial_DrawDisc(
      UPARAM(ref) FPaintContext& context,
      FVector2D origin,
      FFloatInterval radius,
      FFloatInterval arcAngleDegrees,
      const FSlateBrush& brush,
      int32 resolution = 0,
      bool applyTint = false,
      FLinearColor tintColor = FLinearColor::White,
      EOSERadialDiscUVMode uvMode = EOSERadialDiscUVMode::Curved,
      bool drawDisabled = false);

   /// Draws a filled circle or segment of a circle
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Disc (Shape)")
   static void Radial_DrawDiscShape(UPARAM(ref) FPaintContext& context, const FOSERadialDiscShape& disc, FVector2D originOffset, bool applyTint = false, FLinearColor tintColor = FLinearColor::White)
   {
      if (!disc.IsVisible())
      {
         return;
      }
      const TOptional<FColor> tint = applyTint ? TOptional(tintColor.ToFColorSRGB()) : NullOpt;
      FOSERadialPaintContext(context).DrawDisc(disc, FVector2f(originOffset), tint);
   }

   /// Draws a radial slice, taking into account hover state
   UFUNCTION(BlueprintCallable, Category = "Radial Paint Library", DisplayName = "Radial: Draw Slice")
   static void Radial_DrawSlice(UPARAM(ref) FPaintContext& context, const FOSERadialSlice& slice, FVector2D originOffset = FVector2D::ZeroVector)
   {
      FOSERadialPaintContext(context).DrawSlice(slice, FVector2f(originOffset));
   }
};
