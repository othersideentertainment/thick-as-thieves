// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

// ose
#include "Items/Throwable/TATProjectile.h"

#include "TATExplodingProjectile.generated.h"

class UTATParticleSimulationComponent;


/// A projectile that can explode (before or after impact) into several pieces, using a particle simulation to simulate the trails for each of the pieces
UCLASS()
class TAT_API ATATExplodingProjectile : public ATATProjectile
{
   GENERATED_BODY()

public:
   ATATExplodingProjectile();

   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Exploding Projectile")
   TObjectPtr<UTATParticleSimulationComponent> ExplosionSimComponent;

   /// The minimum velocity of explosion particles
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Exploding Projectile", Meta = (UIMin = "0", ClampMin = "0"))
   float MinExplosionVelocity = 1500.0f;

   /// The initial particle velocity range is this value multiplied by the projectile's velocity
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Exploding Projectile")
   FFloatInterval ExplosionParticleInitialVelocityMultiplier = FFloatInterval(0.8f, 1.2f);

   /// Trigger the explosion after this amount of time if the projectile hasn't hit anything yet. Set to 0 to disable.
   UPROPERTY(Replicated, BlueprintReadOnly, EditAnywhere, Category = "Exploding Projectile", meta = (ExposeOnSpawn, UIMin = "0.0", ForceUnits = "seconds"))
   float ExplosionDelayTime = 2.0f;

   UFUNCTION(BlueprintCallable, Category = "Exploding Projectile")
   bool HasExplosionStarted() const;

   UFUNCTION(BlueprintCallable, Category = "Exploding Projectile")
   void TriggerExplosion(FVector direction);

   UFUNCTION(BlueprintNativeEvent, Category = "Exploding Projectile")
   void OnExplosionStart();

   UFUNCTION(BlueprintNativeEvent, Category = "Exploding Projectile")
   void OnExplosionHit(int32 particleIndex, const FHitResult& impactResult);

protected:
   FVector _GetProjectileMovementDirection() const;

   UFUNCTION()
   void _OnExplosionHit(int32 particleIndex, const FHitResult& hitResult);

   UFUNCTION()
   void _OnProjectileStop(const FHitResult& impactResult);

   UFUNCTION()
   void _OnExplodeTimer();

   bool _explosionTriggered = false;

   FTimerHandle _explodeTimer;

};
