// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATClueCustomization.h"

// tat
#include "Variation/Clues/TATClueSet.h"

// ue5
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "IDetailChildrenBuilder.h"
#include "InstancedStructDetails.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "SInstancedStructPicker.h"

void FTATClueSetEntryCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow,
                                                    IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedPtr<IPropertyHandle> enabledProperty = inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATClueSetEntry, Enabled));
   TSharedPtr<IPropertyHandle> clueProperty = inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATClueSetEntry, Clue));

   // partially doing what FInstancedStructDetails does, but with an extra checkbox
   // Setup in the header row so that we still get the TArray dropdown
   headerRow
      .ShouldAutoExpand(true)
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      .MinDesiredWidth(260)
      [
         SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
               enabledProperty->CreatePropertyValueWidget()
            ]
            + SHorizontalBox::Slot()
            .MinWidth(250)
            [
               SNew(SInstancedStructPicker, clueProperty, structCustomizationUtils.GetPropertyUtilities())
            ]
      ];
   // This avoids making duplicate reset boxes
   inStructPropertyHandle->MarkResetToDefaultCustomized();
}

void FTATClueSetEntryCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> inStructPropertyHandle,
   class IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   // partially doing what FInstancedStructDetails does
   TSharedPtr<IPropertyHandle> clueProperty = inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATClueSetEntry, Clue));
   TSharedRef<FInstancedStructDataDetails> dataDetails = MakeShared<FInstancedStructDataDetails>(clueProperty);
   structBuilder.AddCustomBuilder(dataDetails);
}
