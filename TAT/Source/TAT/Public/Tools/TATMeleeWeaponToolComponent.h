// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Damage/TATDamageTypes.h"
#include "Tools/TATToolComponent.h"

#include "TATMeleeWeaponToolComponent.generated.h"

class UAnimMontage;
class UGameplayAbility;
class UOSESyncedAnimationDataAsset;


USTRUCT(BlueprintType)
struct TAT_API FTATMeleeWeaponAttackAnimationPool
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TArray<UAnimMontage*> Animations;
};

/// Represents a melee attack for a particular input for a particular tool
/// Melee weapons may have multiple attacks with different animations, damage levels, etc.
USTRUCT(BlueprintType)
struct TAT_API FTATMeleeWeaponAttack
{
   GENERATED_BODY()
public:

   FTATMeleeWeaponAttack()
   {
      Damage.DamageAmount.SetValue(15.0f);
   }

   /// A set of animations: we sequentially go through the pools in order for each attack,
   /// picking a random animation from the pool for the attack
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TArray<FTATMeleeWeaponAttackAnimationPool> SequentialAttackPools;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float AttackSpeed = 1.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(FullyExpand="true"))
   FTATScalableDamageWithType Damage;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Combat.Attack.DealDamage"))
   FGameplayTag DealDamageEventTag;

   /// If true, usage will loop procedurally through SequentialAttackPools until reaching the last one (or an attack is not performed after ChainAttackMaxIntervalBetweenAttacks since the last attack ended)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool IsChainAttack = false;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "IsChainAttack"))
   float ChainAttackMaxSecondsBetweenHits = 0.7f;

   /// Set to true if we want a non-default miss ability
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool OverrideMissAbilityClass = false;

   /// What ability, if any, to activate if we miss entirely with this attack
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition="OverrideMissAbilityClass", EditConditionHides))
   TSoftClassPtr<UGameplayAbility> MissAbilityClass;

   /// Set to true if we want a non-default value for CancelAbilitiesWithTag
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool OverrideCancelAbilitiesWithTag = false;

   /// Which abilities to cancel when we activate this attack
   /// NB: Categories metadata is explicitly reset to empty since it will be used in MeleeAttacksPerUsage, which by default
   /// would restrict to tool usage
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "OverrideCancelAbilitiesWithTag", EditConditionHides, Categories = ""))
   FGameplayTagContainer CancelAbilitiesWithTag;

   /// Cue that plays when we hit a character
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "GameplayCue"))
   FGameplayTag HitCharacterCue;
};

/// A tool that represents a melee weapon
/// By default these have infinite ammo, and are largely about playing an animation
/// and hooking into the CombatComponent
UCLASS()
class TAT_API UTATMeleeWeaponToolComponent : public UTATToolComponent
{
   GENERATED_BODY()
public:
   UTATMeleeWeaponToolComponent();

   // From UObject
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   /// The definition of attacks for each of the possible inputs
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Tool", Meta = (Categories = "Tool.Usage"))
   TMap<FGameplayTag, FTATMeleeWeaponAttack> MeleeAttacksPerUsage;

   /// If we can use this weapon to perform a stealth takedown
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool CanBeUsedForStealthTakedowns = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "CanBeUsedForStealthTakedowns"))
   TArray<UOSESyncedAnimationDataAsset*> TakedownAnimations;

   /// If we can use this weapon to perform a parry
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool CanBeUsedForParry = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "CanBeUsedForParry"))
   TObjectPtr<UAnimMontage> ParryAnimation;

   // TODO: this struct now has arrays of arrays (and thus will alloc on copy) should mitigate?
   UFUNCTION(BlueprintPure)
   bool GetMeleeWeaponAttackForUsage(UPARAM(Meta = (Categories = "Tool.Usage")) FGameplayTag usageTag, FTATMeleeWeaponAttack& weaponAttack) const;
   const FTATMeleeWeaponAttack* GetMeleeWeaponAttackForUsage(FGameplayTag usageTag) const;

   int32 GetMaxChainAttackLength() const;

private:
   const FTATMeleeWeaponAttack* _GetChainMeleeWeaponAttack() const;
};
