// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/InteractableInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractableInterface)

// Add default functionality here for any IInteractableInterface functions that are not pure virtual.

namespace InteractCVars
{
   static float InstantInteractThreshold = 0.15f;
   FAutoConsoleVariableRef CVarInstantInteractThreshold(
      TEXT("OSE.Interact.InstantInteractThreshold"),
      InstantInteractThreshold,
      TEXT("Threshold in seconds below which the interaction is considered to be a press rather than a hold"),
      ECVF_Default);
}

bool FInteractEndContext::IsProbablyInstant() const
{
   return !bWasCanceled && !IsComplete() && Duration <= InteractCVars::InstantInteractThreshold;
}

