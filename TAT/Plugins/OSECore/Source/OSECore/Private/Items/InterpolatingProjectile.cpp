// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/InterpolatingProjectile.h"

// ue4
#include "GameFramework/ProjectileMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InterpolatingProjectile)

// Sets default values
AInterpolatingProjectile::AInterpolatingProjectile()
{
   ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
}

void AInterpolatingProjectile::PostNetReceiveLocationAndRotation()
{
   if (ProjectileMovement && ProjectileMovement->UpdatedComponent && ProjectileMovement->bInterpMovement)
   {
      const FRepMovement& constRepMovement = GetReplicatedMovement();
      const FVector newLocation = FRepMovement::RebaseOntoLocalOrigin(constRepMovement.Location, this);
      ProjectileMovement->MoveInterpolationTarget(newLocation, constRepMovement.Rotation);
      //DrawDebugSphere(GetWorld(), newLocation, 16, 16, FColor::Orange, false);
   }
   else
   {
      Super::PostNetReceiveLocationAndRotation();
   }
}

