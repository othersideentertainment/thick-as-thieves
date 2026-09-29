// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSERadialBackground.h"

// ue
#include "Blueprint/UserWidget.h"

// ose
#include "Kismet/KismetMathLibrary.h"
#include "Math/OSEMathFunctionLibrary.h"
#include "UI/Slate/SOSERadialBackground.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSERadialBackground)

DEFINE_LOG_CATEGORY_STATIC(LogOSERadialBackground, Log, All);

//---------------------------------------------------------------------------------------
// UOSERadialBackground
//---------------------------------------------------------------------------------------

UOSERadialBackground::UOSERadialBackground()
   : Super()
{
}

TSharedRef<SWidget> UOSERadialBackground::RebuildWidget()
{
   _widget = SNew(SOSERadialBackground)
      .OnRadialPaint_UObject(this, &UOSERadialBackground::_OnRadialPaint);

   if (GetChildrenCount() > 0)
   {
      UPanelSlot* slot = GetContentSlot();
      check(slot != nullptr && slot->Content);
      _widget->SetContent(slot->Content->TakeWidget());
   }

   //TODO: Only enable the tick function while interpolating radial slices
   if (_tickTimerHandle.IsValid())
   {
      FTSTicker::GetCoreTicker().RemoveTicker(_tickTimerHandle);
   }
   _tickTimerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UOSERadialBackground::_Tick));

   return _widget.ToSharedRef();
}

void UOSERadialBackground::SynchronizeProperties()
{
   Super::SynchronizeProperties();

   if (!_widget.IsValid())
   {
      return;
   }

   // For the widget's desired size, factor in item templates if desired (0 disables this behavior and will fall back on the background radius)
   float radiusForDesiredSize = 0;
   if (AllowItemTemplatesToAffectWidgetSize)
   {
      radiusForDesiredSize = Background.Radius.Max;
      for (const auto& pair : ItemTemplates)
      {
         if (pair.Value.Background.Radius.Max > radiusForDesiredSize)
         {
            radiusForDesiredSize = pair.Value.Background.Radius.Max;
         }
         if (pair.Value.UseHoverBackgroundRadius && pair.Value.HoverBackgroundRadius.Max > radiusForDesiredSize)
         {
            radiusForDesiredSize = pair.Value.HoverBackgroundRadius.Max;
         }
      }
   }

   _widget->SetHAlign(HorizontalAlign);
   _widget->SetVAlign(VerticalAlign);
   _widget->SetRadiusForDesiredSize(radiusForDesiredSize);
   _widget->SetClampRadius(ClampBackgroundRadiusToWidgetSize);
   _widget->SetOriginOffset(Background.Origin);
   _widget->SetRadius(Background.Radius);
   _widget->SetArcRangeDegrees(Background.ArcRangeDegrees);
   _widget->SetRadialBackground(&Background.Brush);
   _widget->SetUVMode(Background.UVMode);
   _widget->SetResolution(Background.Resolution);
}

#if WITH_EDITOR
const FText UOSERadialBackground::GetPaletteCategory()
{
   return FText::FromString(TEXT("OSE"));
}
#endif

void UOSERadialBackground::ReleaseSlateResources(bool releaseChildren)
{
   Super::ReleaseSlateResources(releaseChildren);
   _widget.Reset();
}

UClass* UOSERadialBackground::GetSlotClass() const
{
   return UPanelSlot::StaticClass();
}

void UOSERadialBackground::OnSlotAdded(UPanelSlot* slot)
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

void UOSERadialBackground::OnSlotRemoved(UPanelSlot* slot)
{
   if (_widget.IsValid())
   {
      _widget->SetContent(SNullWidget::NullWidget);
   }
}

