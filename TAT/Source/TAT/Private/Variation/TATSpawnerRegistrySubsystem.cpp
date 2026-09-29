// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATSpawnerRegistrySubsystem.h"

// tat
#include "Variation/TATSpawnerComponent.h"

// ue5
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpawnerRegistrySubsystem)

bool UTATSpawnerRegistrySubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if (!Super::ShouldCreateSubsystem(outer))
   {
      return false;
   }

   // don't create this on clients
   UWorld* world = CastChecked<UWorld>(outer);
   return !world->IsNetMode(NM_Client);
}

void UTATSpawnerRegistrySubsystem::AuthorityRegisterSpawner(UTATSpawnerComponent* spawner)
{
   check(spawner);
   check(!_spawners.Contains(spawner));
   _spawners.Add(spawner);
}

#if WITH_EDITOR
void UTATSpawnerRegistrySubsystem::SetEditorVisSpawnerDependency(UActorComponent* child, AActor* parent)
{
   check(child);
   FSpawnerId id(child->GetOwner(), child->GetFName());

   const TWeakObjectPtr<AActor> oldParent = _editorVisSpawnerDependencyMap.FindRef(id);
   if (oldParent == parent) return;

   _RemoveFromReverseDependencies(id);

   if (parent)
   {
      _editorVisSpawnerDependencyMap.Add(id, parent);
      _editorVisReverseDependencyMap.FindOrAdd(parent).AddUnique(id);
   }
   else
   {
      _editorVisSpawnerDependencyMap.Remove(id);
   }
}

void UTATSpawnerRegistrySubsystem::RemoveEditorVisSpawner(UActorComponent* spawner)
{
   FSpawnerId id(spawner->GetOwner(), spawner->GetFName());
   _RemoveFromReverseDependencies(id);
   _editorVisSpawnerDependencyMap.Remove(id);
   _editorVisReverseDependencyMap.Remove(spawner->GetOwner());
}

const AActor* UTATSpawnerRegistrySubsystem::GetEditorVisDependency(const UActorComponent* childSpawner) const
{
   FSpawnerId id(childSpawner->GetOwner(), childSpawner->GetFName());
   return _editorVisSpawnerDependencyMap.FindRef(id).Get();
}

void UTATSpawnerRegistrySubsystem::ForEachDependentSpawnerActor(const AActor * parent, TFunctionRef<void(const AActor*)> handler) const
{
   if (const TArray<FSpawnerId>* dependingSpawners = _editorVisReverseDependencyMap.Find(parent))
   {
      for (const FSpawnerId& id : *dependingSpawners)
      {
         if (const AActor* child = id.Key.Get())
         {
            handler(child);
         }
      }
   }
}

void UTATSpawnerRegistrySubsystem::AddSpawnRates(const TMap<TWeakObjectPtr<UTATSpawnerComponent>, int32>& spawnCounts, int simulationCount)
{
   _editorVisSpawnChanceCache.Reset();
   for (const TPair<TWeakObjectPtr<UTATSpawnerComponent>, int32>& pair : spawnCounts)
   {
      const UTATSpawnerComponent* spawner = pair.Key.Get();
      if (!ensure(spawner) || !spawner->GetOwner())
      {
         continue;
      }

      if (!spawner->IsPrimarySpawnerForActor())
      {
         continue;
      }

      const float chance = 100 * pair.Value / static_cast<float>(simulationCount * FMath::Max(pair.Key->GetMaxInstancesToSpawn(), 1));

      _editorVisSpawnChanceCache.Add(spawner->GetOwner()->GetActorGuid(), chance);
   }
}

void UTATSpawnerRegistrySubsystem::AddExtraSpawnRates(const TMap<TWeakObjectPtr<UActorComponent>, int32>& spawnCounts, int simulationCount)
{
   for (const TPair<TWeakObjectPtr<UActorComponent>, int32>& pair : spawnCounts)
   {
      const UActorComponent* spawner = pair.Key.Get();
      if (!ensure(spawner) || !spawner->GetOwner())
      {
         continue;
      }

      const float chance = 100 * pair.Value / static_cast<float>(simulationCount);
      _editorVisSpawnChanceCache.Add(spawner->GetOwner()->GetActorGuid(), chance);
   }
}

TOptional<float> UTATSpawnerRegistrySubsystem::GetLastSimulatedSpawnRate(const AActor* actor) const
{
   check(actor);
   if (const float* result = _editorVisSpawnChanceCache.Find(actor->GetActorGuid()))
   {
      return *result;
   }
   return NullOpt;
}

void UTATSpawnerRegistrySubsystem::_RemoveFromReverseDependencies(const FSpawnerId& id)
{
   const TWeakObjectPtr<AActor> oldParent = _editorVisSpawnerDependencyMap.FindRef(id);
   if (oldParent.IsValid())
   {
      if (TArray<FSpawnerId>* reverseEntries = _editorVisReverseDependencyMap.Find(oldParent))
      {
         reverseEntries->RemoveSingleSwap(id);
      }
   }
}

#endif
