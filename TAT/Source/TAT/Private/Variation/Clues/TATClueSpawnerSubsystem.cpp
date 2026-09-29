// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/Clues/TATClueSpawnerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueSpawnerSubsystem)

bool UTATClueSpawnerSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_Client);
}

void UTATClueSpawnerSubsystem::RegisterSpawner(UTATClueSpawnerComponent* spawner)
{
   check(spawner);
   _clueSpawners.Add(spawner);
}

void UTATClueSpawnerSubsystem::UnregisterSpawner(UTATClueSpawnerComponent* spawner)
{
   _clueSpawners.RemoveSingleSwap(spawner);
}

bool UTATClueSpawnerSubsystem::DoesSupportWorldType(const EWorldType::Type worldType) const
{
   return worldType == EWorldType::Game || worldType == EWorldType::PIE;
}