void UOSERadialBackground::_OnRadialPaint(FOSERadialPaintContext& paintCtx, const FVector2f& radialOrigin, const FFloatInterval& radius)
{
   if (!_widget.IsValid())
   {
      return;
   }

   // cache the widget's bounding box, since apparently this is the only place that we have access to that data
   const FSlateRect widgetRect = paintCtx.GetAllottedGeometry().GetLayoutBoundingRect();
   _radialBoundingBox = FBox2D(widgetRect.GetTopLeft(), widgetRect.GetBottomRight());

   const FFloatInterval radialArcRangeDeg = _widget->GetArcRangeDegreesAsInterval();
   const int32 radialResolution = _widget->GetResolution();

   if (BackgroundInnerBorder && radius.Min > 0)
   {
      paintCtx.DrawArc(radialOrigin, radius.Min, radialArcRangeDeg, BackgroundInnerBorder, radialResolution);
   }

   if (BackgroundOuterBorder && radius.Max > 0)
   {
      paintCtx.DrawArc(radialOrigin, radius.Max, radialArcRangeDeg, BackgroundOuterBorder, radialResolution);
   }

   for (const FOSERadialDiscShape& extraDisc : ExtraDiscs)
   {
      paintCtx.DrawDisc(extraDisc, radialOrigin);
   }

   for (const FOSERadialArcShape& extraArc : ExtraArcs)
   {
      paintCtx.DrawArc(extraArc, radialOrigin);
   }

   for (const FOSERadialLineShape& extraLine : ExtraLines)
   {
      paintCtx.DrawLineFromOriginAndAngle(extraLine, radialOrigin);
   }

#if WITH_EDITOR && WITH_EDITORONLY_DATA
   if (IsDesignTime() && (ShowItemTemplatesInDesignView || ShowItemTemplateNamesInDesignView))
   {
      // Sort the template keys - the actual order isn't important, but we want a _predictable_ order to avoid annoying z-fighting when template items overlap
      TArray<FName, TInlineAllocator<8>> templateKeys;
      ItemTemplates.GetKeys(templateKeys);
      templateKeys.StableSort([](const FName& lhs, const FName& rhs) { return lhs.FastLess(rhs); });
      for (const FName& key : templateKeys)
      {
         const FOSERadialSlice& templateSlice = ItemTemplates[key];
         if (ShowItemTemplatesInDesignView)
         {
            paintCtx.DrawSlice(templateSlice, radialOrigin);
         }
         if (ShowItemTemplateNamesInDesignView && !templateSlice.IsHidden)
         {
            paintCtx.DrawText(templateSlice.GetBoundingBox().GetCenter() + radialOrigin, key.ToString());
         }
      }
   }
#endif

   for (const FOSERadialSlice& item : _backgroundItems)
   {
      paintCtx.DrawSlice(item, radialOrigin);
   }
}

FVector2D UOSERadialBackground::GetRadialCenter() const
{
   return FVector2D{ Background.Radius.Max };
}

bool UOSERadialBackground::GetRadialBackgroundItemLocation(FVector2D& location, int32 itemIndex, float normalizedDistanceFromCenter, bool useItemRadiusForDistanceFromCenter, float normalizedAngleInsideItem) const
{
   if (!_backgroundItems.IsValidIndex(itemIndex))
   {
      location = FVector2D::Zero();
      return false;
   }
   const FOSERadialSlice& slice = _backgroundItems[itemIndex];
   const float distanceFromCenter = useItemRadiusForDistanceFromCenter
      ? FMath::Lerp(slice.Background.Radius.Min, slice.Background.Radius.Max, normalizedDistanceFromCenter)
      : FMath::Lerp(Background.Radius.Min, Background.Radius.Max, normalizedDistanceFromCenter);
   const float angleDegrees = UOSERadialPaintLibrary::LerpArcAngleDegrees(slice.Background.ArcRangeDegrees, normalizedAngleInsideItem);
   location = UKismetMathLibrary::GetRotated2D(FVector2D(distanceFromCenter, 0.0f), angleDegrees);
   return true;
}

