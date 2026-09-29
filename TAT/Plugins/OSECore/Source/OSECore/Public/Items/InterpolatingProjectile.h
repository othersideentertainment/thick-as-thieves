// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InterpolatingProjectile.generated.h"

class UProjectileMovementComponent;

// An actor that allows interpolating from network updates with fewer assumptions
UCLASS()
class OSECORE_API AInterpolatingProjectile : public AActor
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   AInterpolatingProjectile();

   UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

   virtual void PostNetReceiveLocationAndRotation() override;

protected:
   /// Projectile movement component
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
   UProjectileMovementComponent* ProjectileMovement = nullptr;
};
