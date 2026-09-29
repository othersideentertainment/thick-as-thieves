// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Interactables/TATSwingingDoor.h"

class USceneComponent;

#include "TATTiltedSwingingDoor.generated.h"

// Subclass that allows designers to apply a pre-rotation to the door pivot inherited from TATSwingingDoor, via reparenting it to _referenceFrame.
// NOTE: reparenting BPs from TATSwingingDoor -> TATTiltedSwingingDoor may be risky, due to the reattachment occuring in constructor.
UCLASS()
class TAT_API ATATTiltedSwingingDoor : public ATATSwingingDoor
{
   GENERATED_BODY()

   ATATTiltedSwingingDoor();

private:
   UPROPERTY(EditDefaultsOnly)
   USceneComponent* _referenceFrame;
};
