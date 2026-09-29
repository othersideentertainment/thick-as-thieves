// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Attributes/TATPropHealthAttributeSet.h"

// ue5
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPropHealthAttributeSet)

UTATPropHealthAttributeSet::UTATPropHealthAttributeSet()
   : PropHealth(-1) //< Starting at -1 for now since the replicator starts from the CDO, this guarantees that it won't think a value is already synced (at the expense of potentially sending multiple times)
   , PropDamageReductionMultiplier(1)
{
}

void UTATPropHealthAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.RepNotifyCondition = REPNOTIFY_Always;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PropHealth, params);
}

void UTATPropHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& attribute, float& newValue)
{
   Super::PreAttributeChange(attribute, newValue);

   _ClampAttribute(attribute, newValue);
}

void UTATPropHealthAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const
{
   Super::PreAttributeBaseChange(attribute, newValue);

   _ClampAttribute(attribute, newValue);
}

void UTATPropHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue)
{
   Super::PostAttributeChange(attribute, oldValue, newValue);

   if (attribute == GetPropHealthAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PropHealth, this);
   }
}

void UTATPropHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& data)
{
   Super::PostGameplayEffectExecute(data);

   if (data.EvaluatedData.Attribute == GetPropDamageAttribute())
   {
      SetPropHealth(FMath::Clamp(GetPropHealth() - GetPropDamage(), 0, GetPropHealthMax()));
      SetPropDamage(0);
   }

   if (GetPropHealth() <= 0 && !_outOfHealth)
   {
      const FGameplayEffectContextHandle& effectContext = data.EffectSpec.GetEffectContext();
      AActor* instigator = effectContext.GetOriginalInstigator();
      OnOutOfHealth.Broadcast(instigator, &data.EffectSpec);
   }

   _outOfHealth = GetPropHealth() <= 0;
}

void UTATPropHealthAttributeSet::OnRep_PropHealth(const FGameplayAttributeData& oldValue)
{
   GAMEPLAYATTRIBUTE_REPNOTIFY(UTATPropHealthAttributeSet, PropHealth, oldValue);

   // Call the change callback, but without an instigator
   // This could be changed to an explicit RPC in the future
   // These events on the client should not be changing attributes

   const float currentHealth = GetPropHealth();
   const float estimatedMagnitude = currentHealth - oldValue.GetCurrentValue();

   if (!_outOfHealth && currentHealth <= 0.0f)
   {
      OnOutOfHealth.Broadcast(nullptr, nullptr);
   }

   _outOfHealth = (currentHealth <= 0.0f);
}

void UTATPropHealthAttributeSet::_ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const
{
   if (attribute == GetPropHealthAttribute())
   {
      newValue = FMath::Clamp(newValue, 0.0f, GetPropHealthMax());
   }
}
