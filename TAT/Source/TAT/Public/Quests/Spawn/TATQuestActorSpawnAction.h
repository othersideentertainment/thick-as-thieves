// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATQuestActorSpawnAction.generated.h"

class UTATQuestActorSpawnerComponent;

/// A base class for static data for a clue
USTRUCT()
struct TAT_API FTATQuestActorSpawnAction
{
   GENERATED_BODY()

   // Not actually needed, since nothing deletes via a pointer to base class (or indirectly), but silences warnings
   virtual ~FTATQuestActorSpawnAction() = default;

   virtual void OnSpawnedQuestActor(AActor* spawnedActor, UTATQuestActorSpawnerComponent* spawner) const {}

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif
};
