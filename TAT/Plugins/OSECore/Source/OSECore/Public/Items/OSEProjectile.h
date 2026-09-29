// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue
#include "GameplayTagContainer.h"

#include "OSEProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS(ClassGroup = (Tools), BlueprintType, Blueprintable)
class OSECORE_API AOSEProjectile : public AActor
{
   GENERATED_BODY()

public:
   AOSEProjectile();
   
   // from AActor
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // access
   USphereComponent* GetCollisionComp() const { return CollisionComp; }
   UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

protected:
   /// Called when projectile's collision component hits something
   UFUNCTION()
   void _OnHit(UPrimitiveComponent* hitComp, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit);

   // from AActor
   virtual void PostNetReceiveLocationAndRotation() override;

protected:
   /// Sphere collision component
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   USphereComponent* CollisionComp;

   /// Projectile movement component
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   UProjectileMovementComponent* ProjectileMovement;

   /// Projectile movement component
   UPROPERTY(EditDefaultsOnly, Category = "Projectile")
   float HitVelocityImpulseMultiplier = 100.0f;
   UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere, meta = (ExposeOnSpawn = true), Category = "Projectile")
   FVector InitialVelocity;
};
