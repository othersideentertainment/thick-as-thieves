// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueActorSpawner.h"

// tat
#include "Variation/Clues/TATClueType.h"

// ue
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueActorSpawner)

UTATClueActorSpawnerComponent::UTATClueActorSpawnerComponent()
{
   _clueType = ETATClueType::SpawnActor;
}

void UTATClueActorSpawnerComponent::_SpawnClueActor(TSoftClassPtr<AActor> actorClass,
   TFunction<void(AActor*)>&& initialize) const
{
   check(initialize);
   UAssetManager::GetStreamableManager().RequestAsyncLoad(actorClass.ToSoftObjectPath(), [actorClass, init = MoveTemp(initialize), weakOwner = MakeWeakObjectPtr(GetOwner())]
   {
      if(AActor* owner = weakOwner.Get())
      {
         const FTransform& spawnXfm = owner->GetTransform();
         FActorSpawnParameters spawnParams;
         spawnParams.bNoFail = true;
         spawnParams.bDeferConstruction = true;
         if(AActor* spawnedActor = owner->GetWorld()->SpawnActor(actorClass.Get(), &spawnXfm, spawnParams))
         {
            init(spawnedActor);
            spawnedActor->FinishSpawning(spawnXfm);
         }
      }
   });
}

ATATClueActorSpawner::ATATClueActorSpawner()
{
   _spawner = CreateDefaultSubobject<UTATClueActorSpawnerComponent>("Spawner");
}
