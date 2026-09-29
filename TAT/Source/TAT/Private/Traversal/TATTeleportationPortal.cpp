// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Traversal/TATTeleportationPortal.h"

// ue5
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTeleportationPortal)

DEFINE_LOG_CATEGORY_STATIC(LogTATTeleportation, Log, All);

ATATTeleportationPortal::ATATTeleportationPortal()
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = true;

   InitialLifeSpan = 10.0f;
}

void ATATTeleportationPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // The destination is set when we are first spawned
   DOREPLIFETIME_CONDITION(ATATTeleportationPortal, _teleportDestination, COND_InitialOnly)
}


