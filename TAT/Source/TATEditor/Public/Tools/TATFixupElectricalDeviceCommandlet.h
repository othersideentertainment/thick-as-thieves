// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Tools/OSEFixupMapActorsCommandlet.h"

#include "TATFixupElectricalDeviceCommandlet.generated.h"

UCLASS()
class TATEDITOR_API UTATFixupElectricalDeviceCommandlet : public UOSEFixupMapActorsCommandlet
{
   GENERATED_BODY()
   
protected:
   // from UOSECommandletBase
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("TATFixupElectricalDeviceCommandlet"); };

   // from UOSEFixupMapActorsCommandlet
   virtual bool FixupMapActor(AActor* actor) override;
};
