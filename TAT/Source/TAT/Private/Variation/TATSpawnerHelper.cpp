// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATSpawnerHelper.h"

// TAT
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATMapVariationSeedHelpers.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnPlan.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/Fnv.h"
#include "Misc/UObjectToken.h"
#include "GameplayTagContainer.h"

// Might not stick around
UE_TRACE_CHANNEL(SpawnerChannel)

namespace SpawnHelpers {
   int32 MakeHashForSpawner(const UTATSpawnerComponent* spawner)
   {
      TStringBuilder<256> pathBuilder;
      
      const AActor* owner = spawner->GetOwner();
      check(owner);
      
      // Include world name because of level instances
      const UObject* world = owner->GetTypedOuter(UWorld::StaticClass());
      if (ensure(world))
      {
         FName packageName = world->GetOuter()->GetFName();
         packageName.SetNumber(0); //< Level instances use a different suffix (0,1) for game worlds, clear so they have the same value
         packageName.AppendString(pathBuilder);
         pathBuilder.AppendChar('.');
      }

      owner->GetFName().AppendString(pathBuilder);
      pathBuilder.AppendChar('.');
      spawner->GetFName().AppendString(pathBuilder);

#if WITH_EDITOR
      if(spawner->GetWorld()->IsPlayInEditor())
      {
         pathBuilder = UWorld::RemovePIEPrefix(pathBuilder.ToString());
      }
#endif

      static_assert(PLATFORM_LITTLE_ENDIAN, "assuming little endian");
      return static_cast<int32>(FXxHash64::HashBuffer(pathBuilder.GetData(), pathBuilder.Len() * sizeof(TCHAR)).Hash & 0xFFFFFFFF);
   }

   static int32 MakeSeedForGroupTag(FGameplayTag tag, int32 initialSeed)
   {
      return SeedHelpers::MakeSeedForName(tag.GetTagName(), initialSeed);
   }

   static float ResolveSpawnChance(const UTATSpawnerComponent* spawner, const FTATSceneSpawnerOverride& override)
   {
      const ETATSceneSpawnerOverrideType overrideType = override.Type;
      switch (override.Type)
      {
      case ETATSceneSpawnerOverrideType::Always:
         return 100;
      case ETATSceneSpawnerOverrideType::Never:
         return 0;
      case ETATSceneSpawnerOverrideType::OverrideChance:
         return override.SpawnChancePercent;
      case ETATSceneSpawnerOverrideType::Default:
         // this is NA for spawn groups and disabled, so ignore for now
         switch (spawner->GetSpawnType())
         {
         case ETATSpawnChanceType::Always:
            return 100;
         case ETATSpawnChanceType::PercentChance:
            return spawner->GetSpawnChancePercent();
         default:
            return 0;
         }
      }

      checkNoEntry();
      return 0;
   }

   struct FSpawnerDependencyDepthCache
   {
      int32 GetDependencyDepth(const UTATSpawnerComponent* spawner)
      {
         check(spawner);

         if (!spawner->IsEligibleForAutomaticOrdering())
         {
            return 0;
         }

         if (const FEntry* found = _depthCache.Find(spawner))
         {
            UE_CLOG(found->InProgress, LogTATMapVariation, Error, TEXT("Cycle in spawner involving %s (should be caught by validation)"), *spawner->GetReadableName());
            return found->Depth;
         }

         _depthCache.Add(spawner).InProgress = true;

         int32 depth = 0;
         if (const UTATSpawnerComponent* parent = spawner->GetParentSpawner())
         {
            depth = GetDependencyDepth(parent) + 1;
         }

         FEntry& entryToUpdate = _depthCache.FindChecked(spawner);
         entryToUpdate.Depth = depth;
         entryToUpdate.InProgress = false;

         return depth;
      }

private:
      struct FEntry
      {
         int32 Depth = 0;
         bool InProgress = false;
      };

      TMap<const UTATSpawnerComponent*, FEntry> _depthCache;
   };
}

struct FTATSpawnerHelper
{
public:
   FTATSpawnerHelper(const FTATSpawnGenerationParams& params);
   UE_NONCOPYABLE(FTATSpawnerHelper);

   void Populate();

   void GeneratePlan(FTATSpawnPlan& outPlan);

