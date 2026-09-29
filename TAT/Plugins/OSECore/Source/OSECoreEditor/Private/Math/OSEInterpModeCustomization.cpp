// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Math/OSEInterpModeCustomization.h"
#include "Math/OSEMathFunctionLibrary.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IPropertyTypeCustomization.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "SListViewSelectorDropdownMenu.h"

#define LOCTEXT_NAMESPACE "OSELineParamsCustomization"

//---------------------------------------------------------------------------------------
// SOSEGraphPreviewBlock
//---------------------------------------------------------------------------------------

SLATE_IMPLEMENT_WIDGET(SOSEGraphPreviewBlock)
void SOSEGraphPreviewBlock::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, LineThickness, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, LineColor, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, BackgroundBrush, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, BackgroundTint, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, NumPoints, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, Size, EInvalidateWidgetReason::Layout);
}

SOSEGraphPreviewBlock::SOSEGraphPreviewBlock()
   : LineThickness(*this, 1.0f)
   , LineColor(*this, FLinearColor::White)
   , BackgroundBrush(*this, nullptr)
   , BackgroundTint(*this, FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
   , NumPoints(*this, 32)
   , Size(*this, FVector2D(16, 16))
{
}

void SOSEGraphPreviewBlock::Construct(const FArguments& InArgs)
{
   LineThickness.Assign(*this, InArgs._LineThickness);
   LineColor.Assign(*this, InArgs._LineColor);
   BackgroundBrush.Assign(*this, InArgs._BackgroundBrush);
   BackgroundTint.Assign(*this, InArgs._BackgroundTint);
   NumPoints.Assign(*this, InArgs._NumPoints);
   Size.Assign(*this, InArgs._Size);
   Func = InArgs._Func;
   MouseButtonDownHandler = InArgs._OnMouseButtonDown;
}

void SOSEGraphPreviewBlock::SetGraphFunction(const FOSEGraphPreviewFunction& newFunc)
{
   Func = newFunc;
   Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SOSEGraphPreviewBlock::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
   auto drawRect = [&OutDrawElements, &LayerId, &AllottedGeometry](const FVector2f& origin, const FVector2f& size, const FSlateBrush* brush, const FLinearColor& tintColor)
   {
      FSlateDrawElement::MakeBox(
         OutDrawElements,
         LayerId++,
         AllottedGeometry.ToPaintGeometry(size, FSlateLayoutTransform(origin)),
         brush,
         ESlateDrawEffect::None,
         tintColor);
   };

   auto drawLine = [&OutDrawElements, &LayerId, &AllottedGeometry](TArray<FVector2f> points, const FLinearColor& color, float thickness)
   {
      constexpr bool antialias = true;
      ensure(points.Num() >= 2);
      FSlateDrawElement::MakeLines(
         OutDrawElements,
         LayerId++,
         AllottedGeometry.ToPaintGeometry(),
         MoveTemp(points),
         ESlateDrawEffect::None,
         color,
         antialias,
         thickness);
   };

   const FVector2f backgroundPadding{ 2, 2 };
   const FVector2f backgroundOrigin = FVector2f::Zero() + backgroundPadding;
   const FVector2f backgroundSize = AllottedGeometry.GetLocalSize() - (backgroundPadding * 2);

   const FVector2f graphPadding{ 2, 2 };
   const FVector2f graphOrigin = backgroundOrigin + graphPadding;
   const FVector2f graphSize = backgroundSize - (graphPadding * 2);

   const FSlateBrush* slateBrush = BackgroundBrush.Get();
   if (slateBrush == nullptr)
   {
      slateBrush = FAppStyle::GetBrush("ColorPicker.RoundedAlphaBackground");
   }

   if (slateBrush != nullptr)
   {
      drawRect(backgroundOrigin, backgroundSize, slateBrush, BackgroundTint.Get());
   }

   if (Func)
   {
      TArray<FVector2f> verts;
      const int32 numPoints = FMath::Max(2, NumPoints.Get());
      verts.SetNumUninitialized(numPoints);
      for (int32 i = 0; i < numPoints; i++)
      {
         const float relX = FMath::Clamp(static_cast<float>(i) / static_cast<float>(numPoints - 1), 0.0f, 1.0f);
         verts[i] = graphOrigin + (FVector2f(relX, 1.0f - FMath::Clamp(Func(relX), 0.0f, 1.0f)) * graphSize);
      }
      drawLine(MoveTemp(verts), LineColor.Get(), LineThickness.Get());
   }

   return LayerId;
}

//---------------------------------------------------------------------------------------
// FOSEInterpModeCustomization
//---------------------------------------------------------------------------------------

TSharedRef<IPropertyTypeCustomization> FOSEInterpModeCustomization::MakeInstance()
{
   return MakeShareable(new FOSEInterpModeCustomization);
}

void FOSEInterpModeCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> InPropertyHandle, class FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
   PropertyHandle = InPropertyHandle;
   check(PropertyHandle.IsValid());

   TWeakPtr<IPropertyHandle> weakProp = InPropertyHandle;
   FOSEGraphPreviewFunction graphFunc = [weakProp](float alpha) -> float
   {
      uint8 interpModeAsByte = static_cast<uint8>(EOSEInterpMode::Linear);
      if (TSharedPtr<IPropertyHandle> prop = weakProp.Pin())
      {
         prop->GetValue(interpModeAsByte);
      }
      return UOSEMathFunctionLibrary::InterpolateNormalized(alpha, static_cast<EOSEInterpMode>(interpModeAsByte));
   };

   RefreshCurveTypesList();

   SAssignNew(NameListView, SListView<TSharedPtr<FOSEInterpCurveType>>)
      .SelectionMode(ESelectionMode::Single)
      .ListItemsSource(&FilteredCurveTypes)
      .OnSelectionChanged(this, &FOSEInterpModeCustomization::HandleSelectionChanged)
      .OnGenerateRow(this, &FOSEInterpModeCustomization::HandleGenerateRow)
   ;

   InHeaderRow.NameContent()
   [
      PropertyHandle->CreatePropertyNameWidget()
   ]
   .ValueContent()
   .MinDesiredWidth(251.0f)
   .MaxDesiredWidth(251.0f)
   [
      SNew(SHorizontalBox)
      +SHorizontalBox::Slot()
      .Padding(FMargin(0, 2.0f, 4.0f, 2.0f))
      .VAlign(VAlign_Center)
      .AutoWidth()
      [
         SNew(SBorder)
         .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
         .VAlign(VAlign_Center)
         [
            SNew(SOSEGraphPreviewBlock)
            .LineThickness(2.0f)
            .LineColor(FLinearColor::White)
            .Size(FVector2D(40.0f, 40.0f))
            .NumPoints(16)
            .BackgroundBrush(FAppStyle::GetBrush("Menu.Background"))
            .BackgroundTint(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
            .Func(graphFunc)
         ]
      ]
      +SHorizontalBox::Slot()
      .Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
      .VAlign(VAlign_Center)
      .FillWidth(1.0f)
      [
         SNew(SComboButton)
         .OnGetMenuContent(this, &FOSEInterpModeCustomization::GetComboButtonMenuContent)
         .OnMenuOpenChanged_Lambda([this](bool isOpen)
         {
            if (!isOpen)
            {
               return;
            }
            if (TOptional<EOSEInterpMode> currentMode = GetCurrentInterpMode())
            {
               int32 currentIndex = INDEX_NONE;
               for (int32 i = 0; i < FilteredCurveTypes.Num(); i++)
               {
                  if (FilteredCurveTypes[i]->Mode == *currentMode)
                  {
                     currentIndex = i;
                     break;
                  }
               }

               if (currentIndex != INDEX_NONE)
               {
                  NameListView->SetSelection(FilteredCurveTypes[currentIndex], ESelectInfo::Direct);
                  NameListView->RequestNavigateToItem(FilteredCurveTypes[currentIndex]);
               }
            }
         })
         .ButtonContent()
         [
            SNew(STextBlock)
            .Font(FAppStyle::GetFontStyle("StandardDialog.SmallFont"))
            .Text_Lambda([weakProp]()
            {
               if (TSharedPtr<IPropertyHandle> prop = weakProp.Pin())
               {
                  uint8 interpModeAsByte = static_cast<uint8>(EOSEInterpMode::Linear);
                  prop->GetValue(interpModeAsByte);
                  return StaticEnum<EOSEInterpMode>()->GetDisplayNameTextByValue((int64)interpModeAsByte);
               }
               return INVTEXT("Invalid");
            })
         ]
      ]
      // This shows a normal enum dropdown (useful fallback in case the above fancy version isn't working for some reason)
      // +SHorizontalBox::Slot()
      // .Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
      // .VAlign(VAlign_Center)
      // .FillWidth(1.0f)
      // [
      //    PropertyHandle->CreatePropertyValueWidget()
      // ]
   ];
}

void FOSEInterpModeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

TOptional<EOSEInterpMode> FOSEInterpModeCustomization::GetCurrentInterpMode(FPropertyAccess::Result* OutAccessResult) const
{
   if (PropertyHandle)
   {
      uint8 value = static_cast<uint8>(EOSEInterpMode::Linear);
      FPropertyAccess::Result accessResult = PropertyHandle->GetValue(value);
      if (OutAccessResult != nullptr)
      {
         *OutAccessResult = accessResult;
      }
      if (accessResult == FPropertyAccess::Success)
      {
         return static_cast<EOSEInterpMode>(value);
      }
   }
   return NullOpt;
}

TSharedRef<SWidget> FOSEInterpModeCustomization::GetComboButtonMenuContent()
{
   constexpr float Height = 500.0f;
   constexpr float HorizontalPadding = 2.0f;
   constexpr float VerticalPadding = 2.0f;
   constexpr float WeightOverride = 300.0f;

   auto makeFilterButton = [this](EFilterType filterType, const FText& label, const FText& desc)
   {
      return SNew(SCheckBox)
         .Style(FAppStyle::Get(), "ToggleButtonCheckbox")
         .ToolTipText(desc)
         .Padding(FMargin(4.0f, 2.0f))
         .Type(ESlateCheckBoxType::ToggleButton)
         .IsChecked_Lambda([this, filterType]()
         {
            return (FilterByType == filterType) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
         })
         .OnCheckStateChanged_Lambda([this, filterType](ECheckBoxState state)
         {
            FilterByType = (state == ECheckBoxState::Checked) ? filterType : EFilterType::None;
            RefreshCurveTypesList();
         })
         .Content()
         [
            SNew(STextBlock)
            .Text(label)
            .Font(FAppStyle::GetFontStyle("StandardDialog.SmallFont"))
         ];
   };

   TSharedRef<SSearchBox> SearchBox = SNew(SSearchBox)
      .HintText(LOCTEXT("SearchBoxHint", "Search"))
      .OnTextChanged(this, &FOSEInterpModeCustomization::HandleFilterTextChanged)
      ;

   return SNew(SBox)
      .HeightOverride(Height)
      [
         SNew(SListViewSelectorDropdownMenu<TSharedPtr<FOSEInterpCurveType>>, SearchBox, NameListView)
         [
            SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(HorizontalPadding, VerticalPadding)
            [
               SearchBox
            ]
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(HorizontalPadding, VerticalPadding)
            [
               SNew(SHorizontalBox)
               +SHorizontalBox::Slot().AutoWidth()[makeFilterButton(EFilterType::None, LOCTEXT("FilterNone", "All"), LOCTEXT("FilterNoneDesc", "Show all curves"))]
               +SHorizontalBox::Slot().AutoWidth()[makeFilterButton(EFilterType::Math, LOCTEXT("FilterMath", "Math"), LOCTEXT("FilterMathDesc", "Only show regular math functions (non-easing)"))]
               +SHorizontalBox::Slot().AutoWidth()[makeFilterButton(EFilterType::Easing, LOCTEXT("FilterEasing", "Easing"), LOCTEXT("FilterEasingDesc", "Only show easing curves"))]
               +SHorizontalBox::Slot().AutoWidth()[makeFilterButton(EFilterType::EaseIn, LOCTEXT("FilterEaseIn", "Ease-In"), LOCTEXT("FilterEaseInDesc", "Only show ease-in curves"))]
               +SHorizontalBox::Slot().AutoWidth()[makeFilterButton(EFilterType::EaseOut, LOCTEXT("FilterEaseOut", "Ease-Out"), LOCTEXT("FilterEaseOutDesc", "Only show ease-out curves"))]
               +SHorizontalBox::Slot().AutoWidth()[makeFilterButton(EFilterType::EaseInOut, LOCTEXT("FilterEaseInOut", "Ease-In-Out"), LOCTEXT("FilterEaseInOutDesc", "Only show ease-in-out curves"))]
            ]
            +SVerticalBox::Slot()
            .FillHeight(1.0f)
            .VAlign(VAlign_Fill)
            .Padding(HorizontalPadding, VerticalPadding)
            [
               SNew(SBox)
               .WidthOverride(WeightOverride)
               .HeightOverride(WeightOverride)
               [
                  SNew(SOverlay)
                  +SOverlay::Slot()
                  [
                     SNew(SBorder)
                     .BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
                     .BorderBackgroundColor(FAppStyle::Get().GetSlateColor("Colors.Input"))
                  ]
                  +SOverlay::Slot()
                  [
                     NameListView.ToSharedRef()
                  ]
               ]
            ]
         ]
      ];
}

void FOSEInterpModeCustomization::HandleSelectionChanged(TSharedPtr<FOSEInterpCurveType> InItem, ESelectInfo::Type InSelectionType)
{
   if (InSelectionType == ESelectInfo::Direct)
   {
      return;
   }

   if (PropertyHandle && InItem)
   {
      PropertyHandle->SetValue(static_cast<uint8>(InItem->Mode));
   }

   // Close the popup when clicking a new value
   if (InSelectionType == ESelectInfo::OnMouseClick && NameListView)
   {
      if (TSharedPtr<SWindow> ParentContextMenuWindow = FSlateApplication::Get().FindWidgetWindow(NameListView.ToSharedRef()))
      {
         FSlateApplication::Get().RequestDestroyWindow(ParentContextMenuWindow.ToSharedRef());
      }
   }
}

TSharedRef<ITableRow> FOSEInterpModeCustomization::HandleGenerateRow(TSharedPtr<FOSEInterpCurveType> InItem, const TSharedRef<STableViewBase>& InOwnerTable)
{
   const EOSEInterpMode mode = InItem->Mode;
   return
      SNew(STableRow<TSharedPtr<FSmartName>>, InOwnerTable)
      .Padding(FMargin(8.0f, 0.0f))
      [
         SNew(SHorizontalBox)
         +SHorizontalBox::Slot()
         .VAlign(VAlign_Center)
         .AutoWidth()
         [
            SNew(SOSEGraphPreviewBlock)
            .LineThickness(1.0f)
            .LineColor(FLinearColor::White)
            .Size(FVector2D(28.0f, 28.0f))
            .NumPoints(10)
            .BackgroundBrush(FAppStyle::GetBrush("Menu.Background"))
            .BackgroundTint(FLinearColor(0.0f, 0.0f, 0.0f, 0.5f))
            .Func([mode](float alpha) { return UOSEMathFunctionLibrary::InterpolateNormalized(alpha, mode); })
         ]
         +SHorizontalBox::Slot()
         .VAlign(VAlign_Center)
         .FillWidth(1.0f)
         .Padding(4.0f, 1.0f)
         [
            SNew(STextBlock)
            .Text(InItem->Label)
            .HighlightText_Lambda([this]() { return FText::FromString(FilterText); })
         ]
      ];
}

void FOSEInterpModeCustomization::HandleFilterTextChanged(const FText& InFilterText)
{
   FilterText = InFilterText.ToString();
   RefreshCurveTypesList();
}

void FOSEInterpModeCustomization::UpdateSelection()
{
   TOptional<EOSEInterpMode> currentMode = GetCurrentInterpMode();
   if (!currentMode)
   {
      return;
   }

   int32 currentIndex = INDEX_NONE;
   for (int32 i = 0; i < FilteredCurveTypes.Num(); i++)
   {
      if (FilteredCurveTypes[i]->Mode == *currentMode)
      {
         currentIndex = i;
         break;
      }
   }

   if (currentIndex != INDEX_NONE)
   {
      NameListView->SetSelection(FilteredCurveTypes[currentIndex], ESelectInfo::Direct);
   }
}

void FOSEInterpModeCustomization::RefreshCurveTypesList()
{
   FStringView queryString = FStringView(FilterText).TrimStartAndEnd();

   FilteredCurveTypes.Reset();

   auto matchesFilterType = [](EFilterType filterType, EOSEInterpMode interpMode, FStringView nameString) -> bool
   {
      if (filterType == EFilterType::None)
      {
         return true;
      }

      const bool isEasing = nameString.Contains(TEXT("Ease"));
      switch (filterType)
      {
      case EFilterType::Math:
         return !isEasing;
      case EFilterType::Easing:
         return isEasing;
      case EFilterType::EaseIn:
         return isEasing && nameString.EndsWith(TEXT("EaseIn"));
      case EFilterType::EaseOut:
         return isEasing && nameString.EndsWith(TEXT("EaseOut"));
      case EFilterType::EaseInOut:
         return isEasing && nameString.EndsWith(TEXT("EaseInOut"));
      default:
         break;
      }

      return true;
   };

   UEnum* enumType = StaticEnum<EOSEInterpMode>();
   check(enumType != nullptr);
   FilteredCurveTypes.Reset();
   for (int32 i = 0; i < enumType->NumEnums(); i++)
   {
      const int32 enumValue = enumType->GetValueByIndex(i);
      if (enumValue == enumType->GetMaxEnumValue())
      {
         continue;
      }

      const EOSEInterpMode interpMode = static_cast<EOSEInterpMode>(enumValue);
      const FString nameString = enumType->GetNameStringByIndex(i);

      if (FilterByType != EFilterType::None && !matchesFilterType(FilterByType, interpMode, nameString))
      {
         continue;
      }

      if (!queryString.IsEmpty() && !nameString.Contains(queryString))
      {
         continue;
      }

      TSharedPtr<FOSEInterpCurveType> curveType = MakeShared<FOSEInterpCurveType>();
      curveType->Mode = interpMode;
      curveType->Name = nameString;
      curveType->Label = enumType->GetDisplayNameTextByIndex(i);
      FilteredCurveTypes.Add(curveType);
   }

   if (NameListView)
   {
      NameListView->RequestListRefresh();
      UpdateSelection();
   }
}

#undef LOCTEXT_NAMESPACE
