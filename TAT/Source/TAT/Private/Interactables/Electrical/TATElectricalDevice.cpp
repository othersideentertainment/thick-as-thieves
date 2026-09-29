// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "Interactables/Electrical/TATElectricalDevice.h"

// tat
#include "Interactables/Electrical/TATElectricalDeviceComponent.h"
#include "Interactables/Electrical/TATPowerSource.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATElectricalDevice)

DEFINE_LOG_CATEGORY(LogTATElectricalDevice)

void ATATElectricalDevice::BeginPlay()
{
   Super::BeginPlay();

   if (_powerSource)
   {
      _powerSource->OnPowerStateChanged.AddUObject(this, &ATATElectricalDevice::_OnPowerStateChanged);
      if (_powerSource->IsActorInitialized())
      {
         _OnPowerStateChanged(_powerSource->IsPowered());
      }
   }
   UE_CLOG(_requirePowerSource && _powerSource == nullptr, LogTATElectricalDevice, Error, TEXT("Power source not assigned"));
}

bool ATATElectricalDevice::IsPowered() const
{
   return _powerSource && _powerSource->IsPowered();
}

#if WITH_EDITOR
void ATATElectricalDevice::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (_requirePowerSource && !IsValid(_powerSource))
      {
         FMessageLog("MapCheck")
         .Warning()
         ->AddToken(FUObjectToken::Create(this, FText::FromString(GetActorNameOrLabel())))
         ->AddToken(FTextToken::Create(FText::Format(INVTEXT("{0} does not have an assigned power source. (and is set to require power source)"),
                     FText::FromString(GetActorLabel()))));
      }
   }
}
bool ATATElectricalDevice::MigrateProperties()
{
   if (UTATElectricalDeviceComponent* electricalDeviceComponent = FindComponentByClass<UTATElectricalDeviceComponent>())
   {
      return electricalDeviceComponent->MigrateProperties(_powerSource);
   }
   return false;
}
#endif // WITH_EDITOR

void ATATElectricalDevice::_OnPowerStateChanged(const bool bIsOn)
{
   OnPowerToggled(bIsOn);
}
