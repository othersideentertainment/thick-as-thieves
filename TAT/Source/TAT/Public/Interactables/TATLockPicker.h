// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATLockPicker.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTATLockPicker : public UInterface
{
   GENERATED_BODY()
};

/**
 * Interface for a character that can pick locks
 * 
 * Maybe backed by Gameplay attributes
 */
class TAT_API ITATLockPicker
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   virtual bool CanLockPick() const = 0;
};