   void ValidateEnoughSpawnersToSatisfyRules(FMessageLog& msgLog) const;
   void ValidateNoHashCollisions(FMessageLog& msgLog) const;

private:
   // Not safe if this lives beyond a stack frame, but that is true for the rest of this
   struct FSpawnerEntry
   {
      FSpawnerEntry() = default;
      FSpawnerEntry(UTATSpawnerComponent* spawner, int32 priority, int32 hash, int32 index, float chance)
         : Spawner(spawner), Hash(hash), Priority(static_cast<int16>(priority)), Index(static_cast<int16>(index)), PercentChance(chance)
      {
         check(priority <= TNumericLimits<int16>::Max());
         check(index <= TNumericLimits<int16>::Max());
      }

      UTATSpawnerComponent* Spawner = nullptr;
      int32 Hash = 0;
      int16 Priority = 0;
      int16 Index = 0; //< index if there are multiple spawns for the same spawner
      float PercentChance = 0;

      bool operator<(const FSpawnerEntry& other) const;
   };

   struct FSpawnGroupSection
   {
      // group spawners that have had their spawn chance overridden
      TArray<FSpawnerEntry> ChanceSpawners;
      // high priority group spawners (from scene variants)
      TArray<FSpawnerEntry> PriorityGroupSpawners;
      // regular group spawners
      TArray<FSpawnerEntry> GroupSpawners;

      TArray<FSpawnerEntry>& ChooseArrayForAdd(const FTATSceneRequirement& requirement, const FTATSceneSpawnerOverride & override);

      int32 Num() const
      {
         return ChanceSpawners.Num() + PriorityGroupSpawners.Num() + GroupSpawners.Num();
      }

      int32 GroupOnlyNum() const
      {
         return PriorityGroupSpawners.Num() + GroupSpawners.Num();
      }

      TArray<FSpawnerEntry>& GetFirstGroupWithSpawners()
      {
         return PriorityGroupSpawners.Num() ? PriorityGroupSpawners : GroupSpawners;
      }

      void Sort()
      {
         ChanceSpawners.Sort();
         PriorityGroupSpawners.Sort();
         GroupSpawners.Sort();
      }
   };

   void _SortSpawners();
   void _RunSpawnGroupSpawning(FTATSpawnPlan& outPlan);
   int32 _RunChanceSpawners(const TArray<FSpawnerEntry>& spawners, FTATSpawnPlan& outPlan);
   void _RunWeightedChanceSpawning(FTATSpawnPlan& outPlan);
   void _CompleteSpawnGroupSpawning(FTATSpawnPlan& outPlan);

   bool _ShouldSkipSpawner(const UTATSpawnerComponent* spawner) const;
   int32 _GetSeedForSpawnerEntry(const FSpawnerEntry& spawnerEntry) const;
   static bool _SelectAndRemoveSpawnerFromArray(TArray<FSpawnerEntry>& spawnerArray, const FRandomStream& randomStream, FSpawnerEntry& outEntry);

private:

   // in
   const TConstArrayView<FTATSpawnGroup> _spawnGroups;
   const TConstArrayView<UTATSpawnerComponent*> _spawners;
   const FTATSceneVariantCollection& _sceneVariants;
   const TConstArrayView<const AActor*> _externalSpawners;
   const int32 _initialSeed;

   // generated

   // { spawn group tag : array<spawners> }
   TMap<FGameplayTag, FSpawnGroupSection> _spawnGroupsToSpawners;

   // spawners outside of the spawn group 
   TArray<FSpawnerEntry> _weightedChanceSpawners;

   TSet<FObjectKey> _activatedSpawners;
};

void SpawnHelpers::GeneratePlan(FTATSpawnPlan& outPlan, const FTATSpawnGenerationParams& params)
{
   FTATSpawnerHelper(params).GeneratePlan(outPlan);
}

void SpawnHelpers::ValidateSpawners(TConstArrayView<FTATSpawnGroup> spawnGroups, TConstArrayView<UTATSpawnerComponent*> spawners, FMessageLog& msgLog)
{
   FTATSpawnGenerationParams params;
   params.SpawnGroups = spawnGroups;
   params.Spawners = spawners;

   FTATSpawnerHelper helper(params);
   helper.Populate();
   helper.ValidateEnoughSpawnersToSatisfyRules(msgLog);
   helper.ValidateNoHashCollisions(msgLog);
}

FTATSpawnerHelper::FTATSpawnerHelper(const FTATSpawnGenerationParams& params)
   : _spawnGroups(params.SpawnGroups)
   , _spawners(params.Spawners)
   , _sceneVariants(params.SceneVariants ? *params.SceneVariants : FTATSceneVariantCollection::Empty())
   , _externalSpawners(params.ExternalSpawners)
   , _initialSeed(params.Seed)
{

}