void UOSERadialBackground::UpdateHoverState(bool isHovering, float hoverAngleDegrees, bool forceSelectItem, int32 forceSelectedIndex)
{
   _isHovering = isHovering;
   _hoverAngleDegrees = hoverAngleDegrees;

   int32 newHoveredItemIndex = INDEX_NONE;
   if (forceSelectItem)
   {
      newHoveredItemIndex = _backgroundItems.IsValidIndex(forceSelectedIndex) ? forceSelectedIndex : INDEX_NONE;
   }
   else
   {
      newHoveredItemIndex = _FindFirstBackgroundItemAtAngle(hoverAngleDegrees);
   }

   if (newHoveredItemIndex != _currentHoveredItemIndex)
   {
      _currentHoveredItemIndex = newHoveredItemIndex;
      UWorld* world = GetWorld();
      _currentHoveredItemStartTimeSec = world ? world->GetTimeSeconds() : 0.0f;
   }
}

int32 UOSERadialBackground::GetHoveredItemIndex() const
{
   return _isHovering ? _currentHoveredItemIndex : INDEX_NONE;
}

bool UOSERadialBackground::GetItemTemplate(FName name, FOSERadialSlice& outItem) const
{
   if (const FOSERadialSlice* itemTemplate = ItemTemplates.Find(name))
   {
      outItem = *itemTemplate;
      return true;
   }
   return false;
}

void UOSERadialBackground::SetBackgroundItems(const TArray<FOSERadialSlice>& newBackgroundItems)
{
   _backgroundItems = newBackgroundItems;
   _currentHoveredItemIndex = _isHovering ? _FindFirstBackgroundItemAtAngle(_hoverAngleDegrees) : INDEX_NONE;
}

void UOSERadialBackground::ClearBackgroundItems()
{
   _backgroundItems.Reset();
   _currentHoveredItemIndex = INDEX_NONE;
}

bool UOSERadialBackground::GetBackgroundItem(int32 index, FOSERadialSlice& outRadialSlice)
{
   if (_backgroundItems.IsValidIndex(index))
   {
      outRadialSlice = _backgroundItems[index];
      return true;
   }
   outRadialSlice = FOSERadialSlice();
   return false;
}

bool UOSERadialBackground::SetBackgroundItem(int32 index, const FOSERadialSlice& newItemRadialSlice)
{
   if (_backgroundItems.IsValidIndex(index))
   {
      _backgroundItems[index] = newItemRadialSlice;
      return true;
   }
   return false;
}

bool UOSERadialBackground::GetBackgroundItemArcRange(int32 index, FFloatInterval& itemArcRangeDegrees) const
{
   if (_backgroundItems.IsValidIndex(index))
   {
      itemArcRangeDegrees = _backgroundItems[index].Background.ArcRangeDegrees;
      return true;
   }
   itemArcRangeDegrees = FFloatInterval(0.0f, 0.0f);
   return false;
}

bool UOSERadialBackground::SetBackgroundItemArcRange(int32 index, const FFloatInterval& newItemArcRangeDegrees)
{
   if (_backgroundItems.IsValidIndex(index))
   {
      _backgroundItems[index].Background.ArcRangeDegrees = newItemArcRangeDegrees;
      return true;
   }
   return false;
}

bool UOSERadialBackground::GetBackgroundItemDisabled(int32 index, bool& isDisabled) const
{
   if (_backgroundItems.IsValidIndex(index))
   {
      isDisabled = _backgroundItems[index].IsDisabled;
      return true;
   }
   isDisabled = false;
   return false;
}

bool UOSERadialBackground::SetBackgroundItemDisabled(int32 index, bool newDisabled)
{
   if (_backgroundItems.IsValidIndex(index))
   {
      _backgroundItems[index].IsDisabled = newDisabled;
      return true;
   }
   return false;
}

bool UOSERadialBackground::GetBackgroundItemHidden(int32 index, bool& isHidden) const
{
   if (_backgroundItems.IsValidIndex(index))
   {
      isHidden = _backgroundItems[index].IsHidden;
      return true;
   }
   isHidden = false;
   return false;
}

