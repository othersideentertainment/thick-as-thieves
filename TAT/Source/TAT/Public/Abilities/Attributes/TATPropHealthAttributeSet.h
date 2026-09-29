// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/Attributes/OSEAttributeMacros.h"

// ue5
#include "CoreMinimal.h"
#include "AttributeSet.h"

#include "TATPropHealthAttributeSet.generated.h"

#define PROP_ATTRIBUTE_MODIFIERS(Class, Attribute) \
   OSE_ATTRIBUTE_MODIFIERS(Class, Attribute) \
   GAMEPLAYATTRIBUTE_VALUE_GETTER(Attribute)

// An attribute set to represent the Health of breakable actors in the environment
//
// TODO(5.2) Once attributes have redirects, evaluate whether to extract the character health
//           attributes into a separate attribute set, or keep these separate.
UCLASS()
class TAT_API UTATPropHealthAttributeSet : public UAttributeSet
{
   GENERATED_BODY()

public:
   UTATPropHealthAttributeSet();

   /// Replication
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // from UAttributeSet
   virtual void PreAttributeChange(const FGameplayAttribute& attribute, float& newValue) override;
   virtual void PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const override;
   virtual void PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue) override;
   virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& data) override;

   DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOutOfHealth, AActor* /*EffectInstigator*/, const FGameplayEffectSpec* /*EffectSpec*/);
   mutable FOnOutOfHealth OnOutOfHealth;

   /// Current Health; capped by HealthMax.
   /// Positive changes can use this directly.
   /// Negative changes to Health should go through HealthDamage meta attribute.
   UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_PropHealth)
   FGameplayAttributeData PropHealth;
   PROP_ATTRIBUTE_MODIFIERS(UTATPropHealthAttributeSet, PropHealth)

   /// HealthMax is not replicated
   UPROPERTY(BlueprintReadOnly, Category = "Health", Meta = (HideFromModifiers))
   FGameplayAttributeData PropHealthMax;
   PROP_ATTRIBUTE_MODIFIERS(UTATPropHealthAttributeSet, PropHealthMax)

   /// HealthDamage is a meta attribute used by the effect execution to calculate final damage,
   /// which then turns into -Health. Temporary value that only exists on the Server. Not replicated.
   UPROPERTY(BlueprintReadOnly, Category = "Damage")
   FGameplayAttributeData PropDamage;
   PROP_ATTRIBUTE_MODIFIERS(UTATPropHealthAttributeSet, PropDamage)

   /// DamageReductionMultiplier is a multiplier used by the effect execution to reduce incoming damage before it's applied as HealthDamage
   UPROPERTY(BlueprintReadOnly, Category = "Damage")
   FGameplayAttributeData PropDamageReductionMultiplier;
   PROP_ATTRIBUTE_MODIFIERS(UTATPropHealthAttributeSet, PropDamageReductionMultiplier)

protected:
   UFUNCTION()
   void OnRep_PropHealth(const FGameplayAttributeData& oldValue);

private:
   void _ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const;
   bool _outOfHealth = false;
};