void FTATSpawnerHelper::Populate()
{
   TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(FTATSpawnerHelper::Populate, SpawnerChannel)
   SpawnHelpers::FSpawnerDependencyDepthCache dependencyCache;

   for (UTATSpawnerComponent* spawner : _spawners)
   {
      if (!IsValid(spawner))
         continue;

      switch (spawner->GetSpawnType())
      {
      case ETATSpawnChanceType::UseSpawnGroup:
      {
         const FGameplayTag spawnGroupTag = spawner->GetSpawnGroupTag();
         if (!spawnGroupTag.IsValid())
            continue;

         // Find bucket list this spawner belongs in
         FSpawnGroupSection& spawnerSection = _spawnGroupsToSpawners.FindOrAdd(spawnGroupTag);

         const FTATSceneSpawnerOverride& override = _sceneVariants.ResolveSpawner(spawner->GetSceneRequirement());
         TArray<FSpawnerEntry>& spawnersInBucket = spawnerSection.ChooseArrayForAdd(spawner->GetSceneRequirement(), override);

         // Add an instance of this spawner for each instance it should be spawning (duplicating to avoid random selection to tend towards spawners that produce less instances)
         const int32 hash = SpawnHelpers::MakeHashForSpawner(spawner);
         constexpr int32 priority = 0; //< no meaningful dependency priority within a spawn group
         const int32 numInstancesToSpawn = spawner->GetMaxInstancesToSpawn();
         const float spawnChance = SpawnHelpers::ResolveSpawnChance(spawner, override);
         for (int32 spawnInstance = 0; spawnInstance < numInstancesToSpawn; spawnInstance++)
         {
            spawnersInBucket.Emplace(spawner, priority, hash, spawnInstance, spawnChance);
         }
      }
      break;
      case ETATSpawnChanceType::Always:
      case ETATSpawnChanceType::PercentChance:
      {
         const FTATSceneSpawnerOverride & override = _sceneVariants.ResolveSpawner(spawner->GetSceneRequirement());

         // Add an instance of this spawner for each instance it should be spawning (duplicating to avoid random selection to tend towards spawners that produce less instances)
         const int32 hash = SpawnHelpers::MakeHashForSpawner(spawner);
         const int32 priority = dependencyCache.GetDependencyDepth(spawner);
         const int32 numInstancesToSpawn = spawner->GetMaxInstancesToSpawn();
         const float spawnChance = SpawnHelpers::ResolveSpawnChance(spawner, override);
         for (int32 spawnInstance = 0; spawnInstance < numInstancesToSpawn; spawnInstance++)
         {
            _weightedChanceSpawners.Emplace(spawner, priority, hash, spawnInstance, spawnChance);
         }
      }
      break;
      case ETATSpawnChanceType::Disabled:
      {
         // ignore!
      }
      break;
      default:
         unimplemented();
      }
   }
}

void FTATSpawnerHelper::GeneratePlan(FTATSpawnPlan& plan)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(FTATSpawnerHelper::GeneratePlan)

   Populate();

   int32 expectedCount = _weightedChanceSpawners.Num();
   for (const auto& spawnGroupEntry : _spawnGroupsToSpawners)
   {
      expectedCount += spawnGroupEntry.Value.Num();
   }
   plan.Spawns.Reserve(expectedCount);
   
   _SortSpawners();
   _RunSpawnGroupSpawning(plan);
   _CompleteSpawnGroupSpawning(plan);
   _RunWeightedChanceSpawning(plan);
}

void FTATSpawnerHelper::_SortSpawners()
{
   TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(FTATSpawnerHelper::SortSpawners, SpawnerChannel)
   for (auto& spawnGroupEntry : _spawnGroupsToSpawners)
   {
      spawnGroupEntry.Value.Sort();
   }
   _weightedChanceSpawners.Sort();
}

bool FTATSpawnerHelper::_ShouldSkipSpawner(const UTATSpawnerComponent* spawner) const
{
   check(spawner);
   const ETATSpawnerDependencyType requirement = spawner->GetParentRequirementType();
   if (requirement != ETATSpawnerDependencyType::None)
   {
      const FTATSpawnerParent parent = spawner->GetParent();
      UE_CLOG(!parent.IsValid(), LogTATMapVariation, Warning, TEXT("Could not find parent spawner of %s"), *spawner->GetReadableName());

      const bool parentDidSpawn = parent.Spawner ? _activatedSpawners.Contains(parent.Spawner) : _externalSpawners.Contains(parent.ExternalSpawner);
      const bool shouldSkip = (parentDidSpawn != (requirement == ETATSpawnerDependencyType::RequireParentSpawn));

      UE_LOG(LogTATMapVariation, Verbose, TEXT("Spawner %s was %s because of %s dependency on %s"),
         *spawner->GetReadableName(), shouldSkip ? TEXT("skipped") : TEXT("allowed"),
         *UEnum::GetValueAsString(requirement), *parent.ToString());
      return shouldSkip;
   }

   return false;
}

