// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Variation/TATSpawnerFwd.h"
#include "Variation/TATSpawnerOwnerInterface.h"

// ue5
#include "GameFramework/Actor.h"

#include "TATDestroySpawner.generated.h"

class UTATDestroySpawnerVisualizationComponent;


/// A spawner actor that destroys a set of external actors if its spawner does not spawn
///
/// This is _a_ way of controlling the existence of a set of actors in a level via a spawner.
/// It only directly replicates if it destroys an actor that is not itself replicated.
/// 
/// * It is up to the external actor to not cause an issue if destroyed (e.g. leave a hole in
///   the navmesh), but many simple usecases may not have to do anything special. Remember
///   that TATMissionAwareObstacle exists.
/// * Using this to destroy actors that have active spawners may have unpredictable results
UCLASS(HideCategories = (Rendering, Replication, Collision, HLOD, Input, Physics))
class TAT_API ATATDestroySpawner : public AActor, public ITATSpawnerOwnerInterface
{
   GENERATED_BODY()
   
public:	
   // Sets default values for this actor's properties
   ATATDestroySpawner();

   // Actors that will be destroyed if the spawner does not spawn, and will be left if it does
   UPROPERTY(EditInstanceOnly, Category = "Spawner")
   TArray<TObjectPtr<AActor>> ActorsToKeepOnSpawn;

#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif

protected:
   // from AActor
   virtual void PostInitializeComponents() override;

protected:
   UFUNCTION()
   void _AuthorityOnSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   UFUNCTION()
   void _OnRep_DidNotSpawn();
   
   UPROPERTY(VisibleAnywhere, Category = "Spawner")
   TObjectPtr<UTATSpawnerComponent> _spawnerComponent = nullptr;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_DidNotSpawn)
   bool _didNotSpawn;

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TObjectPtr<UTATDestroySpawnerVisualizationComponent> _visualizationComponent;
#endif
};

UCLASS(ClassGroup = Debug, NotBlueprintable, NotPlaceable)
class TAT_API UTATDestroySpawnerVisualizationComponent : public UPrimitiveComponent
{
   GENERATED_BODY()

public:
   UTATDestroySpawnerVisualizationComponent();

protected:
   //~ Begin UPrimitiveComponent Interface
#if WITH_EDITOR
   virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
   virtual FBoxSphereBounds CalcBounds(const FTransform& localToWorld) const override;
   virtual bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }
#endif
   //~ End UPrimitiveComponent Interface
};
