// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATRespawnMarkerLocationInterface.h"

// ue 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATRespawnAreaOverlapVolume.generated.h"

class ATATRespawnAreaSpawnZoneVolume;
class UBoxComponent;
class UOSEShapeCollisionTrackerComponent;
class ATATRespawnAreaSpawnLocation;


// Used to define custom respawn locations for players within this volume.
UCLASS()
class TAT_API ATATRespawnAreaOverlapVolume : public AActor, public ITATRespawnMarkerLocationInterface
{
   GENERATED_BODY()

public:
   ATATRespawnAreaOverlapVolume();

   virtual void BeginPlay() override;
   

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIDetectedWithinSafeZoneChanged,
      ATATRespawnAreaOverlapVolume*, volume,
      bool, detected);
   
   UPROPERTY(BlueprintAssignable)
   FOnAIDetectedWithinSafeZoneChanged OnAIDetectedWithinSafeZoneChanged;
   
   bool IsAIDetectedWithinSafeZone() const { return _aiDetectedWithinSafeZone; }
   
   ATATRespawnAreaSpawnLocation* GetValidPlayerStartForVolume(const APawn* pawnToSpawn) const;
   int GetPriority() const { return _priority; }

   virtual FVector GetRespawnMarkerLocation() const override;
   
protected:
   
   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent { nullptr };

   UPROPERTY(EditAnywhere)
   UBoxComponent* _boxShapeCollision { nullptr };
   
   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);   
   UFUNCTION()
   void _AuthorityOnActorExitShape(AActor* actor);

   UFUNCTION()
   void _AuthorityOnTrackedAICountChanged();
      
   UPROPERTY(EditInstanceOnly)
   int _priority { 0 };
   
   UPROPERTY(EditInstanceOnly)
   TArray<TObjectPtr<ATATRespawnAreaSpawnLocation>> _playerSpawnPoints; 
   
   UPROPERTY(Transient)
   bool _aiDetectedWithinSafeZone { false };
   
   UPROPERTY(EditInstanceOnly)
   TObjectPtr<ATATRespawnAreaSpawnZoneVolume> _spawnZoneVolume { nullptr };
};
