// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//OSE includes
#include "AttributeBaseInterface.h"
#include "AttributeBaseSystemInterface.h"
#include "OSEAttributeMacros.h"

//UE includes
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Tickable.h"

#include "AttributeBaseSet.generated.h"



//---------------------------------------------------------------------------------------
/// UAttributeBaseSet
//---------------------------------------------------------------------------------------

/// The base attribute set contains standard attributes expected to be useful to all characters
/// that use the ability system
UCLASS(ClassGroup = (Ability))
class OSECORE_API UAttributeBaseSet
   : public UAttributeSet
   , public IAttributeBaseInterface
   , public IAttributeBaseSystemInterface
{
   GENERATED_BODY()

public:

   // Initialization
   UAttributeBaseSet();

   /// AttributeSet overrides
   virtual void PreAttributeChange(const FGameplayAttribute& attribute, float& newValue) override;
   virtual void PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const override;
   virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& data) override;
   virtual void OnAttributeAggregatorCreated(const FGameplayAttribute& attribute, FAggregator* newAggregator) const override;

   /// Replication
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   #if UE_WITH_IRIS
   void PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue) override;
   #endif

public:

   // IAttributeBaseSystemInterface
   virtual TScriptInterface<IAttributeBaseInterface> GetBaseAttributeInterface() const override;

   // IAttributeBaseInterface (Health)
   virtual float GetHealth() const override;
   virtual float GetHealthMax() const override;
   virtual float GetHealthPercent() const override;
   virtual float GetHealthRegenRate() const override;

   // IAttributeBaseInterface (Energy)
   virtual float GetEnergy() const override;
   virtual float GetEnergyMax() const override;
   virtual float GetEnergyPercent() const override;
   virtual float GetEnergyRegenRate() const override;

   // IAttributeBaseInterface (Damage)
   virtual float GetHealthDamage() const override;
   virtual float GetAttackDamage() const override;
   virtual float GetAttackDamageMultiplier() const override;
   virtual float GetDamageReductionMultiplier() const override;


   virtual float GetMovementMaxSpeedMultiplier() const override;
   virtual float GetMovementFrictionMultiplier() const override;
   virtual float GetMovementBrakingDecelerationMultiplier() const override;
   virtual float GetGravityScale() const override;

public:

   //---------------------------------------------------------------------------------------
   // Health
   //---------------------------------------------------------------------------------------

   /// Current Health; capped by HealthMax.
   /// Positive changes can use this directly.
   /// Negative changes to Health should go through HealthDamage meta attribute.
   UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
   FGameplayAttributeData Health;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, Health)
   
   /// HealthMax is its own attribute since GameplayEffects may modify it
   UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_HealthMax)
   FGameplayAttributeData HealthMax;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, HealthMax)

   /// Health regen rate will passively increase Health every second
   UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_HealthRegenRate)
   FGameplayAttributeData HealthRegenRate;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, HealthRegenRate)

protected:

   // Replication
   UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldValue);
   UFUNCTION() virtual void OnRep_HealthMax(const FGameplayAttributeData& OldValue);
   UFUNCTION() virtual void OnRep_HealthRegenRate(const FGameplayAttributeData& OldValue);

public:

   //---------------------------------------------------------------------------------------
   // Energy
   //---------------------------------------------------------------------------------------

   /// Current Energy, capped by EnergyMax
   UPROPERTY(BlueprintReadOnly, Category = "Energy", ReplicatedUsing = OnRep_Energy)
   FGameplayAttributeData Energy;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, Energy)
   
   /// EnergyMax is its own attribute since GameplayEffects may modify it
   UPROPERTY(BlueprintReadOnly, Category = "Energy", ReplicatedUsing = OnRep_EnergyMax)
   FGameplayAttributeData EnergyMax;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, EnergyMax)

   /// Energy regen rate will passively increase Energy every second
   UPROPERTY(BlueprintReadOnly, Category = "Energy", ReplicatedUsing = OnRep_EnergyRegenRate)
   FGameplayAttributeData EnergyRegenRate;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, EnergyRegenRate)

   // Replication
   UFUNCTION() virtual void OnRep_Energy(const FGameplayAttributeData& oldValue);
   UFUNCTION() virtual void OnRep_EnergyMax(const FGameplayAttributeData& oldValue);
   UFUNCTION() virtual void OnRep_EnergyRegenRate(const FGameplayAttributeData& oldValue);


