// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolRangedWeaponComponent.h"

// ose
#include "Items/OSEProjectile.h"

// ue4
#include "GameFramework/ProjectileMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolRangedWeaponComponent)

DEFINE_LOG_CATEGORY(LogOSERangedWeaponComponent);

//////////////////////////////////////////////////////////////////////////
///            FOSERangedWeaponConfig
//////////////////////////////////////////////////////////////////////////

AOSEProjectile* FOSERangedWeaponConfig::TryGetProjectileCDO() const
{
   if (IsValid(ProjectileClass))
   {
      return Cast<AOSEProjectile>(ProjectileClass->GetDefaultObject());
   }
   return nullptr;
}

//////////////////////////////////////////////////////////////////////////
///            UToolRangedWeaponComponent
//////////////////////////////////////////////////////////////////////////

UToolRangedWeaponComponent::UToolRangedWeaponComponent()
   : Super()
{
}

void UToolRangedWeaponComponent::BeginPlay()
{
   Super::BeginPlay();

   // ensure we don't start with more than the # we can have in the mag
   ProjectilesLoaded = FMath::Min(ProjectilesLoaded, Config.ProjectilesInMagazine);
   OnProjectileCountChanged(ProjectilesLoaded, ProjectileCapacity);

   // calculate and store max range
   _CalculateMaxRange();
}

float UToolRangedWeaponComponent::GetProjectileSpeed() const
{
   // ported this behavior, seems weird.

   if (!FMath::IsNearlyZero(Config.ProjectileSpeed))
   {
      return Config.ProjectileSpeed;
   }

   if (AOSEProjectile* defaultProjectile = Config.TryGetProjectileCDO())
   {
      return defaultProjectile->GetProjectileMovement()->InitialSpeed;
   }
   return 0.0f;
}

bool UToolRangedWeaponComponent::HasRequiredProjectilesForActivation() const
{
   return ProjectilesLoaded >= Config.ProjectilesPerActivation;
}

bool UToolRangedWeaponComponent::CanReload() const
{
   // no reload if we don't have anything to reload
   if (ProjectileCapacity == 0)
   {
      return false;
   }

   // no reload if we're already full
   if (ProjectilesLoaded >= Config.ProjectilesInMagazine)
   {
      return false;
   }

   // otherwise we have some capacity to reload and some space to put it, so sure
   return true;
}

bool UToolRangedWeaponComponent::NeedsReload() const
{
   // do we have enough projectiles to activate a shot?
   if (!HasRequiredProjectilesForActivation())
   {
      return CanReload();
   }
   return false;
}

float UToolRangedWeaponComponent::GetGravityZ() const
{
   if (AOSEProjectile* defaultProjectile = Config.TryGetProjectileCDO())
   {
      return defaultProjectile->GetProjectileMovement()->GetGravityZ();
   }
   return 0.0f;
}

bool UToolRangedWeaponComponent::TargetWithinRange(FVector targetLocation) const
{
   if (AActor* owner = UToolRangedWeaponComponent::GetOwner())
   {
      const float distance = (owner->GetActorLocation() - targetLocation).Size();
      if (distance <= _maxRange && distance >= Config.MinRange)
      {
         return true;
      }
   }
   return false;
}

void UToolRangedWeaponComponent::DeductProjectilesForActivation()
{
   ProjectilesLoaded = FMath::Max(ProjectilesLoaded - Config.ProjectilesPerActivation, 0);
   OnProjectileCountChanged(ProjectilesLoaded, ProjectileCapacity);
}

void UToolRangedWeaponComponent::DoReload()
{
   if (CanReload())
   {
      // for now we're just reloading the whole magazine
      // TODO: reload each projectile w/ an animation montage driving the +1 projectile state?
      const int numProjectilesNeededToFill = Config.ProjectilesInMagazine - ProjectilesLoaded;
      const int numProjectilesToLoad = FMath::Min(numProjectilesNeededToFill, ProjectileCapacity);
      ProjectileCapacity -= numProjectilesToLoad;
      ProjectilesLoaded += numProjectilesToLoad;
      check(ProjectileCapacity >= 0);
      OnProjectileCountChanged(ProjectilesLoaded, ProjectileCapacity);
   }
}

void UToolRangedWeaponComponent::_CalculateMaxRange()
{
   // Assumptions: Actor is standing on the ground, shooting parallel to the ground. This does not take into account arching shots.
   // If we do want to arch shots in the future, then we should instead calculate range on demand instead of trying to pre-determine it.

   // First we calculate how long it will take for the projectile to hit the ground when fired horizontally:
   //    h = 0.5(at^2)           - h = height of projectile fire, a = gravity
   //    t = sqrt((2.0 * h)/a)   - Solve for t
   //
   // Then we calculate how far the projectile will fly:
   //    d = v*t                 - t = time till it hits the ground, v = firing velocity


   // 1) Calculate Time to ground:
   float timeInFlight = 0.0;

   float projectileGravity = FMath::Abs(GetGravityZ());
   if (projectileGravity > 0.0)
   {
      float firingHeight = 157.7; // Current eye height of our ranged guards in cm. Is there a saner default?
      if (const APawn* owner = Cast<APawn>(UToolRangedWeaponComponent::GetOwner()))
      {
         firingHeight = owner->BaseEyeHeight + owner->GetSimpleCollisionHalfHeight();
      }

      timeInFlight = firingHeight >= 0.0 ? FMath::Sqrt((2.0 * firingHeight) / projectileGravity) : 0.0f;
   }
   else if (AOSEProjectile* projectile = Config.TryGetProjectileCDO())
   {
      // If a Projectile does not have gravity, use it's life span as timeInFlight.
      timeInFlight = projectile->InitialLifeSpan;
   }

   // 2) Calculate max range:
   _maxRange = GetProjectileSpeed() * timeInFlight;
   UE_LOG(LogOSERangedWeaponComponent, Verbose, TEXT("Ranged Weapon Max Range: %f"), _maxRange);
}

