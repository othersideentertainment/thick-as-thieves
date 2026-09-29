// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class FShuffledToggleBindingRefCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance()
   {
      return MakeShareable(new FShuffledToggleBindingRefCustomization);
   }

   // IPropertyTypeCustomization interface
   virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}
};

class FShuffledToggleTargetRefCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance()
   {
      return MakeShareable(new FShuffledToggleTargetRefCustomization);
   }

   // IPropertyTypeCustomization interface
   virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}
};