int32 FTATSpawnerHelper::_GetSeedForSpawnerEntry(const FSpawnerEntry& spawnerEntry) const
{
   // Might want to do something fancier, but see if this meets the good-enough statistical tests first
   int fnv = FFnv::MemFnv32(&spawnerEntry.Hash, sizeof(spawnerEntry.Hash), _initialSeed);
   fnv = FFnv::MemFnv32(&spawnerEntry.Index, sizeof(spawnerEntry.Index), fnv);
   return fnv;
}

bool FTATSpawnerHelper::_SelectAndRemoveSpawnerFromArray(TArray<FSpawnerEntry>& spawnerArray, const FRandomStream& randomStream, FSpawnerEntry& outEntry)
{
   if (spawnerArray.Num() > 0)
   {
      int indexToSpawnInto = randomStream.RandHelper(spawnerArray.Num());
      outEntry = spawnerArray[indexToSpawnInto];
      check(outEntry.Spawner);

      // remove this spawner from future consideration
      spawnerArray.RemoveAtSwap(indexToSpawnInto);
      return true;
   }

   return false;
}

void FTATSpawnerHelper::_RunSpawnGroupSpawning(FTATSpawnPlan& plan)
{
   TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(FTATSpawnerHelper::RunSpawnGroupSpawning, SpawnerChannel);

   plan.GroupSummaries.Reserve(_spawnGroups.Num());
   for (const FTATSpawnGroup& spawnGroup : _spawnGroups)
   {
      // spawners live in the level that aren't in the spawn group definition, which is a normal case
      // because we may want to put groups in the level for different missions
      FSpawnGroupSection* section = _spawnGroupsToSpawners.Find(spawnGroup.SpawnGroupTag);
      if (section == nullptr)
      {
         continue;
      }

      UE_LOG(LogTATMapVariation, Verbose, TEXT("Spawn Group \"%s\":"), *spawnGroup.SpawnGroupTag.ToString());

      // Remove all spawners that should be skipped
      auto removeSkippable = [&plan, this](const FSpawnerEntry& entry) {
         if (_ShouldSkipSpawner(entry.Spawner))
         {
            const int32 randomSeed = _GetSeedForSpawnerEntry(entry);
            plan.EmitNotSpawn(entry.Spawner, randomSeed);
            return true;
         }
         return false;
         };
      section->PriorityGroupSpawners.RemoveAllSwap(removeSkippable);
      section->GroupSpawners.RemoveAllSwap(removeSkippable);

      const int32 groupSuccessStart = plan.Spawns.Num();
      int numSpawned = 0;
      // Run spawners with overridden probabilities first
      numSpawned += _RunChanceSpawners(section->ChanceSpawners, plan);

      // how many of these do we spawn?
      FRandomStream groupRandomStream(SpawnHelpers::MakeSeedForGroupTag(spawnGroup.SpawnGroupTag, _initialSeed));
      const int numToSpawn = groupRandomStream.RandRange(spawnGroup.SpawnCount.Min, spawnGroup.SpawnCount.Max);
      const int numInitialPotentialSpawners = section->GroupOnlyNum() + numSpawned;

      while (numSpawned < numToSpawn)
      {
         TArray<FSpawnerEntry>& spawnersToSelectFrom = section->GetFirstGroupWithSpawners();

         FSpawnerEntry foundEntry;
         if (!_SelectAndRemoveSpawnerFromArray(spawnersToSelectFrom, groupRandomStream, foundEntry))
         {
            break;
         }

         ++numSpawned;
         UTATSpawnerComponent* spawnedInto = foundEntry.Spawner;
         FRandomStream spawnerStream(_GetSeedForSpawnerEntry(foundEntry));
         FTATSpawnPlanEntry& spawnResult = plan.Spawns.Emplace_GetRef(spawnedInto);
         spawnResult.DidSpawn = true;
         _activatedSpawners.Add(spawnedInto);

         if(const UTATActorSpawnBucketAsset* spawnBucket = spawnedInto->GetSpawnBucketAsset())
         {
            spawnResult.ClassToSpawn = spawnBucket->AuthorityFindClassInBucketToSpawn(spawnerStream);
         }

         spawnResult.RandomSeed = spawnerStream.GetCurrentSeed();
      }

      // Add modifiers after the fact. This allows use of the precise number of spawns, in case it went over or under desired
      if(numSpawned > 0 && spawnGroup.Modifiers.Num())
      {
         FMemMark mark(FMemStack::Get());
         TBitArray<TMemStackAllocator<>> chosenModifiers;
         const int32 numModifiers = spawnGroup.Modifiers.Num();
         spawnGroup.GetModifierIndicesForSpawns(chosenModifiers, numSpawned, spawnGroup.SpawnCount.Max, groupRandomStream);
         check(chosenModifiers.Num() == (numModifiers * numSpawned));

         // Note: This is pretty fast, while most of the non-spawns will happen after this, but that will change as scene variants become more prevalent
         TArrayView<FTATSpawnPlanEntry> spawnsForGroup = MakeArrayView(plan.Spawns).Slice(groupSuccessStart, plan.Spawns.Num() - groupSuccessStart);
         int32 nextIndex = 0;
         for (FTATSpawnPlanEntry& entry : spawnsForGroup)
         {
            if (entry.DidSpawn && !entry.ClassToSpawn.IsNull())
            {
               for (int i = 0; i < numModifiers; ++i)
               {
                  if (chosenModifiers[nextIndex + i])
                  {
                     UTATSpawnModifier* modifier = spawnGroup.Modifiers[i];
                     check(modifier);
                     plan.Modifiers.Add(modifier);
                     entry.ModifierCount += 1;
                  }
               }
               nextIndex += numModifiers;
            }
         }
      }

      FTATSpawnGroupSummary& groupSummary = plan.GroupSummaries.Emplace_GetRef(spawnGroup.SpawnGroupTag);
      groupSummary.NumSpawned = numSpawned;
      groupSummary.NumDesired = numToSpawn;
      groupSummary.PossibleSpawners = numInitialPotentialSpawners;
      groupSummary.MinSpawners = spawnGroup.SpawnCount.Min;
      groupSummary.MaxSpawners = spawnGroup.SpawnCount.Max;
   }
}

