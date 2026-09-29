// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Spawn/TATQuestSpawnUtils.h"

// tat
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"
#include "Quests/Spawn/TATQuestSpawnTypes.h"
#include "Quests/TATQuestLogging.h"

// ue
#include "GameplayTagContainer.h"

namespace TATQuestSpawnUtils
{
   using FSpawnerWithHash = SeedHelpers::TSpawnerWithHash<UTATQuestActorSpawnerComponent>;

   void InitSpawnerChoices(TConstArrayView<UTATQuestActorSpawnerComponent*> input, TArray<FSpawnerWithHash>& outChoices, TArray<FSpawnerWithHash>& outFallbackChoices)
   {
      outChoices.Reset();
      outFallbackChoices.Reset();

      for (UTATQuestActorSpawnerComponent* spawner : input)
      {
         check(spawner);
         // spawners with scene requirements are higher priority
         TArray<FSpawnerWithHash>& destination = spawner->GetSceneRequirement().IsNone() ? outFallbackChoices : outChoices;
         destination.Emplace(spawner, SeedHelpers::MakeHashForComponent(spawner));
      }

      // sort by hash to ensure deterministic ordering
      outChoices.Sort();
      outFallbackChoices.Sort();
   }
}

void TATQuestSpawnUtils::GeneratePlan(FTATQuestSpawnPlan& outPlan, const FTATQuestSpawnPlanParams& params)
{
   if (params.SpawnRequests.IsEmpty())
   {
      return;
   }

   TMap<FGameplayTag, TArray<UTATQuestActorSpawnerComponent*>> spawnersByLocation;

   const FTATSceneVariantCollection* variants = params.Variants;
   check(variants);

   for (UTATQuestActorSpawnerComponent* spawner : params.QuestSpawners)
   {
      if (IsValid(spawner) && spawner->IsEnabled() && (spawner->GetSceneRequirement().IsNone() || variants->ResolveBool(spawner->GetSceneRequirement())))
      {
         spawnersByLocation.FindOrAdd(spawner->GetQuestLocationTag()).Add(spawner);
      }
   }

   TArray<FTATQuestActorSpawnRequest, TInlineAllocator<8>> requests(params.SpawnRequests);
   requests.Sort([](const FTATQuestActorSpawnRequest& a, const FTATQuestActorSpawnRequest& b)
      {
         int32 tagCompare = a.LocationTag.GetTagName().Compare(b.LocationTag.GetTagName());
         return (tagCompare < 0) || (tagCompare == 0 && a.ActorClass.ToSoftObjectPath().LexicalLess(b.ActorClass.ToSoftObjectPath()));
      });

   FGameplayTag lastTag;
   FRandomStream stream;
   TArray<FSpawnerWithHash> currentSpawners;
   TArray<FSpawnerWithHash> currentFallbackSpawners;
   for (const FTATQuestActorSpawnRequest& request : requests)
   {
      const FGameplayTag locationTag = request.LocationTag;
      if (locationTag != lastTag)
      {
         stream.Initialize(SeedHelpers::MakeSeedForName(request.LocationTag.GetTagName(), params.Seed));

         TArray<UTATQuestActorSpawnerComponent*>* possibleSpawners = spawnersByLocation.Find(locationTag);
         InitSpawnerChoices(possibleSpawners ? *possibleSpawners : TArrayView<UTATQuestActorSpawnerComponent*>(), currentSpawners, currentFallbackSpawners);
         lastTag = locationTag;
      }

      TArray<FSpawnerWithHash>& choices = currentSpawners.Num() ? currentSpawners : currentFallbackSpawners;
      if (choices.Num() == 0)
      {
         UE_LOG(LogTATQuest, Warning, TEXT("Unable to find enough quest spawners for location tag %s"), *locationTag.GetTagName().ToString());
         continue;
      }

      const int32 index = stream.RandHelper(choices.Num());
      UTATQuestActorSpawnerComponent* spawner = choices[index].Spawner;
      outPlan.Add({ spawner, request.ActorClass, request.ActorTag });
      choices.RemoveAtSwap(index);
   }
}

void TATQuestSpawnUtils::ExecutePlan(const FTATQuestSpawnPlan& plan)
{
   for (const FTATQuestActorSpawn& spawn : plan)
   {
      if (UTATQuestActorSpawnerComponent* spawner = spawn.Spawner.Get())
      {
         spawner->ExecuteSpawn(spawn.ActorClass);
      }
   }
}

void TATQuestSpawnUtils::EmitSpawnerActors(TArray<const AActor*>& outActors, const FTATQuestSpawnPlan& plan)
{
   outActors.Reserve(plan.Num());
   for (const FTATQuestActorSpawn& spawn : plan)
   {
      if (UTATQuestActorSpawnerComponent* spawner = spawn.Spawner.Get())
      {
         outActors.Add(spawner->GetOwner());
      }
   }
}

#if WITH_EDITOR
void TATQuestSpawnUtils::ScrapeSpawnersInLevel(UWorld* world, TArray<TObjectPtr<UTATQuestActorSpawnerComponent>>& outSpawners)
{
   for (UTATQuestActorSpawnerComponent* spawner : TObjectRange<UTATQuestActorSpawnerComponent>())
   {
      if (spawner && spawner->GetWorld() == world)
      {
         outSpawners.Add(spawner);
      }
   }
}
#endif
