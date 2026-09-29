// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TrapEmitterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTrapEmitterInterface_Old : public UInterface
{
   GENERATED_BODY()
};

// Interface for trap emitters.
// Can be implemented by either components or actors.
//
// CONSIDER: Trap emitters tend to replicate the triggering to clients for ephemeral side effects
//           Is it worth adding a LocalTriggered method to this interface, which the trigger can call
//           that instead of forcing each emitter to do so in the common case. But may be confusing in
//           different use-cases.
class TAT_API ITrapEmitterInterface_Old
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   // Called on authority only
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Traps|Emitters")
   void OnTriggered();
};
