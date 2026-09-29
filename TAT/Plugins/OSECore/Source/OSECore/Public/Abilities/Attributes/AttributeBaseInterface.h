// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UObject/Interface.h"

#include "AttributeBaseInterface.generated.h"


// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(MinimalAPI, Category = "Ability", meta = (CannotImplementInterfaceInBlueprint))
class UAttributeBaseInterface : public UInterface
{
   GENERATED_BODY()
};


//---------------------------------------------------------------------------------------------------
/// Base attribute interface. Base attributes are standard attributes expected to be useful to all
/// pawns that use the ability system.
//---------------------------------------------------------------------------------------------------

class OSECORE_API IAttributeBaseInterface
{
   GENERATED_BODY()

public:

   /// Current Health; capped by HealthMax
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetHealth() const = 0;

   /// Maximum Health value
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetHealthMax() const = 0;

   /// Current / Maximum Health
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetHealthPercent() const = 0;

   /// Health regen rate will passively increase Health every second
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetHealthRegenRate() const = 0;

public:

   /// Current Energy, capped by EnergyMax
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetEnergy() const = 0;

   /// Maximum Energy value
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetEnergyMax() const = 0;

   /// Current / Maximum Energy
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetEnergyPercent() const = 0;

   /// Energy regen rate will passively increase Energy every second
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetEnergyRegenRate() const = 0;

public:

   /// HealthDamage is a meta attribute which turns into -Health
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetHealthDamage() const = 0;

   /// AttackDamage is a meta attribute which is sent to a defender to deal HealthDamage after it's calculated
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetAttackDamage() const = 0;

   /// AttackDamageMultiplier boosts attack damage
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetAttackDamageMultiplier() const = 0;

   /// DamageReductionMultiplier reduces incoming damage
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetDamageReductionMultiplier() const = 0;


public:

   /// MovementMaxSpeedMultiplier is an attribute for *temporary* changes to a characters max movement speed.
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetMovementMaxSpeedMultiplier() const = 0;

   ///
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetMovementFrictionMultiplier() const = 0;

   ///
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetMovementBrakingDecelerationMultiplier() const = 0;

   /// GravityScale is an attribute for *temporary* changes the rate of gravity on the character.
   UFUNCTION(BlueprintCallable, Category = "Ability|Attribute")
   virtual float GetGravityScale() const = 0;
};
