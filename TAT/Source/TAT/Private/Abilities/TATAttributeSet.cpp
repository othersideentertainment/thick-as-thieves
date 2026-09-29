// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATAttributeSet.h"

// ue4
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAttributeSet)

// properties
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATAttributeSet, SneakAttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATAttributeSet, FlankAttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATAttributeSet, CounterAttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATAttributeSet, LightAttackSpeed)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATAttributeSet, HeavyAttackSpeed)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATAttributeSet, NumRespawnsRemaining)

// replication
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATAttributeSet, SneakAttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATAttributeSet, FlankAttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATAttributeSet, CounterAttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATAttributeSet, LightAttackSpeed)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATAttributeSet, HeavyAttackSpeed)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATAttributeSet, NumRespawnsRemaining)

UTATAttributeSet::UTATAttributeSet()
   : Super()
   , SneakAttackDamageMultiplier(1.0f)
   , FlankAttackDamageMultiplier(1.0f)
   , CounterAttackDamageMultiplier(1.0f)
   , LightAttackSpeed(1.0f)
   , HeavyAttackSpeed(1.0f)
   , NumRespawnsRemaining(-1.0f)
{
}

void UTATAttributeSet::PreAttributeChange(const FGameplayAttribute& attribute, float& newValue)
{
   Super::PreAttributeChange(attribute, newValue);

   _ClampAttribute(attribute, newValue);
}

void UTATAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const
{
   Super::PreAttributeBaseChange(attribute, newValue);

   _ClampAttribute(attribute, newValue);
}

void UTATAttributeSet::OnAttributeAggregatorCreated(const FGameplayAttribute& attribute, FAggregator* newAggregator) const
{
   Super::OnAttributeAggregatorCreated(attribute, newAggregator);
}

void UTATAttributeSet::PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue)
{
   Super::PostAttributeChange(attribute, oldValue, newValue);

   if (attribute == GetSneakAttackDamageMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SneakAttackDamageMultiplier, this);
   }
   else if (attribute == GetFlankAttackDamageMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, FlankAttackDamageMultiplier, this);
   }
   else if (attribute == GetCounterAttackDamageMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, CounterAttackDamageMultiplier, this);
   }
   else if (attribute == GetLightAttackSpeedAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LightAttackSpeed, this);
   }
   else if (attribute == GetHeavyAttackSpeedAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, HeavyAttackSpeed, this);
   }
   else if (attribute == GetNumRespawnsRemainingAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, NumRespawnsRemaining, this);
   }
}

void UTATAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.RepNotifyCondition = REPNOTIFY_Always;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SneakAttackDamageMultiplier, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, FlankAttackDamageMultiplier, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CounterAttackDamageMultiplier, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LightAttackSpeed, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, HeavyAttackSpeed, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, NumRespawnsRemaining, params);
}

void UTATAttributeSet::_ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const
{
}
