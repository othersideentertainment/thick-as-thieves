// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueSpawnUtils.h"

// tat
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/Clues/TATClueInfo.h"
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/Clues/TATClueSpawner.h"
#include "Variation/Clues/TATClueType.h"

// ose
#include "Utl/OSEUtlFunctionLibrary.h"

// ue
#include "Variation/SceneVariants/TATSceneVariantCollection.h"

#include "Algo/Accumulate.h"
#include "Logging/MessageLog.h"

namespace TATClueSpawnUtils
{
   struct FClueSpawnerCollection;
   using FSpawnerEntry = FTATClueSpawnPlan::FSpawnerEntry;
   using FClueEntry = FTATClueSpawnPlan::FClueEntry;
   using FClueEntryArray = TArray<FClueEntry, TInlineAllocator<32>>;
   
   static void SortBucket(TArray<UTATClueSpawnerComponent*>& spawners)
   {
      using FSpawnerWithHash = SeedHelpers::TSpawnerWithHash<UTATClueSpawnerComponent>;
      TArray<FSpawnerWithHash, TInlineAllocator<32>> hashedSpawners;
      hashedSpawners.Reserve(spawners.Num());
      for (UTATClueSpawnerComponent* spawner : spawners)
      {
         hashedSpawners.Emplace(spawner, SeedHelpers::MakeHashForComponent(spawner));
      }
      hashedSpawners.Sort();
         
      check(hashedSpawners.Num() == spawners.Num());
      const int32 count = hashedSpawners.Num();
      for(int i = 0; i < count; ++i)
      {
         spawners[i] = hashedSpawners[i].Spawner;
      }
   }
   
   struct FClueSpawnerCollection
   {
      void Add(TConstArrayView<TObjectPtr<UTATClueSpawnerComponent>> spawners, const FTATSceneVariantCollection& variants)
      {
         auto isValidSpawner = [&variants] (const UTATClueSpawnerComponent* spawner)
         {
            if(!spawner)
            {
               return false;
            }
            if(!spawner->IsActive())
            {
               return false;
            }
            const FTATSceneRequirement& requirement = spawner->GetSceneRequirement();
            return requirement.IsNone() || variants.ResolveBool(requirement);
         };
         for (UTATClueSpawnerComponent* spawner : spawners)
         {
            if(isValidSpawner(spawner))
            {
               _spawnersByType.FindOrAdd({spawner->GetClueBucket(), spawner->GetRequiredSourceLocation()})
                  .Spawners.Add(spawner);
            }
         }
      }

      TArray<UTATClueSpawnerComponent*>* FindSorted(FTATClueBucketKey bucketKey, FGameplayTag locationTag)
      {
         // Prefer more location-specific spawners
         while(locationTag.IsValid())
         {
            if(TArray<UTATClueSpawnerComponent*>* result = _FindExact(bucketKey, locationTag))
            {
               return result;
            }
            locationTag = locationTag.RequestDirectParent();
         }

         return _FindExact(bucketKey, FGameplayTag());
      }
      
   private:

      TArray<UTATClueSpawnerComponent*>* _FindExact(FTATClueBucketKey bucketKey, FGameplayTag locationTag)
      {
         FBucket* bucket = _spawnersByType.Find({bucketKey, locationTag});
         if(bucket == nullptr || bucket->Spawners.IsEmpty())
         {
            return nullptr;
         }

         if(!bucket->IsSorted)
         {
            SortBucket(bucket->Spawners);
            bucket->IsSorted = true;
         }

         return &bucket->Spawners;
      }
      
      struct FBucket
      {
         TArray<UTATClueSpawnerComponent*> Spawners;
         bool IsSorted = false;
      };

      struct FKey
      {
         FTATClueBucketKey Bucket;
         FGameplayTag LocationTag;

         bool operator==(const FKey& other) const
         {
            return Bucket == other.Bucket && LocationTag == other.LocationTag;
         }

