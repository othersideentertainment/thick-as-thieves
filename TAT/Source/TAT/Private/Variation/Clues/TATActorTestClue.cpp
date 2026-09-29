// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/Clues/TATActorTestClue.h"

// tat
#include "Variation/Clues/TATClueActorSpawner.h"
#include "Variation/Clues/TATClueType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActorTestClue)

FTATClueBucketKey FTATActorTestClue::GetClueBucket() const
{
   return {ETATClueType::SpawnActor};
}

void FTATActorTestClue::ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const
{
   CastChecked<UTATClueActorSpawnerComponent>(spawner)->SpawnClueActor(ActorToSpawn, [](AActor* actor) {});
}

#if WITH_EDITOR
void FTATActorTestClue::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ActorToSpawn.IsNull())
   {
      reportError(INVTEXT("ActorToSpawn is null"));
   }
}
#endif
