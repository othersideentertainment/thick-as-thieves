// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Components/ContentWidget.h"

// ose
#include "UI/OSERadialPaintLibrary.h"

#include "OSEDynamicReticleWidget.generated.h"

class SOSERadialBackground;


/// Defines a radial progress bar that can be drawn in a UMG or slate widget
USTRUCT(BlueprintType)
struct OSECORE_API FOSEDynamicReticleProgressBar
{
   GENERATED_BODY()

   /// Allows native classes to store the current value inside the struct.
   /// Note that the absence of a value indicates that the whole progress bar should not be drawn.
   TOptional<float> CurrentValue;

   /// Value used when previewing the widget in the UMG editor
   UPROPERTY(EditAnywhere, Category = "Dynamic Reticle Progress Bar", Meta = (UIMin = 0, ClampMin = 0, UIMax = 1, ClampMax = 1))
   float PreviewPercent = 1.0f;

   /// The range of values we can assign to this indicator.
   /// The meaning of these numbers is user-defined (you could use them as a normalized 0..1 range, or as a concrete value range, like a stamina value of 0..50)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dynamic Reticle Progress Bar")
   FFloatInterval ValueRange{ 0.0f, 1.0f };

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dynamic Reticle Progress Bar", Meta = (ShowOnlyInnerProperties))
   FOSERadialProgressBarShape ProgressBar;

   /// Indicates that the starting radius should be offset from the reticle radius.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial Progress Bar Shape")
   bool RadiusIsRelativeToReticle = true;

   /// When used in a widget, should this progress bar affect the desired size of that widget?
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Dynamic Reticle Progress Bar", AdvancedDisplay)
   bool AffectsWidgetDesiredSize = true;

   float GetValue() const { return CurrentValue.Get(ValueRange.Min); }

   void Paint(FOSERadialPaintContext& ctx, const FVector2f& reticleOrigin, float baseReticleRadius, TOptional<float> valueOverride = NullOpt,
      bool isDesignView = false, TOptional<FName> indicatorName = NullOpt, ESlateDrawEffect drawEffect = ESlateDrawEffect::None) const;
};

UENUM(BlueprintType)
enum class EOSEDynamicReticleDiscBorderMode : uint8
{
   NoBorder,
   UseDiscBorderColor,
   UseDiscTintColor,
   UseDiscImageTintColor,
};

/// Dynamic, customizable reticle
UCLASS(BlueprintType, Blueprintable, meta = (DisableNativeTick))
class OSECORE_API UOSEDynamicReticleWidget : public UContentWidget
{
   GENERATED_BODY()

public:
   UOSEDynamicReticleWidget();

   // From UWidget
   virtual TSharedRef<SWidget> RebuildWidget() override;
   virtual void SynchronizeProperties() override;
   virtual void RemoveFromParent() override;
#if WITH_EDITOR
   virtual const FText GetPaletteCategory() override;
#endif

   // From UVisual
   virtual void ReleaseSlateResources(bool releaseChildren) override;

protected:
   // UPanelWidget
   virtual UClass* GetSlotClass() const override;
   virtual void OnSlotAdded(UPanelSlot* Slot) override;
   virtual void OnSlotRemoved(UPanelSlot* Slot) override;

protected:
   virtual void _OnRadialPaint(FOSERadialPaintContext& paintCtx, const FVector2f& radialOrigin, const FFloatInterval& radius);
   virtual bool _Tick(float deltaSeconds);

public:
   /// Child slot horizontal alignment
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Child Layout")
   TEnumAsByte<EHorizontalAlignment> HorizontalAlign = HAlign_Fill;

   /// Child slot vertical alignment
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Child Layout")
   TEnumAsByte<EVerticalAlignment> VerticalAlign = VAlign_Fill;

   /// Primary reticle radius. Elements of the reticle can be positioned relative to this value.
   UPROPERTY(EditAnywhere, BlueprintGetter = "GetReticleRadius", BlueprintSetter = "SetReticleRadius", Category = "Dynamic Reticle", Meta = (UIMin = 0, ClampMin = 0))
   float ReticleRadius = 32.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetReticleTintColor", Category = "Dynamic Reticle")
   FLinearColor ReticleTintColor = FLinearColor::White;

   // ====== Arc Shape ======

