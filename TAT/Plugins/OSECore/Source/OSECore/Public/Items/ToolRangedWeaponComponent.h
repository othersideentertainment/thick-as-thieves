// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Items/ToolComponent.h"

#include "ToolRangedWeaponComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOSERangedWeaponComponent, Log, All);

class AOSEProjectile;
class UToolRangedWeaponAmmo;

USTRUCT(BlueprintType)
struct FOSERangedWeaponConfig
{
   GENERATED_BODY()

public:
   // What class is the projectile we spawn?
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   TSubclassOf<AOSEProjectile> ProjectileClass;
   AOSEProjectile* TryGetProjectileCDO() const;

   // Minimum distance for weapon firing.
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float MinRange = 200.0f;

   // TODO - Is EngagementDistance deprecated? Are we using it anywhere? If not, remove.
   // Appropriate distance to attack from. Advisory to AI only; no effect on gun behavior.
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float EngagementDistance = 3000.0f;

   // How many projectiles are spawned at a time?
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   int ProjectilesPerActivation = 1;

   // How many times per input do we spawn a group of projectiles?
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   int BurstsPerActivation = 1;

   // If BurstsPerActivation>1, what is the timing between them? (seconds)
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float TimeBetweenBursts = 1.0f;

   // Inaccuracy in aim for any given pellet, in degrees
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float SpreadAngle = 0.0f;

   // Number of shots before gun must be reloaded.
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   int ProjectilesInMagazine = 10;

   // Projectile speed override, in cm/sec
   UPROPERTY(BlueprintReadWrite, EditAnywhere)
   float ProjectileSpeed = 4000.0f;
};

UCLASS(ClassGroup = (Tools), Abstract, Blueprintable, BlueprintType, meta = (IsBlueprintBase = "true"))
class OSECORE_API UToolRangedWeaponComponent : public UToolComponent
{
   GENERATED_BODY()

public:
   UToolRangedWeaponComponent();

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (TitleProperty = ProjectileClass))
   FOSERangedWeaponConfig Config;

   // from UActorComponent
   virtual void BeginPlay() override;

   // TODO: Replicate projectile state

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
   int ProjectilesLoaded = 0;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
   int ProjectileCapacity = 0;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   float GetProjectileSpeed() const;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   bool HasRequiredProjectilesForActivation() const;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   bool CanReload() const;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   bool NeedsReload() const;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   float GetGravityZ() const;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   bool TargetWithinRange(FVector targetLocation) const;

   UFUNCTION(BlueprintPure, Category = "Weapon")
   float GetMaxRange() const { return _maxRange; }

   // called by abilities managing the projectiles on this component
   void DeductProjectilesForActivation();
   void DoReload();

protected:
   UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
   void OnProjectileCountChanged(int loaded, int capacity);
   void OnProjectileCountChanged_Implementation(int loaded, int capacity) { };

private:
   // Determined on component begin play.
   float _maxRange = 0.0f;

   void _CalculateMaxRange();
};
