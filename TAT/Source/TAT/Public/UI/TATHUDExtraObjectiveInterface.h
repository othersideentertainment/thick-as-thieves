// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "TATHUDExtraObjectiveInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UTATHUDExtraObjectiveInterface : public UInterface
{
   GENERATED_BODY()
};

// A hook to set an extra hud objective line
class TAT_API ITATHUDExtraObjectiveInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   // A hook to set an extra hud objective line
   //
   // If an extra objective is already set, it will be replaced
   // If the objective text is empty, it will be removed
   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
   void SetExtraHudObjective(const FText& objectiveText);
};
