// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATSpawnPlan.h"

// tat
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnerComponent.h"

// ue5
#include "Logging/MessageLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpawnPlan)


void FTATSpawnPlan::Reset()
{
   Spawns.Reset();
   Modifiers.Reset();
   GroupSummaries.Reset();
}

void FTATSpawnPlan::Execute(ETATSpawnTiming timing) const
{
   FTATSpawnCursor(timing).Execute(*this);
}

TSet<FSoftObjectPath> FTATSpawnPlan::CollectPreloadAssets() const
{
   TSet<FSoftObjectPath> result;
   for (const FTATSpawnPlanEntry& spawnResult : Spawns)
   {
      if (spawnResult.DidSpawn && !spawnResult.ClassToSpawn.IsNull())
      {
         result.Add(spawnResult.ClassToSpawn.ToSoftObjectPath());
      }
   }
   return result;
}

void FTATSpawnPlan::LogGroupSummaries(FMessageLog& messageLog) const
{
   for (const FTATSpawnGroupSummary& summary : GroupSummaries)
   {
      messageLog.Info()
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(" - Group \"%s\" spawned %d objects [min: %d, max: %d, spawners %d]"), *summary.SpawnGroupTag.ToString(), summary.NumSpawned, summary.MinSpawners, summary.MaxSpawners, summary.PossibleSpawners))));
      if (summary.NumSpawned < summary.MinSpawners)
      {
         messageLog.Error()
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(" ---- Group \"%s\" spawned fewer than the minimum (%d < %d)"), *summary.SpawnGroupTag.ToString(), summary.NumSpawned, summary.MinSpawners))));
      }
      else if (summary.NumSpawned < summary.NumDesired)
      {
         messageLog.Warning()
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT(" ---- Group \"%s\" spawned fewer than desired (%d < %d)"), *summary.SpawnGroupTag.ToString(), summary.NumSpawned, summary.NumDesired))));
      }
   }
}

bool FTATSpawnCursor::IsValid(const FTATSpawnPlan& plan) const
{
   return plan.Spawns.IsValidIndex(_index);
}

void FTATSpawnCursor::Step(const FTATSpawnPlan& plan)
{
   FTATVariationSpawnContext context;

   for ( ; _index < plan.Spawns.Num(); ++_index)
   {
      const FTATSpawnPlanEntry& spawnResult = plan.Spawns[_index];
      const int32 currentModifierIndex = _modifierIndex;
      _modifierIndex += spawnResult.ModifierCount;

      UTATSpawnerComponent* spawner = spawnResult.Spawner;
      if (!spawner || spawner->GetSpawnTiming() != _timing)
      {
         continue;
      }

      FRandomStream randomStream(spawnResult.RandomSeed);

      if (!spawnResult.DidSpawn)
      {
         spawner->AuthorityNotSpawn(context, randomStream);
      }
      else if (spawnResult.ClassToSpawn.IsNull())
      {
         spawner->AuthoritySpawn(context, randomStream);
      }
      else
      {
         // we can send over an actor class, or not, depending on how the bucket was defined
         AActor* spawnedActorInstance = spawnedActorInstance = spawner->AuthoritySpawnActorDeferred(context, spawnResult.ClassToSpawn, randomStream);
         if (!spawnedActorInstance)
         {
            VALIDATE_MAPVARIATION_ERROR(TEXT("Spawner \"%s\" in failed to deferred-spawn an actor instance of class %s!")
               , *spawner->GetReadableName()
               , spawnResult.ClassToSpawn->IsValidLowLevelFast() ? *spawnResult.ClassToSpawn->GetName() : TEXT("<Invalid class>"));
            continue;
         }

         TConstArrayView<TObjectPtr<UTATSpawnModifier>> modifiersForSpawner = MakeArrayView(plan.Modifiers).Slice(currentModifierIndex, spawnResult.ModifierCount);
         auto applyModifiers = [&randomStream, modifiersForSpawner](AActor* spawnedActor, bool beforeFinish) {
            for (UTATSpawnModifier* modifier : modifiersForSpawner)
            {
               check(modifier);
               if (modifier->ApplyBeforeActorFinishSpawn() == beforeFinish)
               {
                  modifier->ApplyModifier(*spawnedActor, randomStream);
               }
            }
            };

         applyModifiers(spawnedActorInstance, true);

         // now finish spawning
         check(::IsValid(spawnedActorInstance));
         spawner->AuthorityFinishSpawnActor(context, spawnResult.ClassToSpawn, randomStream, spawnedActorInstance);

         // if we have some modifiers to apply to this spawner after we finish spawning the actor, apply them now
         applyModifiers(spawnedActorInstance, false);
      }

      // Only do a single spawn per Step call
      ++_index;
      break;
   }
}

void FTATSpawnCursor::Execute(const FTATSpawnPlan& plan)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(FTATSpawnCursor::Execute);
   while (IsValid(plan))
   {
      Step(plan);
   }
}
