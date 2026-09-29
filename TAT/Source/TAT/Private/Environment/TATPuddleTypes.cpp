// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATPuddleTypes.h"

// tat
#include "Environment/TATPuddleUtilities.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPuddleTypes)


bool FTATPuddle::CanApplyDamage(float damageAmount) const
{
   if (damageAmount == 0 || IsPendingRemove())
   {
      return false;
   }
   const float newHealth = FMath::Max(0.0f, Health - damageAmount);
   return !FMath::IsNearlyEqual(newHealth, Health);
}

bool FTATPuddle::ApplyDamage(float damageAmount, double worldTimeSeconds, float lifetimeAfterRemove)
{
   const float newHealth = FMath::Max(0.0f, Health - damageAmount);
   const bool newPendingRemove = ServerRemoveWorldTime <= 0 && Health > 0 && newHealth <= 0;

   Health = newHealth;

   if (newPendingRemove)
   {
      ServerRemoveWorldTime = worldTimeSeconds + lifetimeAfterRemove;
   }

   return newPendingRemove;
}

FVector FTATPuddle::GetPositionAroundPuddle(float angleDeg, float extentMultiplier) const
{
   return UTATPuddleUtilities::GetPositionAroundPuddle(Location, Extent, Rotation, angleDeg, extentMultiplier);
}

// ================================================================================================================

bool FTATPuddleGameplayEffect::CanApplyToTarget(UAbilitySystemComponent* asc, const AActor* owner) const
{
   return UTATPuddleUtilities::TargetMatchesPuddleFilter(owner, TargetFilter);
}
