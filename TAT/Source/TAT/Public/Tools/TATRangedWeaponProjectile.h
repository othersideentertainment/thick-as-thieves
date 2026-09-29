// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Damage/TATDamageTypes.h"
#include "Items/Throwable/TATProjectile.h"
#include "Abilities/TATGameplayEffectSetByCallerParam.h"

#include "TATRangedWeaponProjectile.generated.h"

class UGameplayEffect;

/// A projectile to be used by a ranged weapon
UCLASS()
class TAT_API ATATRangedWeaponProjectile : public ATATProjectile
{
   GENERATED_BODY()
public:

   virtual void BeginPlay() override;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true))
   FTATDamageWithType Damage;

   /// A gameplay effect to apply to a hit target
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TSubclassOf<UGameplayEffect> HitGameplayEffect;

   /// SetByCaller parameters to pass to the HitGameplayEffect
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   TArray<FTATGameplayEffectSetByCallerParam> HitGameplayEffectSetByCallerParams;

   /// Called when we stop moving, e.g. when we hit something
   /// Damage is done natively, so this can be for additional custom effects
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnProjectileStop(const FHitResult& impact);

protected:
   /// Called when the projectile stops moving, or can be called manually if there is custom logic in child classes
   /// (e.g. extra colliders)
   UFUNCTION(BlueprintCallable)
   void ProjectileHasHitObject(const FHitResult& impact);
};

