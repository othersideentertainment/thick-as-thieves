// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class FDetailWidgetRow;
class IPropertyHandle;
class IPropertyTypeCustomizationUtils;

class OSECOREEDITOR_API SOSELinePreviewBlock : public SLeafWidget
{
   SLATE_DECLARE_WIDGET(SOSELinePreviewBlock, SLeafWidget)

public:
   SLATE_BEGIN_ARGS(SOSELinePreviewBlock)
      : _Thickness(1.0f)
      , _Color(FLinearColor::White)
      , _BackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.AlphaBackground"))
      , _Size(FVector2D(16, 16))
   {}
      SLATE_ATTRIBUTE(float, Thickness)
      SLATE_ATTRIBUTE(FLinearColor, Color)
      SLATE_ATTRIBUTE(const FSlateBrush*, BackgroundBrush)
      SLATE_ATTRIBUTE(FVector2D, Size)
      SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
   SLATE_END_ARGS()

public:
   SOSELinePreviewBlock();
   void Construct(const FArguments& InArgs);

private:
   // SWidget overrides
   virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
   virtual FVector2D ComputeDesiredSize(float) const override { return Size.Get(); }
   virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
   {
      return MouseButtonDownHandler.IsBound() ? MouseButtonDownHandler.Execute(MyGeometry, MouseEvent) : FReply::Unhandled();
   }

   TSlateAttribute<float> Thickness;
   TSlateAttribute<FLinearColor> Color;
   TSlateAttribute<const FSlateBrush*> BackgroundBrush;
   TSlateAttribute<FVector2D> Size;
   FPointerEventHandler MouseButtonDownHandler;
};

class OSECOREEDITOR_API FOSELineParamsCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance();

protected:
   // IPropertyTypeCustomization interface
   virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

protected:
   // Constructs a widget for editing a linear color property
   TSharedRef<SWidget> MakeColorWidget();

   // Widget callbacks
   FSlateColor GetColorWidgetBorderColor() const;
   FLinearColor OnGetColorForColorBlock() const;
   FReply OnMouseButtonDownColorBlock(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
   bool IsValueEnabled(TWeakPtr<IPropertyHandle> WeakHandlePtr) const;
   EVisibility GetMultipleValuesTextVisibility() const;

   // Gets the current struct values
   TOptional<float> GetCurrentThickness(FPropertyAccess::Result* OutAccessResult = nullptr) const;
   TOptional<FLinearColor> GetCurrentColor(FPropertyAccess::Result* OutAccessResult = nullptr) const;

   // Opens a new dialog window with a color picker
   void OpenNewColorPickerDialog();

   // Callbacks for the color picker dialog window
   void OnSetColorFromColorPicker(FLinearColor NewColor);
   void OnColorPickerCancelled(FLinearColor OriginalColor);
   void OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window);
   void OnColorPickerInteractiveBegin();
   void OnColorPickerInteractiveEnd();

protected:
   /// Struct handle
   TSharedPtr<IPropertyHandle> StructPropertyHandle;

   /// Inner color struct handle
   TSharedPtr<IPropertyHandle> ThicknessStructPropertyHandle;
   TSharedPtr<IPropertyHandle> ColorStructPropertyHandle;

   /// Cached widget for the color picker to use as a parent
   TSharedPtr<SWidget> ColorPickerParentWidget;

   TSharedPtr<SWidget> ColorWidgetBackgroundBorder;

   /// True if the user is performing an interactive color change
   bool bIsInteractive = false;

   /// Last color set from color picker as string
   FString LastPickerColorString;

   /// The value won't be updated while editing
   bool bDontUpdateWhileEditing = false;
};
