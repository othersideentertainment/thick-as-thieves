// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATRespawnMarkerLocationInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UTATRespawnMarkerLocationInterface : public UInterface
{
   GENERATED_BODY()
};

/**
 * 
 */
class TAT_API ITATRespawnMarkerLocationInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   virtual FVector GetRespawnMarkerLocation() const  = 0;
};
