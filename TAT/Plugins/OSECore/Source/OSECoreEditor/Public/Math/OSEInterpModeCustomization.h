// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Math/OSEMathFunctionLibrary.h"

class FDetailWidgetRow;
class IPropertyHandle;
class IPropertyTypeCustomizationUtils;
class SScrollBox;

using FOSEGraphPreviewFunction = TFunction<float(float)>;

class OSECOREEDITOR_API SOSEGraphPreviewBlock : public SLeafWidget
{
   SLATE_DECLARE_WIDGET(SOSEGraphPreviewBlock, SLeafWidget)

public:
   SLATE_BEGIN_ARGS(SOSEGraphPreviewBlock)
      : _LineThickness(1.0f)
      , _LineColor(FLinearColor::White)
      , _BackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.AlphaBackground"))
      , _BackgroundTint(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
      , _Size(FVector2D(16, 16))
   {}
      SLATE_ATTRIBUTE(float, LineThickness)
      SLATE_ATTRIBUTE(FLinearColor, LineColor)
      SLATE_ATTRIBUTE(const FSlateBrush*, BackgroundBrush)
      SLATE_ATTRIBUTE(FLinearColor, BackgroundTint)
      SLATE_ATTRIBUTE(FVector2D, Size)
      SLATE_ATTRIBUTE(int32, NumPoints)
      SLATE_ARGUMENT(FOSEGraphPreviewFunction, Func)
      SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
   SLATE_END_ARGS()

public:
   SOSEGraphPreviewBlock();
   void Construct(const FArguments& InArgs);

   void SetGraphFunction(const FOSEGraphPreviewFunction& newFunc);

private:
   // SWidget overrides
   virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
   virtual FVector2D ComputeDesiredSize(float) const override { return Size.Get(); }
   virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
   {
      return MouseButtonDownHandler.IsBound() ? MouseButtonDownHandler.Execute(MyGeometry, MouseEvent) : FReply::Unhandled();
   }

   FOSEGraphPreviewFunction Func;

   TSlateAttribute<float> LineThickness;
   TSlateAttribute<FLinearColor> LineColor;
   TSlateAttribute<const FSlateBrush*> BackgroundBrush;
   TSlateAttribute<FLinearColor> BackgroundTint;
   TSlateAttribute<int32> NumPoints;
   TSlateAttribute<FVector2D> Size;
   FPointerEventHandler MouseButtonDownHandler;
};

class OSECOREEDITOR_API FOSEInterpModeCustomization : public IPropertyTypeCustomization
{
   struct FOSEInterpCurveType
   {
      EOSEInterpMode Mode = EOSEInterpMode::Linear;
      FString Name;
      FText Label;
   };

public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance();

protected:
   // IPropertyTypeCustomization interface
   virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

protected:
   /// Get the current property value
   TOptional<EOSEInterpMode> GetCurrentInterpMode(FPropertyAccess::Result* OutAccessResult = nullptr) const;

   TSharedRef<SWidget> GetComboButtonMenuContent();

   // dropdown callbacks
   void HandleSelectionChanged(TSharedPtr<FOSEInterpCurveType> InItem, ESelectInfo::Type InSelectionType);
   TSharedRef<ITableRow> HandleGenerateRow(TSharedPtr<FOSEInterpCurveType> InItem, const TSharedRef<STableViewBase>& InOwnerTable);
   void HandleFilterTextChanged(const FText& InFilterText);

   void UpdateSelection();
   void RefreshCurveTypesList();

protected:
   /// InterpMode enum property
   TSharedPtr<IPropertyHandle> PropertyHandle;

   // Widget handles
   TSharedPtr<SListView<TSharedPtr<FOSEInterpCurveType>>> NameListView;

   // Currently selected filter type
   enum class EFilterType
   {
      None,
      Math,
      Easing,
      EaseIn,
      EaseOut,
      EaseInOut,
   };

   // List filters
   EFilterType FilterByType = EFilterType::None;
   FString FilterText;

   /// List contents
   TArray<TSharedPtr<FOSEInterpCurveType>> FilteredCurveTypes;
};
