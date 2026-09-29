// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

//ue5
#include "Subsystems/WorldSubsystem.h"

#include "TATSpawnerRegistrySubsystem.generated.h"

class UTATSpawnerComponent;

/// A subsystem that keeps track of MissionSpawner components in the world, so they can be used later
UCLASS()
class TAT_API UTATSpawnerRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()


public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;

   void AuthorityRegisterSpawner(UTATSpawnerComponent* spawner);

   const TArray<UTATSpawnerComponent*>& GetSpawners() const { return _spawners; }
	
#if WITH_EDITOR
   void SetEditorVisSpawnerDependency(UActorComponent* child, AActor* parent);
   void RemoveEditorVisSpawner(UActorComponent* spawner);
   const AActor* GetEditorVisDependency(const UActorComponent* childSpawner) const;
   void ForEachDependentSpawnerActor(const AActor* parent, TFunctionRef<void(const AActor*)> handler) const;

   void AddSpawnRates(const TMap<TWeakObjectPtr<UTATSpawnerComponent>, int32>& spawnCounts, int simulationCount);
   void AddExtraSpawnRates(const TMap<TWeakObjectPtr<UActorComponent>, int32>& spawnCounts, int simulationCount);
   TOptional<float> GetLastSimulatedSpawnRate(const AActor* actor) const;
#endif

private:

   UPROPERTY(Transient)
   TArray<UTATSpawnerComponent*> _spawners;

#if WITH_EDITORONLY_DATA
   using FSpawnerId = TTuple< TWeakObjectPtr<AActor>, FName>;

#if WITH_EDITOR
   void _RemoveFromReverseDependencies(const FSpawnerId& id);
#endif

   TMap<FSpawnerId, TWeakObjectPtr<AActor>> _editorVisSpawnerDependencyMap;
   TMap<TWeakObjectPtr<const AActor>, TArray<FSpawnerId>> _editorVisReverseDependencyMap;

   TMap<FGuid, float> _editorVisSpawnChanceCache;
#endif
};
