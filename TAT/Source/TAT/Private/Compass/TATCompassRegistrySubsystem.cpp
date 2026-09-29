// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Compass/TATCompassRegistrySubsystem.h"

// tat
#include "Compass/TATGenericIndicator.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCompassRegistrySubsystem)

bool UTATCompassRegistrySubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   const UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_DedicatedServer);
}

bool UTATCompassRegistrySubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::PIE || worldType == EWorldType::Game;
}

void UTATCompassRegistrySubsystem::RegisterIndicator(ATATGenericIndicator* indicator)
{
   check(IsValid(indicator));
   if (!_registeredIndicators.Contains(indicator))
   {
      _registeredIndicators.Add(indicator);
      OnIndicatorAdded.Broadcast(indicator);
   }
}

void UTATCompassRegistrySubsystem::UnregisterIndicator(ATATGenericIndicator* indicator)
{
   // NB: _not_ checking IsValid, since it could be garbage being unregistered
   check(indicator);
   if (_registeredIndicators.Contains(indicator))
   {
      _registeredIndicators.Remove(indicator);
      OnIndicatorRemoved.Broadcast(indicator);
   }
}
