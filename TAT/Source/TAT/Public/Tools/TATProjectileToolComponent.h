// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/TATToolComponent.h"

#include "TATProjectileToolComponent.generated.h"

class ATATProjectile;
class UAnimMontage;

UCLASS()
class TAT_API UTATProjectileToolComponent : public UTATToolComponent
{
   GENERATED_BODY()
public:
   // From UTATToolComponent
   virtual bool AuthorityGetParametersForWorldActor(FGameplayTag usageTag, FTATGearWorldActorParameters& worldActorParams) const override;

   /// Given the start and end points, compute the initial direction/velocity of the projectile
   UFUNCTION(BlueprintCallable)
   FVector GetSuggestedProjectileVelocity(FVector startPos, FVector endPos);

   /// Function for users to add customizable data to the world actors through the respective tool blueprints
   UFUNCTION(BlueprintImplementableEvent)
   FTATToolWorldActorData GetToolWorldActorData() const;

   /// The actor to spawn in, based on the usage type, that performs the effect, once the projectile lands
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Tool", Meta = (Categories="Tool.Usage"))
   TMap<FGameplayTag, TSoftClassPtr<AActor>> WorldActorClasses;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   TSoftClassPtr<ATATProjectile> ProjectileToSpawn;

   /// What animation to play when we throw the projectile
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   UAnimMontage* ThrowMontage = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   bool bCanThrowAnimationBeInterrupted = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   float MaxInitialSpeed = 3000.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   float ArcParam = 0.9f;

   // if true the projectile will use gravity 0
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   bool bUseGravityZero = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
   FVector ThrowStartOffset = FVector::ZeroVector;

protected:
   virtual bool OnEquip_Implementation(const TScriptInterface<IToolInterface>& prevTool) override;
};


