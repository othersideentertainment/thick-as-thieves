// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATShuffledToggleCustomization.h"

// tat
#include "Interactables/TATShuffledToggleBindings.h"

// ue5
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"


namespace ShuffledToggleHelpers
{
   static TSharedPtr<IPropertyHandle> FindShuffleSetProperty(TSharedRef<class IPropertyHandle> property)
   {
      for (TSharedPtr<IPropertyHandle> parent = property->GetParentHandle(); parent.IsValid(); parent = parent->GetParentHandle())
      {
         // If this ever differs, can read from property metadata instead
         static FName kShuffleSetName = GET_MEMBER_NAME_CHECKED(FTATToggleResolver_ShuffledBinding, ShuffleSet);
         TSharedPtr<IPropertyHandle> shuffleSetHandle = parent->GetChildHandle(kShuffleSetName, false);
         if (shuffleSetHandle.IsValid())
         {
            return shuffleSetHandle;
         }
      }

      return nullptr;
   }
}

void FShuffledToggleBindingRefCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedPtr<IPropertyHandle> inner = inStructPropertyHandle->GetChildHandle(0);
   
   TSharedPtr<IPropertyHandle> shuffleSetHandle = ShuffledToggleHelpers::FindShuffleSetProperty(inStructPropertyHandle);
   FPropertyComboBoxArgs comboBoxArgs(inner, FOnGetPropertyComboBoxStrings::CreateLambda([shuffleSetHandle](TArray<TSharedPtr<FString>>& outStrings, TArray<TSharedPtr<SToolTip>>& , TArray<bool>&) {
      if (!shuffleSetHandle.IsValid()) return;

      UObject* setPropObject = nullptr;
      shuffleSetHandle->GetValue(setPropObject);

      if (const UTATShuffledToggleSet* shuffleSet = Cast<UTATShuffledToggleSet>(setPropObject))
      {
         for (const FTATShuffledToggleBinding& binding : shuffleSet->Bindings)
         {
            outStrings.Add(MakeShared<FString>(binding.Identifier.ToString()));
         }
      }
   }));
   
   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      [
         PropertyCustomizationHelpers::MakePropertyComboBox(comboBoxArgs)
      ];
}

void FShuffledToggleTargetRefCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedPtr<IPropertyHandle> inner = inStructPropertyHandle->GetChildHandle(0);

   TSharedPtr<IPropertyHandle> shuffleSetHandle = ShuffledToggleHelpers::FindShuffleSetProperty(inStructPropertyHandle);
   FPropertyComboBoxArgs comboBoxArgs(inner, FOnGetPropertyComboBoxStrings::CreateLambda([shuffleSetHandle](TArray<TSharedPtr<FString>>& outStrings, TArray<TSharedPtr<SToolTip>>&, TArray<bool>&) {
      if (!shuffleSetHandle.IsValid()) return;

      UObject* setPropObject = nullptr;
      shuffleSetHandle->GetValue(setPropObject);

      if (const UTATShuffledToggleSet* shuffleSet = Cast<UTATShuffledToggleSet>(setPropObject))
      {
         for (const FTATShuffledToggleTarget& target : shuffleSet->Targets)
         {
            outStrings.Add(MakeShared<FString>(target.Identifier.ToString()));
         }
      }
      }));

   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      [
         PropertyCustomizationHelpers::MakePropertyComboBox(comboBoxArgs)
      ];
}