         friend uint32 GetTypeHash(const FKey& key)
         {
            return HashCombineFast(GetTypeHash(key.Bucket), GetTypeHash(key.LocationTag));
         }
      };
      
      TMap<FKey, FBucket> _spawnersByType;
   };

   static FClueEntryArray TakeCluesOfType(TArrayView<FTATClueRequest> requests, FTATClueBucketKey bucket)
   {
      FClueEntryArray clues;
      for(FTATClueRequest& request : requests)
      {
         for(auto it = request.Clues.CreateIterator(); it; ++it)
         {
            FTATClueInfoView clue = *it;
            if(clue.Get<const FTATClueInfo>().GetClueBucket() == bucket)
            {
               clues.Add({clue, request.Context});
               it.RemoveCurrentSwap();
            }
         }
      }
      // certainly hoping for NRVO, since it is inline
      return clues;
   }

   // For electrotype clues, they are all used, and evenly distributed between spawners
   // NB: As a first pass, per David, not attempting to prevent clues from the same source from ending up at the same spawner
   static void DistributeElectrotypeClues(TArrayView<FTATClueRequest> requests, FClueSpawnerCollection& spawnerCollection, int32 seed, FTATClueSpawnPlan& outPlan)
   {
      const FTATClueBucketKey clueBucket = {ETATClueType::Electrotype};

      TArray<UTATClueSpawnerComponent*>* spawnersPtr = spawnerCollection.FindSorted(clueBucket, FGameplayTag());
      if(spawnersPtr == nullptr)
      {
         return;
      }
      TArrayView<UTATClueSpawnerComponent*> spawners = MakeArrayView(*spawnersPtr);
      
      FClueEntryArray clues = TakeCluesOfType(requests, clueBucket);
      FClueEntryArray globalClues = TakeCluesOfType(requests, {ETATClueType::ElectrotypeGlobal});

      FRandomStream randomStream(seed);
      UOSEUtlFunctionLibrary::ShuffleWithRandomStream(spawners, randomStream);
      UOSEUtlFunctionLibrary::ShuffleWithRandomStream(clues, randomStream);
      UOSEUtlFunctionLibrary::ShuffleWithRandomStream(globalClues, randomStream);

      // global clues are just duplicated to all the spawners in the same order (for now)

      // Since clues and spawners are both shuffled, just dole out the clues
      const int spawnerCount = spawners.Num();
      const int cluesPerSpawner = clues.Num() / spawnerCount;
      const int remainder = clues.Num() % spawnerCount;
      const int globalClueCount = globalClues.Num();
      int clueIndex = 0;
      for(int i = 0; i < spawnerCount; ++i)
      {
         const int clueCount = cluesPerSpawner + (i < remainder ? 1 : 0);
         const int combinedClueCount = clueCount + globalClueCount;
         outPlan.Spawners.Add(FSpawnerEntry { spawners[i], combinedClueCount});
         outPlan.Clues.Append(globalClues);
         outPlan.Clues.Append(MakeArrayView(clues).Slice(clueIndex, clueCount));
         clueIndex += clueCount;
      }

      check(clueIndex == clues.Num());
      // take spawners, so not used later (even if the clues have also)
      spawnersPtr->Reset();
   }
}

