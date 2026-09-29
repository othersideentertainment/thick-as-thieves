// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/Electrical/TATElectricalDeviceComponent.h"

// tat
#include "Interactables/Electrical/TATPowerSource.h"

// ue
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATElectricalDeviceComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATElectricalDeviceComponent, Log, All);

void UTATElectricalDeviceComponent::BeginPlay()
{
   Super::BeginPlay();
   _hasFinishedBeginPlay  = false;
   if (_powerSource)
   {
      _powerSource->OnPowerStateChanged.AddUObject(this, &ThisClass::_OnPowerStateChanged);
      if (_powerSource->IsActorInitialized())
      {
         _OnPowerStateChanged(_powerSource->IsPowered());
      }
   }
   _hasFinishedBeginPlay  = true;
   UE_CLOG(_requirePowerSource && _powerSource == nullptr, LogTATElectricalDeviceComponent, Error, TEXT("[%s] Power source not assigned"), *GetOwner()->GetName());
}

#if WITH_EDITOR
void UTATElectricalDeviceComponent::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (_requirePowerSource && !IsValid(_powerSource))
      {
         // NOTE: disabled until we finish migrating all _powerSource overrides via MigrateProperties() / TATFixupElectricalDeviceCommandletr
         //FMessageLog("MapCheck")
         //.Warning()
         //->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(GetOwner()))))
         //->AddToken(FTextToken::Create(FText::Format(INVTEXT("TATElectricalDeviceComponent attached to {0} does not have an assigned power source. (and is set to require power source)"),
         //            FText::FromString(GetNameSafe(GetOwner())))));
      }
   }
}

bool UTATElectricalDeviceComponent::MigrateProperties(TObjectPtr<ATATPowerSource> powerSource)
{
   const bool changed = powerSource != _powerSource;
   _powerSource = powerSource;
   UE_CLOG(changed, LogTATElectricalDeviceComponent, Warning, TEXT("[%s] Migrated properties to component!"), *GetOwner()->GetName());
   return changed;
}
#endif // WITH_EDITOR

bool UTATElectricalDeviceComponent::IsPowered() const
{
   return _powerSource && _powerSource->IsPowered();
}

bool UTATElectricalDeviceComponent::GetEffectivePoweredState() const
{
   return _requirePowerSource ? IsPowered() : true;
}

void UTATElectricalDeviceComponent::TEMP_MigrateFrom(UTATElectricalDeviceComponent* other)
{
   if(other)
   {
      _powerSource = other->_powerSource;
   }
}

void UTATElectricalDeviceComponent::_OnPowerStateChanged(bool bIsOn)
{
   OnPoweredChanged.Broadcast(bIsOn);
}