int32 FTATSpawnerHelper::_RunChanceSpawners(const TArray<FSpawnerEntry>& spawners, FTATSpawnPlan& plan)
{
   FTATVariationSpawnContext spawnContext = FTATVariationSpawnContext();
   int32 numSpawned = 0;

   for (const FSpawnerEntry& entry : spawners)
   {
      UTATSpawnerComponent* spawner = entry.Spawner;
      check(spawner);
      FRandomStream spawnerStream(_GetSeedForSpawnerEntry(entry));

      FTATSpawnPlanEntry& spawnResult = plan.Spawns.Emplace_GetRef(spawner);
      ETATSpawnChanceType spawnChance = spawner->GetSpawnType();
      float spawnChancePercent = entry.PercentChance;
      const bool shouldSkip = _ShouldSkipSpawner(spawner);
      const bool doSpawn = !shouldSkip && (spawnChancePercent >= 100 || (spawnChancePercent > 0 && spawnerStream.FRandRange(0.0f, 100.0f) <= spawnChancePercent));

      spawnResult.DidSpawn = doSpawn;
      if (doSpawn)
      {
         numSpawned++;
         _activatedSpawners.Add(spawner);

         UTATActorSpawnBucketAsset* spawnBucket = spawner->GetSpawnBucketAsset();
         spawnResult.ClassToSpawn = spawnBucket ? spawnBucket->AuthorityFindClassInBucketToSpawn(spawnerStream) : nullptr;
      }
      spawnResult.RandomSeed = spawnerStream.GetCurrentSeed();
   }

   return numSpawned;
}

void FTATSpawnerHelper::_RunWeightedChanceSpawning(FTATSpawnPlan& plan)
{
   TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(FTATSpawnerHelper::RunWeightedChanceSpawning, SpawnerChannel);
   _RunChanceSpawners(_weightedChanceSpawners, plan);
}