FTATClueSpawnPlan TATClueSpawnUtils::GeneratePlan(const FTATClueSpawnParams& params)
{
   FTATClueSpawnPlan plan;
   plan.Clues.Reserve(Algo::TransformAccumulate(params.SpawnRequests,
      [](const FTATClueRequest& request) { return request.Clues.Num(); }, 0));

   FClueSpawnerCollection spawnerCollection;
   const FTATSceneVariantCollection& variants = params.Variants ? *params.Variants : FTATSceneVariantCollection::Empty();
   spawnerCollection.Add(params.Spawners, variants);
   
   TArray<FTATClueRequest, TInlineAllocator<8>> clueRequests(params.SpawnRequests);

   // TODO:
   // 1. Location-specific spawner pass

   // Do electrotype clues in a separate pass
   {
      constexpr int32 electrotypeSalt = 0x217a3c80; // CLUE-WIP: have convenience consteval helper generate from it from a string literal?
      DistributeElectrotypeClues(clueRequests, spawnerCollection, SeedHelpers::CombineSeed(params.Seed, electrotypeSalt), plan);
   }

   // TODO: shuffle request order?

   // It is harder to have stratified seeds here, given the interleaved order, so just use a single one (for now, at least)
   FRandomStream randomStream(params.Seed);

   for(FTATClueRequest& clueRequest : clueRequests)
   {
      UOSEUtlFunctionLibrary::ShuffleWithRandomStream(clueRequest.Clues, randomStream);
   }

   // Round-robin through the clue sources, so it tries to allocate them evenly
   while(!clueRequests.IsEmpty())
   {
      for(auto it = clueRequests.CreateIterator(); it; ++it)
      {
         FTATClueRequest& request = *it;
         const FGameplayTag locationTag = request.Context.Location->GetClueLocationTag();
         while(request.Clues.Num())
         {
            FTATClueInfoView clue = request.Clues.Pop(EAllowShrinking::No);
            const FTATClueBucketKey clueBucket = clue.Get<const FTATClueInfo>().GetClueBucket();
            if(TArray<UTATClueSpawnerComponent*>* spawners = spawnerCollection.FindSorted(clueBucket, locationTag))
            {
               const int32 indexToRemove = randomStream.RandHelper(spawners->Num());
               UTATClueSpawnerComponent* spawner = (*spawners)[indexToRemove];
               spawners->RemoveAtSwap(indexToRemove);
               plan.AddSpawnerWithClue(spawner, clue, request.Context);
               break;
            }
            else
            {
               plan.FailedClues.Add({clue, request.Context});
            }
         }

         if(request.Clues.IsEmpty())
         {
            it.RemoveCurrent();
         }
      }
   }
   
   return plan;
}

void FTATClueSpawnPlan::AddClue(const FTATClueContext& context, const FTATClueInfoView& clue)
{
   Clues.Emplace(FClueEntry { clue, context });
}

void FTATClueSpawnPlan::AddSpawnerWithClue(UTATClueSpawnerComponent* spawner,
   const FTATClueInfoView& clue, const FTATClueContext& context)
{
   AddClue(context, clue);
   Spawners.Emplace(FSpawnerEntry { spawner, 1 });
}

void FTATClueSpawnPlan::Execute() const
{
   int32 clueIndex = 0;
   for(const FSpawnerEntry& entry : Spawners)
   {
      UTATClueSpawnerComponent* spawner = entry.Spawner;
      check(spawner);
      TConstArrayView<FClueEntry> clues = MakeArrayView(Clues).Slice(clueIndex, entry.ClueCount);

      for(const FClueEntry& clueEntry : clues)
      {
         const FTATClueInfo& clue = clueEntry.Clue.Get<const FTATClueInfo>();
         clue.ApplyToSpawner(spawner, clueEntry.Context);
      }
      spawner->OnAllCluesApplied();
      
      clueIndex += entry.ClueCount;
   }
}

void FTATClueSpawnPlan::WriteFailuresToLog(FMessageLog& messageLog) const
{
   for(const FClueEntry& clue : FailedClues)
   {
      const FTATClueBucketKey clueBucket = clue.Clue.Get<const FTATClueInfo>().GetClueBucket();
      messageLog.Warning(FText::FromString(
         FString::Format(TEXT("Failed To Spawn Clue: {0}, Source: {1}, Location: {2}, (Placement {3})"), {
            clue.Clue.GetScriptStruct()->GetName(), clue.Context.SourceTag.ToString(),
            clue.Context.Location->GetClueLocationTag().ToString(), clueBucket.PlacementTag.ToString()
         })));
   }
}
