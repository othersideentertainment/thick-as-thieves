// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Thiefsign/TATThiefsignMaterialParamKeyCustomization.h"

// ue
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"

void FTATThiefsignMaterialParamKeyCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> inStructPropertyHandle, FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   // Gets ParamName
   TSharedPtr<IPropertyHandle> paramName = inStructPropertyHandle->GetChildHandle(0);

   // Gets Perspective
   TSharedPtr<IPropertyHandle> perspective = inStructPropertyHandle->GetChildHandle(1);

   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      [
         SNew(SHorizontalBox)
            + SHorizontalBox::Slot() // ParamName variable
            .FillWidth(1.0f)
            [
               paramName->CreatePropertyValueWidget()
            ]
            + SHorizontalBox::Slot() // space padding
            .AutoWidth()
            [
               SNew(SSpacer)
               .Size(10.f)
            ]
            + SHorizontalBox::Slot() // "Enabled for" display text
            .AutoWidth()
            [
               SNew(STextBlock)
               .Text(FText::FromString(FString(TEXT("Enabled for "))))
               .ToolTipText(FText::FromString(FString(TEXT("Should this param apply to first person instances of this VFX"))))
               .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            + SHorizontalBox::Slot() // space padding
            .AutoWidth()
            [
               SNew(SSpacer)
               .Size(10.f)
            ]
            + SHorizontalBox::Slot() // Perspective variable
            .AutoWidth()
            [
               perspective->CreatePropertyValueWidget()
            ]
      ];
}

void FTATThiefsignMaterialParamKeyCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> inStructPropertyHandle, IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
}
