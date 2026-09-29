// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/Attributes/OSEAttributeMacros.h"

// ue
#include "AttributeSet.h"

#include "TATStaminaAttributeSet.generated.h"

UCLASS()
class TAT_API UTATStaminaAttributeSet : public UAttributeSet
{
   GENERATED_BODY()

public:
   UTATStaminaAttributeSet();

   /// Replication
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // from UAttributeSet
   virtual void PreAttributeChange(const FGameplayAttribute& attribute, float& newValue) override;
   virtual void PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const override;
   virtual void PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue) override;

protected:
   UFUNCTION()
   virtual void OnRep_Stamina(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_StaminaMax(const FGameplayAttributeData& oldValue);
   UFUNCTION()
   virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& oldValue);

private:
   void _ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const;

public:
   /// Stamina is an attribute consumed by (and limiting the usage of) traversal abilities (eg. sprinting / climbing) as well as some tool usage (eg. blackjack takedown).
   UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_Stamina)
   FGameplayAttributeData Stamina;
   virtual float GetStamina() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATStaminaAttributeSet, Stamina)

   /// StaminaRegenRate is an attribute driving the rate of stamina recovery over time.
   UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_StaminaRegenRate)
   FGameplayAttributeData StaminaRegenRate;
   virtual float GetStaminaRegenRate() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATStaminaAttributeSet, StaminaRegenRate)

   /// StaminaMax is an meta-attribute, as it can be modified by effects.
   UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_StaminaMax)
   FGameplayAttributeData StaminaMax;
   virtual float GetStaminaMax() const;
   OSE_ATTRIBUTE_MODIFIERS(UTATStaminaAttributeSet, StaminaMax)
};
