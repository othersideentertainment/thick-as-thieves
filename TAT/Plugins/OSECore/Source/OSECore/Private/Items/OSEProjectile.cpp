// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/OSEProjectile.h"

// ose

// ue4
#include "Net/UnrealNetwork.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEProjectile)

//////////////////////////////////////////////////////////////////////////
///            AOSEProjectile
//////////////////////////////////////////////////////////////////////////

AOSEProjectile::AOSEProjectile()
   : Super()
{
   bReplicates = true;
   SetReplicateMovement(true);

   // Use a sphere as a simple collision representation
   CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
   CollisionComp->InitSphereRadius(5.0f);
   CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
   CollisionComp->OnComponentHit.AddDynamic(this, &AOSEProjectile::_OnHit); // set up a notification for when this component hits something blocking

   // Players can't walk on it
   CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
   CollisionComp->CanCharacterStepUpOn = ECB_No;

   // Set as root component
   RootComponent = CollisionComp;

   // Use a ProjectileMovementComponent to govern this projectile's movement
   ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
   ProjectileMovement->UpdatedComponent = CollisionComp;
   ProjectileMovement->InitialSpeed = 0.0f;
   ProjectileMovement->MaxSpeed = 3000.f;
   ProjectileMovement->bRotationFollowsVelocity = true;
   ProjectileMovement->bShouldBounce = false;
   ProjectileMovement->bInterpMovement = true;

   // Die after 3 seconds by default
   InitialLifeSpan = 3.0f;
}

void AOSEProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(AOSEProjectile, InitialVelocity, COND_InitialOnly);
}

void AOSEProjectile::BeginPlay()
{
   Super::BeginPlay();

   if (!InitialVelocity.IsZero())
      ProjectileMovement->Velocity = InitialVelocity;

   // Ignore collision with instigator and vice versa
   if (AActor* instigator = GetInstigator())
   {
      CollisionComp->IgnoreActorWhenMoving(instigator, true);
      if (auto theirPrimitive = Cast<UPrimitiveComponent>(instigator->GetRootComponent()))
      {
         theirPrimitive->IgnoreActorWhenMoving(this, true);
      }
   }
   else if (AActor* owner = GetOwner())
   {
      // For when there are non-pawn instigators, could add flag if even not desired
      CollisionComp->IgnoreActorWhenMoving(owner, true);
   }
}

void AOSEProjectile::_OnHit(UPrimitiveComponent* hitComp, AActor* otherActor, UPrimitiveComponent* otherComp, FVector normalImpulse, const FHitResult& hit)
{
   // Only add impulse and destroy projectile if we hit a physics
   if ((otherActor != nullptr) && (otherActor != this) && (otherComp != nullptr) && otherComp->IsSimulatingPhysics())
   {
      otherComp->AddImpulseAtLocation(GetVelocity() * HitVelocityImpulseMultiplier, GetActorLocation());
      Destroy();
   }
}

void AOSEProjectile::PostNetReceiveLocationAndRotation()
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