public:

   //---------------------------------------------------------------------------------------
   // Damage
   //---------------------------------------------------------------------------------------

   /// HealthDamage is a meta attribute used by the effect execution to calculate final damage,
   /// which then turns into -Health. Temporary value that only exists on the Server. Not replicated.
   UPROPERTY(BlueprintReadOnly, Category = "Damage")
   FGameplayAttributeData HealthDamage;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, HealthDamage)

   /// AttackDamage is a meta attribute used by the effect execution to calculate attacker damage,
   /// which is then sent to a defender to respond with and mitigate with their own defense attributes.
   UPROPERTY(BlueprintReadOnly, Category = "Damage")
   FGameplayAttributeData AttackDamage;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, AttackDamage)

   /// AttackDamageMultiplier is a multiplier used by the effect execution to boost outgoing attack damage
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_AttackDamageMultiplier)
   FGameplayAttributeData AttackDamageMultiplier;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, AttackDamageMultiplier)

   /// DamageReductionMultiplier is a multiplier used by the effect execution to reduce incoming damage before it's applied as HealthDamage
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_DamageReductionMultiplier)
   FGameplayAttributeData DamageReductionMultiplier;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, DamageReductionMultiplier)

   // Replication
   UFUNCTION() virtual void OnRep_AttackDamageMultiplier(const FGameplayAttributeData& oldValue);
   UFUNCTION() virtual void OnRep_DamageReductionMultiplier(const FGameplayAttributeData& oldValue);


   //---------------------------------------------------------------------------------------
   // Movement Speed
   //---------------------------------------------------------------------------------------

   /// MovementMaxSpeedMultiplier is an attribute for *temporary* changes to a characters max movement speed.
   /// Does not affect root motion.
   UPROPERTY(BlueprintReadOnly, Category = "Movement Speed", ReplicatedUsing = OnRep_MovementMaxSpeedMultiplier)
   FGameplayAttributeData MovementMaxSpeedMultiplier;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, MovementMaxSpeedMultiplier)

   ///
   UPROPERTY(BlueprintReadOnly, Category = "Movement Speed", ReplicatedUsing = OnRep_MovementFrictionMultiplier)
   FGameplayAttributeData MovementFrictionMultiplier;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, MovementFrictionMultiplier)

   ///
   UPROPERTY(BlueprintReadOnly, Category = "Movement Speed", ReplicatedUsing = OnRep_MovementBrakingDecelerationMultiplier)
   FGameplayAttributeData MovementBrakingDecelerationMultiplier;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, MovementBrakingDecelerationMultiplier)

   // Replication
   UFUNCTION() virtual void OnRep_MovementMaxSpeedMultiplier(const FGameplayAttributeData& oldValue);
   UFUNCTION() virtual void OnRep_MovementFrictionMultiplier(const FGameplayAttributeData& oldValue);
   UFUNCTION() virtual void OnRep_MovementBrakingDecelerationMultiplier(const FGameplayAttributeData& oldValue);

   //---------------------------------------------------------------------------------------
  // Gravity Scale
  //---------------------------------------------------------------------------------------

   UPROPERTY(BlueprintReadOnly, Category = "Gravity", ReplicatedUsing = OnRep_GravityScale)
   FGameplayAttributeData GravityScale;
   OSE_ATTRIBUTE_MODIFIERS(UAttributeBaseSet, GravityScale)

   // Replication
   UFUNCTION() virtual void OnRep_GravityScale(const FGameplayAttributeData& oldValue);

protected:

private:
   void _UpdateHealthDamage();
   void _ClampAttributes(const FGameplayAttribute& attribute, float& newValue) const;
};

