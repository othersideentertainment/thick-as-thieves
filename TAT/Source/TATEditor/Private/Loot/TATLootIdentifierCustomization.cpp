// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Loot/TATLootIdentifierCustomization.h"

// ue5
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"


void FTATLootIdentifierCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedPtr<IPropertyHandle> tagProperty = inStructPropertyHandle->GetChildHandle(0);

   // Just show the inner tag
   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      [
         tagProperty->CreatePropertyValueWidgetWithCustomization(nullptr)
      ];
}
