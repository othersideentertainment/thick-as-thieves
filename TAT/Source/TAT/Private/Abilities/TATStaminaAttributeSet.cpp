// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATStaminaAttributeSet.h"

// ue
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStaminaAttributeSet)

// properties
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATStaminaAttributeSet, Stamina)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATStaminaAttributeSet, StaminaMax)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UTATStaminaAttributeSet, StaminaRegenRate)

// replication
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATStaminaAttributeSet, Stamina)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATStaminaAttributeSet, StaminaMax)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UTATStaminaAttributeSet, StaminaRegenRate)

UTATStaminaAttributeSet::UTATStaminaAttributeSet()
   : Super()
   , Stamina(1.0f)
   , StaminaRegenRate(1.0f)
   , StaminaMax(1.0f)
{
}

void UTATStaminaAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;
   params.RepNotifyCondition = REPNOTIFY_Always;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Stamina, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StaminaRegenRate, params);
   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StaminaMax, params);
}

void UTATStaminaAttributeSet::PreAttributeChange(const FGameplayAttribute& attribute, float& newValue)
{
   Super::PreAttributeChange(attribute, newValue);

   _ClampAttribute(attribute, newValue);
}

void UTATStaminaAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const
{
   Super::PreAttributeBaseChange(attribute, newValue);

   _ClampAttribute(attribute, newValue);
}

void UTATStaminaAttributeSet::PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue)
{
   Super::PostAttributeChange(attribute, oldValue, newValue);

   if (attribute == GetStaminaAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Stamina, this);
   }
   else if (attribute == GetStaminaRegenRateAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StaminaRegenRate, this);
   }
   else if (attribute == GetStaminaMaxAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StaminaMax, this);
   }
}

void UTATStaminaAttributeSet::_ClampAttribute(const FGameplayAttribute& attribute, float& newValue) const
{
   if (attribute == GetStaminaAttribute())
   {
      newValue = FMath::Clamp(newValue, 0.0f, GetStaminaMax());
   }
}