   /// Draw a circle/arc in the center of the reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Arc")
   bool ShowArc = true;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetArcRadius", Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc"))
   float ArcRadius = 10.0f;

   /// If enabled, adds the reticle's radius to the arc radius
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc"))
   bool ArcRadiusIsRelativeToReticleRadius = true;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetArcColor", Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc"))
   FLinearColor ArcColor = FLinearColor::White;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetArcThickness", Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc"))
   float ArcThickness = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetArcRangeStartDegrees", Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc", UIMin = "0", UIMax = "360", ForceUnits="degrees"))
   float ArcRangeStartDegrees = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetArcRangeEndDegrees", Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc", UIMin = "0", UIMax = "360", ForceUnits="degrees"))
   float ArcRangeEndDegrees = 360.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetArcOriginOffset", Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc"))
   FVector2D ArcOriginOffset = FVector2D::Zero();

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Arc", Meta = (EditCondition = "ShowArc", ClampMin = "0", UIMin = "0", UIMax = "360"))
   int32 ArcResolution = 0;

   // ====== Crosshair Shape ======

   /// Draws a crosshair in the center of the reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Crosshair")
   bool ShowCrosshair = true;

   /// Number of lines in the crosshair
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair", UIMin = 1, ClampMin = 1, UIMax = 16))
   int32 CrosshairSpokes = 4;

   /// Spacing between the reticle and the start of the line.
   /// This is either from the center point of the reticle, or from the outer edge of the reticle if CrosshairCenterSpacingIsRelativeToReticleRadius is enabled.
   UPROPERTY(EditAnywhere, BlueprintSetter = "SetCrosshairCenterSpacing", Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair"))
   float CrosshairCenterSpacing = 0.0f;

   /// If enabled, adds the reticle's radius to the center offset
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair"))
   bool CrosshairCenterSpacingIsRelativeToReticleRadius = true;

   /// How long each spoke should be
   UPROPERTY(EditAnywhere, BlueprintSetter = "SetCrosshairLength", Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair", UIMin = 0, ClampMin = 0))
   float CrosshairLength = 20.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetCrosshairThickness", Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair"))
   float CrosshairThickness = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetCrosshairColor", Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair"))
   FLinearColor CrosshairColor = FLinearColor::White;

   /// Rotation for all spokes in this crosshair
   UPROPERTY(EditAnywhere, BlueprintSetter = "SetCrosshairRotationDegrees", Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair", UIMin = 0, UIMax = 360, ForceUnits="degrees"))
   float CrosshairRotationDegrees = 0.0f;

   /// Translation for all spokes from the origin of the reticle
   UPROPERTY(EditAnywhere, BlueprintSetter = "SetCrosshairOriginOffset", Category = "Dynamic Reticle|Crosshair", Meta = (EditCondition = "ShowCrosshair"))
   FVector2D CrosshairOriginOffset = FVector2D::Zero();

   // ====== Disc Shape ======

   /// Draws a filled disc shape in the center of the reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc")
   bool ShowDisc = true;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscRadius", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   float DiscRadius = 12.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   bool DiscRadiusIsRelativeToReticleRadius = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (InlineEditConditionToggle))
   bool IsOutline = false;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscOutlineThickness", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "IsOutline", ClampMin = "0", UIMin = "0"))
   float DiscOutlineThickness = 6.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscTintColor", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   FLinearColor DiscTintColor = FLinearColor::White;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   FOSERadialFillImage DiscFillImage;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   EOSERadialDiscUVMode DiscFillImageUVMode = EOSERadialDiscUVMode::Curved;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   EOSEDynamicReticleDiscBorderMode DiscBorderMode = EOSEDynamicReticleDiscBorderMode::UseDiscBorderColor;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscBorderColor", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc && DiscBorderMode == EOSEDynamicReticleDiscBorderMode::UseDiscBorderColor"))
   FLinearColor DiscBorderColor = FLinearColor::White;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscBorderThickness", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc && DiscBorderMode != EOSEDynamicReticleDiscBorderMode::NoBorder"))
   float DiscBorderThickness = 1.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscArcRangeStartDegrees", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc", UIMin = "0", UIMax = "360", ForceUnits="degrees"))
   float DiscArcRangeStartDegrees = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetDiscArcRangeEndDegrees", Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc", UIMin = "0", UIMax = "360", ForceUnits="degrees"))
   float DiscArcRangeEndDegrees = 360.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc", ClampMin = "0", UIMin = "0", UIMax = "360"))
   int32 DiscResolution = 0;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Disc", Meta = (EditCondition = "ShowDisc"))
   bool DiscFillImagePremultipliedAlpha = false;

   // ====== Progress Indicators ======

   UPROPERTY(EditAnywhere, Category = "Dynamic Reticle|Progress Indicators")
   bool ShowProgressIndicatorNamesInDesignView = false;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle|Progress Indicators")
   TMap<FName, FOSEDynamicReticleProgressBar> RadialProgressIndicators;

   // ====== Advanced Section ======

   UPROPERTY(EditAnywhere, BlueprintSetter = "SetReticleOriginOffset", Category = "Dynamic Reticle", AdvancedDisplay)
   FVector2D ReticleOriginOffset;

   /// Shrinks the reticle if the widget's desired size is too small to fit
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dynamic Reticle", AdvancedDisplay)
   bool ClampReticleRadiusToWidgetSize = false;

   // ====== Decorative Shapes ======

   /// Arc shapes that are always drawn for this reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes")
   TArray<FOSERadialArcShape> ArcShapes;

   /// Crosshair shapes that are always drawn for this reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes")
   TArray<FOSECrosshairShape> CrosshairShapes;

   /// Disc shapes that are always drawn for this reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes")
   TArray<FOSERadialDiscShape> DiscShapes;

   /// Line shapes that are always drawn for this reticle
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes")
   TArray<FOSERadialLineShape> LineShapes;

   /// Take decorative arc shapes into account when computing the widget's desired size
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes", AdvancedDisplay)
   bool DecorativeArcShapesAffectDesiredSize = true;

   /// Take decorative crosshair shapes into account when computing the widget's desired size
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes", AdvancedDisplay)
   bool DecorativeCrosshairShapesAffectDesiredSize = true;

   /// Take decorative disc shapes into account when computing the widget's desired size
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes", AdvancedDisplay)
   bool DecorativeDiscShapesAffectDesiredSize = true;

   /// Take decorative line shapes into account when computing the widget's desired size
   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decorative Shapes", AdvancedDisplay)
   bool DecorativeLineShapesAffectDesiredSize = false;

   UFUNCTION(BlueprintPure, Category = "Dynamic Reticle")
   float GetReticleRadius() const;

   // This block of setters is required boilerplate that enables these properties to be animated in UMG animations

   /// Sets a new radius for the reticle and any components that are relative to the reticle radius
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetReticleRadius(float newReticleRadius);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetReticleTintColor(FLinearColor newDiscColor);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetArcRadius(float newArcRadius);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetArcColor(FLinearColor newArcColor);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetArcThickness(float newArcThickness);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetArcRangeStartDegrees(float newArcRangeStartDegrees);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetArcRangeEndDegrees(float newArcRangeEndDegrees);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetArcOriginOffset(FVector2D newArcOriginOffset);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetCrosshairCenterSpacing(float newCrosshairCenterSpacing);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetCrosshairLength(float newCrosshairLength);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetCrosshairThickness(float newCrosshairThickness);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetCrosshairColor(FLinearColor newCrosshairColor);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetCrosshairRotationDegrees(float newCrosshairRotationDegrees);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetCrosshairOriginOffset(FVector2D newCrosshairOriginOffset);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscRadius(float newDiscRadius);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscOutlineThickness(float newDiscOutlineThickness);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscTintColor(FLinearColor newDiscTintColor);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscBorderColor(FLinearColor newDiscBorderColor);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscBorderThickness(float newDiscBorderThickness);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscArcRangeStartDegrees(float newDiscArcRangeStartDegrees);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetDiscArcRangeEndDegrees(float newDiscArcRangeEndDegrees);
   UFUNCTION(BlueprintSetter, Category = "Dynamic Reticle")
   void SetReticleOriginOffset(FVector2D newReticleOriginOffset);

   UFUNCTION(BlueprintPure, Category = "Dynamic Reticle")
   FLinearColor GetDiscBorderColor() const;

   UFUNCTION(BlueprintPure, Category = "Dynamic Reticle")
   FFloatInterval GetDiscArcRangeDegrees() const;

   UFUNCTION(BlueprintPure, Category = "Dynamic Reticle")
   FVector2D GetReticleCenterInWidgetSpace() const
   {
      return FVector2D(GetReticleRadius()) + ReticleOriginOffset;
   }

   UFUNCTION(BlueprintPure, Category = "Dynamic Reticle")
   bool GetProgressIndicatorValue(FName indicatorName, float& value) const;

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   bool SetProgressIndicatorValue(FName indicatorName, float newValue);

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   bool ClearProgressIndicatorValue(FName indicatorName);

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   void AddTrackedActor(AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   void RemoveTrackedActor(AActor* actor);

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   void ClearTrackedActors();

   UFUNCTION(BlueprintPure, Category = "Dynamic Reticle")
   int32 NumTrackedActors() const { return _trackedActors.Num(); }

   using FProgressIndicatorCallback = TFunction<void(UOSEDynamicReticleWidget*, double, bool)>;

   FTimerHandle StartProgressIndicatorTimerWithCallback(FName progressIndicatorName, float duration, const FProgressIndicatorCallback& callback = nullptr, bool resetFirst = true, TOptional<FVector2D> progressValueRange = NullOpt);

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   FTimerHandle StartProgressIndicatorTimer(FName progressIndicatorName, float duration, bool resetFirst = true, bool useProgressValueRange = false, FVector2D progressValueRange = FVector2D(0, 1));

   DECLARE_DYNAMIC_DELEGATE_TwoParams(FProgressIndicatorTimerEvent, double, elapsedSeconds, bool, complete);

   UFUNCTION(BlueprintCallable, Category = "Dynamic Reticle")
   FTimerHandle StartProgressIndicatorTimerWithDelegate(FName progressIndicatorName, float duration, FProgressIndicatorTimerEvent onProgressUpdated, bool resetFirst = true, bool useProgressValueRange = false, FVector2D progressValueRange = FVector2D(0, 1));

protected:
   virtual void _OnPaintDynamicReticle(FPaintContext& context, const FVector2D& radialOrigin);

   DECLARE_DYNAMIC_DELEGATE_TwoParams(FDynamicReticlePaintEvent, UPARAM(ref) FPaintContext&, context, FVector2D, radialOrigin);
   UPROPERTY(BlueprintReadWrite, Category = "Dynamic Reticle")
   FDynamicReticlePaintEvent OnPaintDynamicReticle;

   virtual void _OnPaintTrackedActor(FPaintContext& context, const FVector2D& radialOrigin, AActor* actor, bool positionIsValid, const FVector2D& newActorPosition);

   DECLARE_DYNAMIC_DELEGATE_FiveParams(FDynamicReticleActorTrackingEvent, UPARAM(ref) FPaintContext&, context, FVector2D, radialOrigin, AActor*, actor, bool, positionIsValid, FVector2D, newPosition);
   UPROPERTY(BlueprintReadWrite, Category = "Dynamic Reticle")
   FDynamicReticleActorTrackingEvent OnPaintTrackedActor;

   DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(FVector, FDynamicReticleOverrideGetActorLocation, AActor*, actor);

   /// Can assign a function to this to override the world location returned for a given actor when mapping it to a screen-space position in the reticle.
   /// If not overridden, GetActorLocation() will be used.
   UPROPERTY(BlueprintReadWrite, Category = "Dynamic Reticle")
   FDynamicReticleOverrideGetActorLocation OverrideGetTrackedActorLocation;

   /// Enables the _Tick function (if it's not already enabled)
   void _EnableReticleTick();

   /// Disables the _Tick function (if it's enabled)
   void _DisableReticleTick();

private:
   FFloatInterval _CalcDiscRadiusWithDiscOutlineThickness(float outlineThickness, float outerRadius, float* outAdjustedOutlineThickness = nullptr) const;

   /// Compute the radius of this reticle that is passed to the underlying Slate widget to compute it's desired size in layouts
   float _CalcReticleRadiusForWidgetDesiredSize() const;

   struct FTrackedActor
   {
      TWeakObjectPtr<AActor> Actor;
      bool NeedsWidgetPositionUpdate = false;
      bool HaveValidWidgetPosition = false;
      FVector2D LastWidgetPosition = FVector2D::Zero();
   };
   TArray<FTrackedActor> _trackedActors;

   FTSTicker::FDelegateHandle _tickTimerHandle;

protected:
   TSharedPtr<SOSERadialBackground> _widget;
};
