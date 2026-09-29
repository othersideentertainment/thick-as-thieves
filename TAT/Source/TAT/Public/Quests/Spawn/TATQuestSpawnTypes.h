// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UTATQuestActorSpawnerComponent;
struct FTATQuestActorSpawn;
struct FTATQuestSpawnLocationName;

struct TAT_API FTATQuestActorSpawnRequest
{
   // Actor to spawn 
   TSoftClassPtr<AActor> ActorClass;
   // Pass through tag that represents the spawn (e.g. loot/item type)
   FGameplayTag ActorTag;
   // Tag representing the location type to spawn in
   FGameplayTag LocationTag;

   bool operator==(const FTATQuestActorSpawnRequest& other) const
   {
      return ActorClass == other.ActorClass && ActorTag == other.ActorTag && LocationTag == other.LocationTag;
   }
};

struct TAT_API FTATQuestActorSpawn
{
   TWeakObjectPtr<UTATQuestActorSpawnerComponent> Spawner = nullptr;
   TSoftClassPtr<AActor> ActorClass;
   FGameplayTag ActorTag;

   bool operator==(const FGameplayTag& tag) const { return ActorTag == tag; }
};

