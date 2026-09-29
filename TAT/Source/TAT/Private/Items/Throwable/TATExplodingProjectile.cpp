// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/Throwable/TATExplodingProjectile.h"

// tat
#include "Tools/TATParticleSimulationComponent.h"

// ue
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATExplodingProjectile)


ATATExplodingProjectile::ATATExplodingProjectile()
{
   ExplosionSimComponent = CreateDefaultSubobject<UTATParticleSimulationComponent>(TEXT("ExplosionSimComponent"));
   ExplosionSimComponent->AutoStartSimulation = false;

   if (CollisionComp)
   {
      ExplosionSimComponent->SetupAttachment(CollisionComp);
   }

   if (ProjectileMovement)
   {
      ProjectileMovement->bShouldBounce = false;
   }
}

void ATATExplodingProjectile::BeginPlay()
{
   Super::BeginPlay();

   if (ensure(ExplosionSimComponent))
   {
      ExplosionSimComponent->OnParticleHit.AddDynamic(this, &ATATExplodingProjectile::_OnExplosionHit);
   }

   if (UProjectileMovementComponent* projMovement = GetProjectileMovement())
   {
      projMovement->OnProjectileStop.AddDynamic(this, &ATATExplodingProjectile::_OnProjectileStop);
   }

   if (ExplosionDelayTime > 0)
   {
      constexpr bool loop = false;
      GetWorldTimerManager().SetTimer(_explodeTimer, FTimerDelegate::CreateUObject(this, &ATATExplodingProjectile::_OnExplodeTimer), ExplosionDelayTime, loop);
   }
}

void ATATExplodingProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATExplodingProjectile, ExplosionDelayTime, COND_InitialOnly);
}

bool ATATExplodingProjectile::HasExplosionStarted() const
{
   return _explosionTriggered;
}

void ATATExplodingProjectile::TriggerExplosion(FVector direction)
{
   if (!ensure(ExplosionSimComponent))
   {
      return;
   }

   UProjectileMovementComponent* projMovement = GetProjectileMovement();
   if (!ensure(projMovement))
   {
      return;
   }

   GetWorldTimerManager().ClearTimer(_explodeTimer);

   // Compute particle velocity _before_ we set the projectile to inactive and turn off collision
   const float baseParticleVelocity = FMath::Max3(0.0f, MinExplosionVelocity, static_cast<float>(projMovement->Velocity.Length()));
   const FFloatInterval particleVelocityRange{
      baseParticleVelocity * ExplosionParticleInitialVelocityMultiplier.Min,
      baseParticleVelocity * ExplosionParticleInitialVelocityMultiplier.Max,
   };

   projMovement->SetActive(false);

   if (USphereComponent* projCollision = GetCollisionComp())
   {
      projCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   }

   _explosionTriggered = true;

   ExplosionSimComponent->StartSimulation(direction, particleVelocityRange);

   OnExplosionStart();
}

void ATATExplodingProjectile::OnExplosionStart_Implementation()
{

}

void ATATExplodingProjectile::OnExplosionHit_Implementation(int32 particleIndex, const FHitResult& impactResult)
{

}

FVector ATATExplodingProjectile::_GetProjectileMovementDirection() const
{
   if (UProjectileMovementComponent* projMovement = GetProjectileMovement())
   {
      return projMovement->Velocity.GetSafeNormal();
   }
   return GetActorQuat().GetForwardVector();
}

void ATATExplodingProjectile::_OnExplosionHit(int32 particleIndex, const FHitResult& hitResult)
{
   OnExplosionHit(particleIndex, hitResult);
}

void ATATExplodingProjectile::_OnProjectileStop(const FHitResult& impactResult)
{
   if (_explosionTriggered)
   {
      return;
   }

   // We hit something before exploding - pass that along in case we want to apply any effects
   OnExplosionHit(-1, impactResult);

   // Exploding on impact
   TriggerExplosion(impactResult.Normal);
}

void ATATExplodingProjectile::_OnExplodeTimer()
{
   if (_explosionTriggered)
   {
      return;
   }

   // Exploding in mid-air before a collision - use a forward-facing direction
   TriggerExplosion(_GetProjectileMovementDirection());
}
