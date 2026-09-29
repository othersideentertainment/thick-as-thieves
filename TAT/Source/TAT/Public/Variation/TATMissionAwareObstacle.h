// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// tat
#include "Variation/TATSpawnerFwd.h"
#include "Variation/TATSpawnerOwnerInterface.h"

// ue4
#include "GameFramework/Actor.h"

#include "TATMissionAwareObstacle.generated.h"

class UTATSpawnerComponent;

UCLASS(Blueprintable, BlueprintType, HideCategories = (Lighting, LightColor, Force, Collision, Rendering, Replication, Input, LOD, Cooking, Actor, "Actor Tick"))
class TAT_API ATATMissionAwareObstacle : public AActor, public ITATSpawnerOwnerInterface
{
   GENERATED_BODY()

public:
   ATATMissionAwareObstacle();
   
   UFUNCTION(BlueprintPure, Category = "Actor Spawner")
   UTATSpawnerComponent* GetSpawnerComponent() const { return _spawnerComponent; }

protected:
   // from AActor
   virtual void PostInitializeComponents() override;

#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR

private:
   UFUNCTION()
   void _AuthorityOnMissionSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   UFUNCTION()
   void _AuthorityOnMissionSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

protected:
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actor Spawner")
   UTATSpawnerComponent* _spawnerComponent = nullptr;
};
