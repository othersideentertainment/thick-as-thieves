// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Traversal/TATTeleportUtilities.h"

// ue4
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATTeleportBladeMarker.generated.h"

UCLASS()
class TAT_API ATATTeleportBladeMarker : public AActor
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATATTeleportBladeMarker();

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

public:   
   // Called every frame
   virtual void Tick(float deltaTime) override;

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category=Teleport)
   bool FindTeleportLocation(FVector& outLocation) const;

   UFUNCTION(BlueprintCallable, Category = Teleport)
   static bool HasTeleportLocationForMarker(const AActor* teleportingActor, TSubclassOf<ATATTeleportBladeMarker> markerClass, const FTransform& markerTransform);

private:
   void _CheckDespawnDistance(const AActor* instigator);
   void _OnExceededDespawnDistance(const AActor* instigator);

private:
   UPROPERTY(EditDefaultsOnly, Category = Teleport)
   FTATTeleportQuerySettings _teleportSettings;

   // Distance from the player that the marker despawns itself (if already placed)
   UPROPERTY(EditDefaultsOnly, Category = Range, Meta=(Unit=Cm))
   float _despawnDistance;

   UPROPERTY(EditDefaultsOnly, Category = Range, Meta = (Categories = "GameplayCue"))
   FGameplayTag _exceededDespawnDistanceTag;
};
