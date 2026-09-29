// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Components/ContentWidget.h"

// ose
#include "UI/OSERadialPaintLibrary.h"

#include "OSERadialBackground.generated.h"

class SOSERadialBackground;

/// Dynamic radial background.
/// Can be used for anything that wants to show a radial-like interface.
/// This does not depend on OSERadialWidget, but OSERadialWidget can automatically set up its items on this.
UCLASS(BlueprintType, Blueprintable, meta = (DisableNativeTick))
class OSECORE_API UOSERadialBackground : public UContentWidget
{
   GENERATED_BODY()

public:
   UOSERadialBackground();

   // From UWidget
   virtual TSharedRef<SWidget> RebuildWidget() override;
   virtual void SynchronizeProperties() override;
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

private:
   void _OnRadialPaint(FOSERadialPaintContext& paintCtx, const FVector2f& radialOrigin, const FFloatInterval& radius);

public:
   /// Child slot horizontal alignment
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Child Layout")
   TEnumAsByte<EHorizontalAlignment> HorizontalAlign = HAlign_Fill;

   /// Child slot vertical alignment
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Child Layout")
   TEnumAsByte<EVerticalAlignment> VerticalAlign = VAlign_Fill;

   /// When hovering over an item, the number of seconds to interpolate in/out the hover params in FOSERadialSlice
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial", meta = (ClampMin = "0.01", UIMin = "0.01", ForceUnits = "seconds"))
   float HoverInterpSpeed = 0.25f;

   /// If enabled, if the radial's radius is smaller than the available widget space, it will be adjusted to match.
   /// If disabled, the radial will be drawn at the radius specified in Background regardless of the size of the widget.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial")
   bool ClampBackgroundRadiusToWidgetSize = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial")
   FOSERadialDiscShape Background;

   /// The inner border to draw inside the radial background (set thickness to zero to disable)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial")
   FOSELineParams BackgroundInnerBorder{ 1.0f, FLinearColor::Gray };

   /// The outer border to draw around the radial background (set thickness to zero to disable)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial")
   FOSELineParams BackgroundOuterBorder{ 1.0f, FLinearColor::White };

#if WITH_EDITORONLY_DATA
   /// Toggles visibility of item templates in the design view
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial|Items")
   bool ShowItemTemplatesInDesignView = true;

   /// Toggles visibility of item templates _names_ in the design view
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial|Items")
   bool ShowItemTemplateNamesInDesignView = true;
#endif

   /// If enabled, this widget's desired size will take item templates sizes into account
   /// If disabled it will compute the desired size from just the background
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial")
   bool AllowItemTemplatesToAffectWidgetSize = true;

   /// You can create templates for different item types here. You can use the function GetItemTemplate to retrieve these at runtime to get
   /// templates when creating new items. If you're using OSERadialWidget, you can also specify a template name in FOSERadialItemInfo.
   /// Theoretically this should be stored elsewhere, but in practice it's incredibly useful to have this as part of the background widget because you get
   /// a live preview in the designer while editing it.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial|Items")
   TMap<FName, FOSERadialSlice> ItemTemplates;

   /// Extra decorative radial discs to draw
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial|Decorative Shapes")
   TArray<FOSERadialDiscShape> ExtraDiscs;

   /// Extra decorative rings to draw
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial|Decorative Shapes")
   TArray<FOSERadialArcShape> ExtraArcs;

   /// Extra decorative lines to draw
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Radial|Decorative Shapes")
   TArray<FOSERadialLineShape> ExtraLines;

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   FVector2D GetRadialCenter() const;

   /// Gets the location of a background item relative to the center of the wheel.
   /// @param location The item's location
   /// @param itemIndex Background item index
   /// @param normalizedDistanceFromCenter The normalized distance between the radial background's min and max radius. A value of 0.5 returns the center.
   /// @param useItemRadiusForDistanceFromCenter If true, normalizedDistanceFromCenter uses the item's radius range rather than the whole wheel's radius range.
   /// @param normalizedAngleInsideItem The normalized angle between the item's arc angle min and max range. A value of 0.5 returns the center.
   UFUNCTION(BlueprintPure, Category = "Radial Background")
   bool GetRadialBackgroundItemLocation(FVector2D& location, int32 itemIndex, float normalizedDistanceFromCenter = 0.5f, bool useItemRadiusForDistanceFromCenter = false, float normalizedAngleInsideItem = 0.5f) const;

