// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Damage/TATDamageTypes.h"
#include "Tools/TATToolComponent.h"

#include "TATRangedWeaponToolComponent.generated.h"

class ATATRangedWeaponProjectile;
class UAnimMontage;

/// A base tool that holds the information for a ranged weapon that shoots projectiles (e.g. bow)
UCLASS()
class TAT_API UTATRangedWeaponToolComponent : public UTATToolComponent
{
   GENERATED_BODY()
public:
   UTATRangedWeaponToolComponent();

   /// The projectile that we launch on casting
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Weapon")
   TSoftClassPtr<ATATRangedWeaponProjectile> RangedProjectile;

   /// The animation we play for casting
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Weapon")
   UAnimMontage* CastAnimation;

   /// How fast should the projectile move
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Weapon")
   float ProjectileSpeed = 600.0f;

   /// How much should the projectile be affected by gravity
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Weapon", Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float InitialGravityScale = 0.0f;

   /// How much damage does the projectile do on impact
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Weapon")
   FTATScalableDamageWithType ProjectileDamage;
};

