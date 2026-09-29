// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "TATTutorialTriggerVolumeInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UTATTutorialTriggerVolumeInterface : public UInterface
{
   GENERATED_BODY()
};

// A marker interface for actors that should be used as an overlap volume in the tutorial.
//
// The tutorial condition is just going to poll, so it does not currently need any functionality
// beyond attesting that its collision is appropriate. This can be changed later if it needs to be more
// event-driven.
class TAT_API ITATTutorialTriggerVolumeInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
};
