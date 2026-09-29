// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATBreakableActorInfoInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UTATBreakableActorInfoInterface : public UInterface
{
   GENERATED_BODY()
};

// The AI need to react to a broken object, and there are a couple of ways to "break" something via the BreakableComponent
// or for the loot durability events. The goal of this interface is to have a single place to call in order to discover if
// an object is broken or not.
class TAT_API ITATBreakableActorInfoInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   virtual bool IsBroken() const = 0;
};