   /// Updates the hover state to point to the specified angle.
   /// If forceSelectItem is true, the background item at forceSelectedIndex will be used for the hover state regardless of the current hover angle.
   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   void UpdateHoverState(bool isHovering, float hoverAngleDegrees, bool forceSelectItem = false, int32 forceSelectedIndex = -1);

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   int32 GetHoveredItemIndex() const;

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   bool GetItemTemplate(FName name, FOSERadialSlice& outItem) const;

   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   void SetBackgroundItems(const TArray<FOSERadialSlice>& newBackgroundItems);

   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   void ClearBackgroundItems();

   FORCEINLINE const TArray<FOSERadialSlice>& GetBackgroundItems() const { return _backgroundItems; }

   FOSERadialSlice& GetBackgroundItemRef(int32 index) { check(_backgroundItems.IsValidIndex(index)); return _backgroundItems[index]; }
   const FOSERadialSlice& GetBackgroundItemRef(int32 index) const { check(_backgroundItems.IsValidIndex(index)); return _backgroundItems[index]; }

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   FORCEINLINE int32 NumBackgroundItems() const { return _backgroundItems.Num(); }

   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   bool GetBackgroundItem(int32 index, FOSERadialSlice& outRadialSlice);

   /// Replaces an existing background item with a new one. Returns true if the item was successfully updated.
   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   bool SetBackgroundItem(int32 index, const FOSERadialSlice& newItemRadialSlice);

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   bool GetBackgroundItemArcRange(int32 index, FFloatInterval& itemArcRangeDegrees) const;

   /// Updates an item's arc angle. Returns true if the index was valid.
   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   bool SetBackgroundItemArcRange(int32 index, const FFloatInterval& newItemArcRangeDegrees);

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   bool GetBackgroundItemDisabled(int32 index, bool& isDisabled) const;

   /// Sets the disabled state of an item
   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   bool SetBackgroundItemDisabled(int32 index, bool newDisabled);

   UFUNCTION(BlueprintPure, Category = "Radial Background")
   bool GetBackgroundItemHidden(int32 index, bool& isHidden) const;

   /// Sets the hidden state of an item
   UFUNCTION(BlueprintCallable, Category = "Radial Background")
   bool SetBackgroundItemHidden(int32 index, bool newHidden);

   /// Recomputes all background item arc angles.
   /// @param itemWeights If non-empty, weights will be used to determine the size of each item relative to other items. Otherwise all items will take up an equal amount of space.
   /// @param shrinkItemArcAngleDegrees Once placed, items will have their arc angles reduced by this amount (this can be used to add padding between radial items).
   /// @param startAngleDegrees Starting angle for placement
   /// @param totalArcRangeDegrees How far to wind around the radial (starting from startAngleDegrees) when placing items
   /// @param clockwise If true, placement will continue clockwise for all items. Otherwise counter-clockwise.
   /// @param reverseItemOrder If true, the last item will be placed first, followed by the second to last item, and so on.
   UFUNCTION(BlueprintCallable, Category = "Radial Background", meta = (AutoCreateRefTerm = "itemWeights"))
   void UpdateAllBackgroundItemArcAngles(const TArray<int32>& itemWeights, float shrinkItemArcAngleDegrees = 2.0f, float startAngleDegrees = -90.0f, float totalArcRangeDegrees = 360.0f, bool clockwise = true, bool reverseItemOrder = false);

private:
   bool _Tick(float deltaSeconds);
   int32 _FindFirstBackgroundItemAtAngle(float angleDegrees) const;

private:
   TSharedPtr<SOSERadialBackground> _widget;

   FTSTicker::FDelegateHandle _tickTimerHandle;

   UPROPERTY(Transient)
   TArray<FOSERadialSlice> _backgroundItems;

   // widget state
   FBox2D _radialBoundingBox;

   // hover state
   bool _isHovering = false;
   int32 _currentHoveredItemIndex = INDEX_NONE;
   float _currentHoveredItemStartTimeSec = 0.0f;
   float _hoverAngleDegrees = 0.0f;
};
