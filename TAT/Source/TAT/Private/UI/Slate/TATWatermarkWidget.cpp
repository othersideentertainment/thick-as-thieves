// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Slate/TATWatermarkWidget.h"

// ue
#include "GameFramework/GameUserSettings.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// tat
#include "Developer/TATProjectSettings.h"

SLATE_IMPLEMENT_WIDGET(STATWatermarkWidget)
void STATWatermarkWidget::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{}

void STATWatermarkWidget::Construct(const STATWatermarkWidget::FArguments& args)
{
   auto userInfos = SNew(SVerticalBox);
   userInfos->SetRenderTransformPivot(FVector2D(0.5, 0.5));
   userInfos->SetRenderTransform(FSlateRenderTransform(FTransform2D(FQuat2D(-0.79)))); // radians - 45 deg

   for(int i = 0; i < 20; i++)
   {
   userInfos->AddSlot()
   [
      SNew(STATUserInfoWidget)
      .Username(args._Username)
      .UniqueId(args._UniqueId)
   ];
   }

   ChildSlot
   .HAlign(HAlign_Fill)
   [
      SNew(SOverlay)
      + SOverlay::Slot()
      .HAlign(HAlign_Fill)
      .Padding(0, 20, 0, 20)
      [
         userInfos
      ]
   ];
}

SLATE_IMPLEMENT_WIDGET(STATUserInfoWidget)
void STATUserInfoWidget::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void STATUserInfoWidget::Construct(const FArguments& args)
{
   FSlateFontInfo fontInfo = UTATProjectSettings::GetTATSettings()->WatermarkFontInfo;
   FSlateColor color = FLinearColor(1.0f, 1.0f, 1.0f, 0.05f);

   ChildSlot
   .HAlign(HAlign_Fill)
   .Padding(0, 20, 0, 20)
   [
      SNew(SHorizontalBox)
      // spacer
      + SHorizontalBox::Slot()
      .FillWidth(1)
      [
         SNew(SSpacer)
      ]

      // username
      + SHorizontalBox::Slot()
      .Padding(10, 0, 10, 0)
      .HAlign(HAlign_Center)
      .VAlign(VAlign_Center)
      .AutoWidth()
      [
         SNew(STextBlock)
         .Text(FText::FromString("Username: "))
         .Font(fontInfo)
         .ColorAndOpacity(color)
      ]
      + SHorizontalBox::Slot()
      .Padding(10, 0, 10, 0)
      .HAlign(HAlign_Center)
      .VAlign(VAlign_Center)
      .AutoWidth()
      [
         SNew(STextBlock)
         .Text(args._Username)
         .Font(fontInfo)
         .ColorAndOpacity(color)
      ]
          
      // spacer
      + SHorizontalBox::Slot()
      .FillWidth(1)
      [
         SNew(SSpacer)
      ]

      // user id
      + SHorizontalBox::Slot()
      .Padding(10, 0, 10, 0)
      .HAlign(HAlign_Center)
      .VAlign(VAlign_Center)
      .AutoWidth()
      [
         SNew(STextBlock)
         .Text(FText::FromString("User ID: "))
         .Font(fontInfo)
         .ColorAndOpacity(color)
      ]
      + SHorizontalBox::Slot()
      .Padding(10, 0, 10, 0)
      .HAlign(HAlign_Center)
      .VAlign(VAlign_Center)
      .AutoWidth()
      [
         SNew(STextBlock)
         .Text(args._UniqueId)
         .Font(fontInfo)
         .ColorAndOpacity(color)
      ]

      // spacer
      + SHorizontalBox::Slot()
      [
         SNew(SSpacer)
      ]
   ];
}
