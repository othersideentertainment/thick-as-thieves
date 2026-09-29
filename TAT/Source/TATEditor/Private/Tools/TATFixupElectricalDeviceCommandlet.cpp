// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATFixupElectricalDeviceCommandlet.h"

// tat
#include "Interactables/Electrical/TATElectricalDevice.h"
#include "Interactables/Electrical/TATElectricalDeviceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFixupElectricalDeviceCommandlet)

bool UTATFixupElectricalDeviceCommandlet::FixupMapActor(AActor* actor)
{
   if (ATATElectricalDevice* electricalDevice = Cast<ATATElectricalDevice>(actor))
   {
      return electricalDevice->MigrateProperties();
   }
   return false;
}
