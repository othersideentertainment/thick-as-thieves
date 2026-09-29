// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

class FTATClueSetEntryCustomization : public IPropertyTypeCustomization
{
public:
   static TSharedRef<IPropertyTypeCustomization> MakeInstance()
   {
      return MakeShareable(new FTATClueSetEntryCustomization);
   }

   virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils) override;
   virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils) override;
};
