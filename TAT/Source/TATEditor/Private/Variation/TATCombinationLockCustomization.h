// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "IPropertyTypeCustomization.h"

struct FEventReply;
class IPropertyHandle;

class FTATLockCombinationNameCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance()
   {
      return MakeShareable(new FTATLockCombinationNameCustomization);
   }

   // IPropertyTypeCustomization interface
   virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils) override {}

   TSharedPtr<class IPropertyHandle> PropertyHandle;
};

class FTATLockCombinationNameRefCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance()
   {
      return MakeShareable(new FTATLockCombinationNameRefCustomization);
   }

   // IPropertyTypeCustomization interface
   virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils) override {}

   TSharedPtr<class IPropertyHandle> PropertyHandle;
   TWeakObjectPtr<const class UTATQuestGraphBase> OwningQuestGraph;
};
