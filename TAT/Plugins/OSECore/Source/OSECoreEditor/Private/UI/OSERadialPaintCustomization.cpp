// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSERadialPaintCustomization.h"
#include "UI/OSERadialPaintLibrary.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IPropertyTypeCustomization.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OSELineParamsCustomization"

//---------------------------------------------------------------------------------------
// SOSELinePreviewBlock
//---------------------------------------------------------------------------------------

SLATE_IMPLEMENT_WIDGET(SOSELinePreviewBlock)
void SOSELinePreviewBlock::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, Thickness, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, Color, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, BackgroundBrush, EInvalidateWidgetReason::Paint);
   SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION(AttributeInitializer, Size, EInvalidateWidgetReason::Layout);
}

SOSELinePreviewBlock::SOSELinePreviewBlock()
   : Thickness(*this, 1.0f)
   , Color(*this, FLinearColor::White)
   , BackgroundBrush(*this, nullptr)
   , Size(*this, FVector2D(16, 16))
{
}

void SOSELinePreviewBlock::Construct(const FArguments& InArgs)
{
   Thickness.Assign(*this, InArgs._Thickness);
   Color.Assign(*this, InArgs._Color);
   BackgroundBrush.Assign(*this, InArgs._BackgroundBrush);
   Size.Assign(*this, InArgs._Size);
   MouseButtonDownHandler = InArgs._OnMouseButtonDown;
}

int32 SOSELinePreviewBlock::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
   FOSERadialPaintContext ctx{ LayerId, AllottedGeometry, MyCullingRect, OutDrawElements, InWidgetStyle, bParentEnabled };
   const FVector2f widgetSize = AllottedGeometry.GetLocalSize();
   const FSlateBrush* slateBrush = BackgroundBrush.Get();
   if (slateBrush == nullptr)
   {
      slateBrush = FAppStyle::GetBrush("ColorPicker.RoundedAlphaBackground");
   }
   if (slateBrush != nullptr)
   {
      ctx.DrawRect(FVector2f::ZeroVector, widgetSize, *slateBrush);
   }
   FOSELineParams line{ Thickness.Get(), Color.Get() };
   if (!line)
   {
      return LayerId;
   }
   const float maxLineLength = widgetSize.Length();
   const float lineOffset = (maxLineLength < 6.0f) ? 0.0f : (maxLineLength * 0.1f);
   line.Thickness = FMath::Clamp(line.Thickness, 0.0f, maxLineLength * 0.5f);
   ctx.DrawLine(FVector2f(lineOffset), widgetSize - FVector2f(lineOffset), line);
   return LayerId;
}

//---------------------------------------------------------------------------------------
// FOSELineParamsCustomization
//---------------------------------------------------------------------------------------

TSharedRef<IPropertyTypeCustomization> FOSELineParamsCustomization::MakeInstance()
{
   return MakeShareable(new FOSELineParamsCustomization);
}

void FOSELineParamsCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& InHeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
   StructPropertyHandle = InStructPropertyHandle;
   check(StructPropertyHandle.IsValid());

   ThicknessStructPropertyHandle = StructPropertyHandle->GetChildHandle(TEXT("Thickness"));
   check(ThicknessStructPropertyHandle.IsValid());

   ColorStructPropertyHandle = StructPropertyHandle->GetChildHandle(TEXT("Color"));
   check(StructPropertyHandle.IsValid());

   TSharedPtr<IPropertyUtilities> PropertyUtils = StructCustomizationUtils.GetPropertyUtilities();
   bDontUpdateWhileEditing = PropertyUtils.IsValid() ? PropertyUtils->DontUpdateValueWhileEditing() : false;

   InHeaderRow.NameContent()
   [
      StructPropertyHandle->CreatePropertyNameWidget()
   ]
   .ValueContent()
   .MinDesiredWidth(251.0f)
   .MaxDesiredWidth(251.0f)
   [
      SNew(SHorizontalBox)
      +SHorizontalBox::Slot()
      .Padding(FMargin(0, 2.0f, 4.0f, 2.0f))
      .VAlign(VAlign_Center)
      .FillWidth(0.1f)
      [
         SNew(SBorder)
         .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
         .VAlign(VAlign_Center)
         [
            SNew(SOSELinePreviewBlock)
            .Thickness_Lambda([this]() -> float { return GetCurrentThickness().Get(0.0f); })
            .Color_Lambda([this]() -> FLinearColor { return GetCurrentColor().Get(FLinearColor::Transparent); })
            .Size(FVector2D(20.0f, 20.0f))
            .BackgroundBrush(FAppStyle::GetBrush("Menu.Background"))
         ]
      ]
      +SHorizontalBox::Slot()
      .Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
      .VAlign(VAlign_Center)
      .FillWidth(0.45f)
      [
         ThicknessStructPropertyHandle->CreatePropertyValueWidget()
      ]
      +SHorizontalBox::Slot()
      .VAlign(VAlign_Center)
      .FillWidth(0.45f)
      [
         MakeColorWidget()
      ]
   ];
}

void FOSELineParamsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

TSharedRef<SWidget> FOSELineParamsCustomization::MakeColorWidget()
{
   check(ColorStructPropertyHandle.IsValid());
   TWeakPtr<IPropertyHandle> structWeakHandlePtr = ColorStructPropertyHandle;
   return
      SNew(SBox)
      .Padding(FMargin(0, 0, 4.0f, 0.0f))
      .VAlign(VAlign_Center)
      [
         SAssignNew(ColorWidgetBackgroundBorder, SBorder)
         .Padding(1)
         .BorderImage(FAppStyle::Get().GetBrush("ColorPicker.RoundedSolidBackground"))
         .BorderBackgroundColor(this, &FOSELineParamsCustomization::GetColorWidgetBorderColor)
         .VAlign(VAlign_Center)
         [
            SNew(SOverlay)
            +SOverlay::Slot()
            .VAlign(VAlign_Center)
            [
               SAssignNew(ColorPickerParentWidget, SColorBlock)
               .AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
               .Color(this, &FOSELineParamsCustomization::OnGetColorForColorBlock)
               .ShowBackgroundForAlpha(true)
               .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
               .OnMouseButtonDown(this, &FOSELineParamsCustomization::OnMouseButtonDownColorBlock)
               .Size(FVector2D(70.0f, 20.0f))
               .CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
               .IsEnabled(this, &FOSELineParamsCustomization::IsValueEnabled, structWeakHandlePtr)
            ]
            +SOverlay::Slot()
            .VAlign(VAlign_Center)
            [
               SNew(SBorder)
               .Visibility(this, &FOSELineParamsCustomization::GetMultipleValuesTextVisibility)
               .BorderImage(FAppStyle::Get().GetBrush("ColorPicker.MultipleValuesBackground"))
               .VAlign(VAlign_Center)
               .ForegroundColor(FAppStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox").ForegroundColor)
               .Padding(FMargin(12.0f, 2.0f))
               [
                  SNew(STextBlock)
                  .Text(NSLOCTEXT("PropertyEditor", "MultipleValues", "Multiple Values"))
                  .Font(IDetailLayoutBuilder::GetDetailFont())
               ]
            ]
         ]
      ];
}

FSlateColor FOSELineParamsCustomization::GetColorWidgetBorderColor() const
{
   static const FSlateColor hoveredColor = FAppStyle::Get().GetSlateColor("Colors.Hover");
   static const FSlateColor defaultColor = FAppStyle::Get().GetSlateColor("Colors.InputOutline");
   return (ColorWidgetBackgroundBorder && ColorWidgetBackgroundBorder->IsHovered()) ? hoveredColor : defaultColor;
}

FLinearColor FOSELineParamsCustomization::OnGetColorForColorBlock() const
{
   return GetCurrentColor().Get(FLinearColor::White);
}

FReply FOSELineParamsCustomization::OnMouseButtonDownColorBlock(const struct FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
   if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
   {
      return FReply::Unhandled();
   }

   bool CanShowColorPicker = true;
   if (ColorStructPropertyHandle.IsValid() && ColorStructPropertyHandle->GetProperty() != nullptr)
   {
      CanShowColorPicker = !ColorStructPropertyHandle->IsEditConst();
   }

   if (CanShowColorPicker)
   {
      OpenNewColorPickerDialog();
   }

   return FReply::Handled();
}

bool FOSELineParamsCustomization::IsValueEnabled(TWeakPtr<IPropertyHandle> WeakHandlePtr) const
{
   if (WeakHandlePtr.IsValid())
   {
      return !WeakHandlePtr.Pin()->IsEditConst();
   }
   return false;
}

EVisibility FOSELineParamsCustomization::GetMultipleValuesTextVisibility() const
{
   FPropertyAccess::Result ValueResult = FPropertyAccess::Fail;
   GetCurrentColor(&ValueResult);
   return (ValueResult == FPropertyAccess::MultipleValues) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

namespace OSERadialHelpers
{
   template<typename T>
   TOptional<T> GetPropValue(const TSharedPtr<IPropertyHandle>& prop, FPropertyAccess::Result* outAccessResult)
   {
      static constexpr bool canUseGetValue = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, FString>
         || std::is_same_v<T, FText> || std::is_same_v<T, FName> || std::is_same_v<T, FVector>;
      if (prop)
      {
         T value{};
         FPropertyAccess::Result accessResult{};
         bool initSuccess = false;
         if constexpr (canUseGetValue)
         {
            accessResult = prop->GetValue(value);
            initSuccess = true;
         }
         else
         {
            FString stringValue;
            accessResult = prop->GetValueAsFormattedString(stringValue);
            if (accessResult == FPropertyAccess::Success)
            {
               // assume T has a method like "InitFromString(FString)->bool" method (FLinearColor, FRotator, generally has this)
               initSuccess = value.InitFromString(stringValue);
            }
         }

         if (outAccessResult != nullptr)
         {
            *outAccessResult = accessResult;
         }
         if (accessResult == FPropertyAccess::Success && initSuccess)
         {
            return value;
         }
      }
      return NullOpt;
   }
}

TOptional<float> FOSELineParamsCustomization::GetCurrentThickness(FPropertyAccess::Result* OutAccessResult) const
{
   return OSERadialHelpers::GetPropValue<float>(ThicknessStructPropertyHandle, OutAccessResult);
}

TOptional<FLinearColor> FOSELineParamsCustomization::GetCurrentColor(FPropertyAccess::Result* OutAccessResult) const
{
   return OSERadialHelpers::GetPropValue<FLinearColor>(ColorStructPropertyHandle, OutAccessResult);
}

void FOSELineParamsCustomization::OpenNewColorPickerDialog()
{
   check(StructPropertyHandle.IsValid());
   check(ColorStructPropertyHandle.IsValid());

   GEditor->BeginTransaction(FText::Format(LOCTEXT("SetColorProperty", "Edit Color {0}"), StructPropertyHandle->GetPropertyDisplayName()));

   const FLinearColor InitialColor = GetCurrentColor().Get(FLinearColor::White);

   const bool bRefreshOnlyOnOk = bDontUpdateWhileEditing || ColorStructPropertyHandle->HasMetaData("DontUpdateWhileEditing");
   const bool bOnlyRefreshOnMouseUp = ColorStructPropertyHandle->HasMetaData("OnlyUpdateOnInteractionEnd");

   FColorPickerArgs PickerArgs;
   {
      PickerArgs.bUseAlpha = true;
      PickerArgs.bOnlyRefreshOnMouseUp = bOnlyRefreshOnMouseUp;
      PickerArgs.bOnlyRefreshOnOk = bRefreshOnlyOnOk;
      PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
      PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &FOSELineParamsCustomization::OnSetColorFromColorPicker);
      PickerArgs.OnColorPickerCancelled = FOnColorPickerCancelled::CreateSP(this, &FOSELineParamsCustomization::OnColorPickerCancelled);
      PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateSP(this, &FOSELineParamsCustomization::OnColorPickerWindowClosed);
      PickerArgs.OnInteractivePickBegin = FSimpleDelegate::CreateSP(this, &FOSELineParamsCustomization::OnColorPickerInteractiveBegin);
      PickerArgs.OnInteractivePickEnd = FSimpleDelegate::CreateSP(this, &FOSELineParamsCustomization::OnColorPickerInteractiveEnd);
      PickerArgs.InitialColor = InitialColor;
      PickerArgs.ParentWidget = ColorPickerParentWidget;
      PickerArgs.OptionalOwningDetailsView = ColorPickerParentWidget;
      FWidgetPath ParentWidgetPath;
      if (FSlateApplication::Get().FindPathToWidget(ColorPickerParentWidget.ToSharedRef(), ParentWidgetPath))
      {
         PickerArgs.bOpenAsMenu = FSlateApplication::Get().FindMenuInWidgetPath(ParentWidgetPath).IsValid();
      }
   }

   OpenColorPicker(PickerArgs);
}

void FOSELineParamsCustomization::OnSetColorFromColorPicker(FLinearColor NewColor)
{
   check(ColorStructPropertyHandle.IsValid());
   LastPickerColorString = NewColor.ToString();
   EPropertyValueSetFlags::Type PropertyFlags = EPropertyValueSetFlags::NotTransactable;
   PropertyFlags |= bIsInteractive ? EPropertyValueSetFlags::InteractiveChange : 0;
   ColorStructPropertyHandle->SetValueFromFormattedString(LastPickerColorString, PropertyFlags);
   ColorStructPropertyHandle->NotifyFinishedChangingProperties();
}

void FOSELineParamsCustomization::OnColorPickerCancelled(FLinearColor OriginalColor)
{
   LastPickerColorString.Reset();
   GEditor->CancelTransaction(0);
}

void FOSELineParamsCustomization::OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window)
{
   // Transact only at the end to avoid opening a lingering transaction. Reset value before transacting.
   if (!LastPickerColorString.IsEmpty())
   {
      //@TODO: Not using reset & apply instant scoped transition pattern since certain property nodes are
      // returning nullptr when finding objects to modify on reset, so we can't reset correctly for those.
      {
         // FScopedTransaction Transaction(FText::Format(LOCTEXT("SetColorProperty", "Edit {0}"), StructPropertyHandle->GetPropertyDisplayName()));
         check(ColorStructPropertyHandle.IsValid());
         ColorStructPropertyHandle->SetValueFromFormattedString(LastPickerColorString);
      }
   }

   GEditor->EndTransaction();
}

void FOSELineParamsCustomization::OnColorPickerInteractiveBegin()
{
   bIsInteractive = true;
}

void FOSELineParamsCustomization::OnColorPickerInteractiveEnd()
{
   bIsInteractive = false;
}

#undef LOCTEXT_NAMESPACE
