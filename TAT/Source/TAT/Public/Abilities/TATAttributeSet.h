// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Attributes/AttributeBaseSet.h"
#include "TATAttributeSet.generated.h"

UCLASS()
class TAT_API UTATAttributeSet : public UAttributeSet
{
   GENERATED_BODY()

public:
   UTATAttributeSet();

   // from UAttributeSet
   virtual void PreAttributeChange(const FGameplayAttribute& attribute, float& newValue) override;
   virtual void PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const override;
   virtual void OnAttributeAggregatorCreated(const FGameplayAttribute& attribute, FAggregator* newAggregator) const override;
   virtual void PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue) override;

   /// Replication
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   /// SneakAttackDamageMultiplier is a multiplier used by the effect execution to boost outgoing attack damage when we're sneaking up on a target
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_SneakAttackDamageMultiplier)
   FGameplayAttributeData SneakAttackDamageMultiplier;
   virtual float GetSneakAttackDamageMultiplier() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATAttributeSet, SneakAttackDamageMultiplier)

   /// FlankAttackDamageMultiplier is a multiplier used by the effect execution to boost outgoing attack damage when we're flanking a target
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_FlankAttackDamageMultiplier)
   FGameplayAttributeData FlankAttackDamageMultiplier;
   virtual float GetFlankAttackDamageMultiplier() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATAttributeSet, FlankAttackDamageMultiplier)

   /// CounterAttackDamageMultiplier is a multiplier used by the effect execution to boost outgoing attack damage when we're attacking a parried target
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_CounterAttackDamageMultiplier)
   FGameplayAttributeData CounterAttackDamageMultiplier;
   virtual float GetCounterAttackDamageMultiplier() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATAttributeSet, CounterAttackDamageMultiplier)

   /// LightAttackSpeed is a multiplier used by the effect execution to increase/decrease light attack animation speeds
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_FlankAttackDamageMultiplier)
   FGameplayAttributeData LightAttackSpeed;
   virtual float GetLightAttackSpeed() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATAttributeSet, LightAttackSpeed)

   /// HeavyAttackSpeed is a multiplier used by the effect execution to increase/decrease light attack animation speeds
   UPROPERTY(BlueprintReadOnly, Category = "Damage", ReplicatedUsing = OnRep_FlankAttackDamageMultiplier)
   FGameplayAttributeData HeavyAttackSpeed;
   virtual float GetHeavyAttackSpeed() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATAttributeSet, HeavyAttackSpeed)

   /// NumRespawnsRemaining is the number of times a player can still respawn in the current match
   UPROPERTY(BlueprintReadOnly, Category = "Combat", ReplicatedUsing = OnRep_NumRespawnsRemaining)
   FGameplayAttributeData NumRespawnsRemaining;
   virtual float GetNumRespawnsRemaining() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATAttributeSet, NumRespawnsRemaining);

protected:
   // Replication
   UFUNCTION()
   virtual void OnRep_SneakAttackDamageMultiplier(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_FlankAttackDamageMultiplier(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_CounterAttackDamageMultiplier(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_LightAttackSpeed(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_HeavyAttackSpeed(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_NumRespawnsRemaining(const FGameplayAttributeData& oldValue);

private:
   void _ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const;
};
