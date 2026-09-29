// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue 
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "TATRespawnAreaSpawnZoneVolume.generated.h"


class UBoxComponent;
class UOSEShapeCollisionTrackerComponent;

// Used alongside TATRespawnAreaOverlapVolume to act as a warning of AI actors within it. Presented to the players on the HUD.
UCLASS()
class TAT_API ATATRespawnAreaSpawnZoneVolume : public AActor
{
   GENERATED_BODY()

public:
   ATATRespawnAreaSpawnZoneVolume();
   virtual void BeginPlay() override;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpawnZoneAITrackedCountChanged);
   
   UPROPERTY(BlueprintAssignable)
   FOnSpawnZoneAITrackedCountChanged OnSpawnZoneAITrackedCountChanged;
   
   int GetTrackedAICharactersWithinShape() const { return _trackedAICharactersWithinShape; }

   FVector GetRespawnMarkerLocation() const;
   
protected:
   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);
   UFUNCTION()
   void _AuthorityOnActorExitShape(AActor* actor);
   
   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent { nullptr };

   UPROPERTY(EditAnywhere)
   UBoxComponent* _boxShapeCollision { nullptr };

   UPROPERTY(EditAnywhere, meta = (MakeEditWidget))
   FVector _respawnMarkerOffset;
   
   int _trackedAICharactersWithinShape { 0 };
   
};
