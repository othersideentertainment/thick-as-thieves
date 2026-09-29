// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "IDetailCustomization.h"

	 
class FTATTimedSwitchDetailsCustomization : public IDetailCustomization
{
public:
   static TSharedRef<IDetailCustomization> MakeInstance() {  return MakeShareable(new FTATTimedSwitchDetailsCustomization()); }
   
   virtual void CustomizeDetails(IDetailLayoutBuilder& detailBuilder) override;
};
