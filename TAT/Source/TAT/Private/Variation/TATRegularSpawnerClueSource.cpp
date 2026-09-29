// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATRegularSpawnerClueSource.h"

// tat
#include "Variation/Clues/TATClueSet.h"
#include "Variation/Clues/TATClueSourceInterface.h"
#include "Variation/Clues/TATClueSpawnTypes.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/TATSpawnPlan.h"

class UTATClueSetBase;

namespace TATRegularSpawnerClueSource
{
   using FClueParams = ITATClueSourceInterface::FClueSourceParams;
   struct FClueSetAndTag
   {
      FSoftObjectPath ClueSet;
      FGameplayTag SourceTag;

      bool IsValid() const
      {
         return ClueSet.IsValid();
      }
   };

   static FClueSetAndTag FindClueSetForActorClass(const TSoftClassPtr<AActor>& classToSpawn, int32 seed, FGameplayTag locationTag)
   {
      if(classToSpawn.IsNull())
      {
         return {};
      }

      // For now, assume that the actual spawning should have loaded the actor
      // _probably_ don't want to force a sync load here?
      // If I do, probably strength reduce by checking if it implements the interface before loading
      TSubclassOf<AActor> loadedClass = classToSpawn.Get();
      if(!loadedClass)
      {
         // TODO: Log that it wasn't loaded
         return {};
      }

      const ITATClueSourceInterface* clueSource = Cast<ITATClueSourceInterface>(loadedClass.GetDefaultObject());
      if(clueSource == nullptr)
      {
         return {};
      }
      
      const TOptional<FClueParams> params = clueSource->GetClueParameters();
      if(!params)
      {
         return {};
      }

      const FGameplayTag sourceTag = params->SourceTag;
      FSoftObjectPath clueSet = UTATClueSet::FindRandomMatchingSet(sourceTag, locationTag, seed);
      if(clueSet.IsNull())
      {
         clueSet = params->FallbackClueSet;
      }

      return {clueSet, sourceTag};
   }
}

void TATRegularSpawnerClueSource::AddClueSourcesFromSpawnPlan(const FTATSpawnPlan& plan,
                                                              TArray<FTATPendingClueSource>& outSources,
                                                              int32 seed)
{
   for(const FTATSpawnPlanEntry& entry : plan.Spawns)
   {
      if(!entry.DidSpawn) continue;

      UTATSpawnerComponent* spawner = entry.Spawner.Get();
      check(spawner);
      
      switch(spawner->GetClueMode())
      {
      case ETATSpawnerClueMode::FromSpawnedActor:
         {
            const FClueSetAndTag found = FindClueSetForActorClass(entry.ClassToSpawn, seed, spawner->GetClueLocationTag());
            if(found.IsValid())
            {
               outSources.Add(FTATPendingClueSource{
                  .ClueSet = TSoftObjectPtr<UTATClueSetBase>(found.ClueSet),
                  .Location = spawner,
                  .SourceTag = found.SourceTag,
               });
            }
            break;
         }

      default:
         break;
      }
   }
}
