// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

class FTATPatrolPointOverrideCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance()
   {
      return MakeShareable(new FTATPatrolPointOverrideCustomization);
   }


   /** IPropertyTypeCustomization interface */
   virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils) override;

private:
   FReply OnSnapSelected(TSharedRef<IPropertyHandle> propertyHandle);
   FReply OnFocusSelected(TSharedRef<IPropertyHandle> propertyHandle);

};