void FTATSpawnerHelper::_CompleteSpawnGroupSpawning(FTATSpawnPlan& plan)
{
   TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(FTATSpawnerHelper::CompleteSpawnGroupSpawning, SpawnerChannel)
   FTATVariationSpawnContext spawnContext = FTATVariationSpawnContext();

   auto emitNotSpawn = [this](const TArray<FSpawnerEntry>& spawners, FTATSpawnPlan& plan) {
      for (const FSpawnerEntry& entry : spawners)
      {
         const int32 randomSeed = _GetSeedForSpawnerEntry(entry);
         plan.EmitNotSpawn(entry.Spawner, randomSeed);
      }
   };

   // tell the rest of the unused spawners we don't need 'em so they can do any cleanup that they may need to
   // TODO: this order is not deterministic
   for (const auto& spawnGroupEntry : _spawnGroupsToSpawners)
   {
      emitNotSpawn(spawnGroupEntry.Value.PriorityGroupSpawners, plan);
      emitNotSpawn(spawnGroupEntry.Value.GroupSpawners, plan);
   }
}

void FTATSpawnerHelper::ValidateEnoughSpawnersToSatisfyRules(FMessageLog& msgLog) const
{
   // verify we have enough spawners in the level for each spawn group
   // TODO: segregate by scene variants?
   for (const FTATSpawnGroup& spawnGroup : _spawnGroups)
   {
      const int minSpawns = spawnGroup.SpawnCount.Min;
      const int maxSpawns = spawnGroup.SpawnCount.Max;
      if (maxSpawns == 0)
      {
         continue;
      }

      if (const FSpawnGroupSection* foundSpawners = _spawnGroupsToSpawners.Find(spawnGroup.SpawnGroupTag))
      {
         const int numSpawners = foundSpawners->Num();
         if (numSpawners < minSpawns)
         {
            msgLog.Warning(FText::FromString(FString::Printf(TEXT("Spawn group %s requires at least %d spawners but only has %d."), *spawnGroup.SpawnGroupTag.ToString(), minSpawns, numSpawners)));
         }
         else if (numSpawners < maxSpawns)
         {
            msgLog.Info(FText::FromString(FString::Printf(TEXT("Spawn group %s could use up to %d spawners but only has %d."), *spawnGroup.SpawnGroupTag.ToString(), maxSpawns, numSpawners)));
         }
      }
      else
      {
         msgLog.Warning(FText::FromString(FString::Printf(TEXT("Spawn group %s requires spawners but has none."), *spawnGroup.SpawnGroupTag.ToString())));
      }
   }
}

void FTATSpawnerHelper::ValidateNoHashCollisions(FMessageLog& msgLog) const
{
   TMap<int32, UTATSpawnerComponent*> lookup;
   auto checkArray = [&lookup, &msgLog](const TArray<FSpawnerEntry>& spawners) {
      lookup.Reset();

      auto createObjectToken = [](const UTATSpawnerComponent* spawner) {
         return FUObjectToken::Create(spawner->GetOwner(), FText::FromString(spawner->GetReadableName()));
      };

      for (const FSpawnerEntry& entry : spawners)
      {
         const UTATSpawnerComponent* previous = lookup.FindOrAdd(entry.Hash, entry.Spawner);
         if (previous && previous != entry.Spawner)
         {
            msgLog.Error()
               ->AddToken(FTextToken::Create(FText::FromString(TEXT("Spawner hash collision between"))))
               ->AddToken(createObjectToken(previous))
               ->AddToken(FTextToken::Create(FText::FromString(TEXT("and"))))
               ->AddToken(createObjectToken(entry.Spawner))
               ->AddToken(FTextToken::Create(FText::FromString(TEXT(". Rename slightly or fix code to tolerate this."))));
         }
      }
   };

   for (const auto& spawnGroupEntry : _spawnGroupsToSpawners)
   {
      checkArray(spawnGroupEntry.Value.ChanceSpawners);
      checkArray(spawnGroupEntry.Value.PriorityGroupSpawners);
      checkArray(spawnGroupEntry.Value.GroupSpawners);
   }
   checkArray(_weightedChanceSpawners);
}

bool FTATSpawnerHelper::FSpawnerEntry::operator<(const FSpawnerEntry& other) const
{
   return MakeTuple(Priority, Hash, Index) < MakeTuple(other.Priority, other.Hash, other.Index);
}

TArray<FTATSpawnerHelper::FSpawnerEntry>& FTATSpawnerHelper::FSpawnGroupSection::ChooseArrayForAdd(const FTATSceneRequirement& requirement, const FTATSceneSpawnerOverride & override)
{
   if (requirement.IsNone())
   {
      return GroupSpawners;
   }

   // Group spawners associated with a scene are higher priority
   return override.IsDefault() ? PriorityGroupSpawners : ChanceSpawners;
}
