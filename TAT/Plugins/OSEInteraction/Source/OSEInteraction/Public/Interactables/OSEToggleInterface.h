// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "OSEToggleInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOSEToggleInterface : public UInterface
{
   GENERATED_BODY()
};

// A minimal interface for toggle-like things extracted from OSESyncedToggle
//
// So that other things can be driven by it. This does not currently expose enough to fully drive
// `UOSEInteractableToggleSwitchComponent`, but that could be worked up to. Want to be cautious
// about the surface area.
//
// Some notes:
// 1. Method names are kept separate from OSESyncedToggle, so that it does not imply too much
//    in other implementers
// 2. Keeping surface area minimal to start with. May expand somewhat, but should keep small
// 3. Keeping non-BP-exposed initially so it remains easy to change for now
class OSEINTERACTION_API IOSEToggleInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   virtual bool IsToggleOn() const = 0;
   // Not guaranteed to succeed
   virtual void SetToggleOn(bool isOn) = 0;
};
