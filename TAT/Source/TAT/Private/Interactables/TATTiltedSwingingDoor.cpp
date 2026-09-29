// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATTiltedSwingingDoor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTiltedSwingingDoor)

DEFINE_LOG_CATEGORY_STATIC(LogTATTiltedSwingingDoor, Log, All);

ATATTiltedSwingingDoor::ATATTiltedSwingingDoor()
{
   // Create reference frame to allow applying a pre-rotation to pivot
   _referenceFrame = CreateDefaultSubobject<USceneComponent>(TEXT("ReferenceFrame"));
   _referenceFrame->SetupAttachment(RootComponent);

   // Reparent pivot to reference frame
   check(_pivot);
   _pivot->SetupAttachment(_referenceFrame);
}