bool UOSERadialBackground::SetBackgroundItemHidden(int32 index, bool newHidden)
{
   if (_backgroundItems.IsValidIndex(index))
   {
      _backgroundItems[index].IsHidden = newHidden;
      return true;
   }
   return false;
}

void UOSERadialBackground::UpdateAllBackgroundItemArcAngles(const TArray<int32>& itemWeights, float shrinkItemArcAngleDegrees, float startAngleDegrees, float totalArcRangeDegrees, bool clockwise, bool reverseItemOrder)
{
   auto getItemWeight = [this, &itemWeights](int32 itemIndex) -> int32
   {
      return itemWeights.IsValidIndex(itemIndex) ? FMath::Max(1, itemWeights[itemIndex]) : 1;
   };

   int32 totalItemWeight = totalItemWeight = _backgroundItems.Num();
   if (itemWeights.Num() > 0)
   {
      for (int32 i = 0; i < _backgroundItems.Num(); i++)
      {
         totalItemWeight += getItemWeight(i);
      }
   }

   const float angleIncrementDegrees = totalArcRangeDegrees / static_cast<float>(totalItemWeight);
   float lastItemEndAngleDegrees = startAngleDegrees;
   auto calcRadialItemAngle = [&](int32 itemIndex, int32 weight)
   {
      FOSERadialSlice& item = _backgroundItems[itemIndex];
      const float increment = angleIncrementDegrees * static_cast<float>(weight) * (clockwise ? 1.0f : -1.0f);
      item.Background.ArcRangeDegrees = FFloatInterval(
         lastItemEndAngleDegrees,
         lastItemEndAngleDegrees + increment);
      lastItemEndAngleDegrees = item.Background.ArcRangeDegrees.Max;

      if (shrinkItemArcAngleDegrees != 0)
      {
         item.Background.ArcRangeDegrees.Min += shrinkItemArcAngleDegrees * 0.5f;
         item.Background.ArcRangeDegrees.Max -= shrinkItemArcAngleDegrees * 0.5f;
      }
   };

   if (reverseItemOrder)
   {
      for (int32 i = _backgroundItems.Num() - 1; i >= 0; --i)
      {
         calcRadialItemAngle(i, getItemWeight(i));
      }
   }
   else
   {
      for (int32 i = 0; i < _backgroundItems.Num(); i++)
      {
         calcRadialItemAngle(i, getItemWeight(i));
      }
   }
}


bool UOSERadialBackground::_Tick(float deltaSeconds)
{
   // adjust items hover lerp alpha
   const float interpIncrement = deltaSeconds * (1.0f / FMath::Max(0.01f, HoverInterpSpeed));

   for (int32 i = 0; i < _backgroundItems.Num(); i++)
   {
      FOSERadialSlice& item = _backgroundItems[i];
      if (item.IsHidden)
      {
         continue;
      }

      const bool itemHovered = _isHovering && i == _currentHoveredItemIndex && !item.IsDisabled;
      if (item.HoverLerpAlpha > 0.0f && !itemHovered)
      {
         item.HoverLerpAlpha = FMath::Max(0.0f, item.HoverLerpAlpha - interpIncrement);
      }
      else if (item.HoverLerpAlpha < 1.0f && itemHovered)
      {
         item.HoverLerpAlpha = FMath::Min(1.0f, item.HoverLerpAlpha + interpIncrement);
      }
   }

   // we keep ticking if this returns true
   return _widget.IsValid();
}

int32 UOSERadialBackground::_FindFirstBackgroundItemAtAngle(float angleDegrees) const
{
   for (int32 i = 0; i < _backgroundItems.Num(); i++)
   {
      if (UOSERadialPaintLibrary::AngleContainedInRadialArc(angleDegrees, _backgroundItems[i].Background.ArcRangeDegrees)
         && !_backgroundItems[i].IsDisabled
         && !_backgroundItems[i].IsHidden)
      {
         return i;
      }
   }
   return INDEX_NONE;
}
