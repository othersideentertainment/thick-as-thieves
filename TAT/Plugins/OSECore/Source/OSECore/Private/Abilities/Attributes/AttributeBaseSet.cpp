// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Attributes/AttributeBaseSet.h"

#include "GameplayEffect.h"
#include "GameplayEffectAggregatorLibrary.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AttributeBaseSet)

//---------------------------------------------------------------------------------------
// Property access
//---------------------------------------------------------------------------------------

// Health
OSE_ATTRIBUTE_PROPERTY_PERCENTAGE(UAttributeBaseSet, Health, HealthMax)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, Health)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, HealthMax)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, HealthRegenRate)

// Energy
OSE_ATTRIBUTE_PROPERTY_PERCENTAGE(UAttributeBaseSet, Energy, EnergyMax)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, Energy)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, EnergyMax)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, EnergyRegenRate)

// Damage
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, HealthDamage)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, AttackDamage)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, AttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, DamageReductionMultiplier)

// Movement
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, MovementMaxSpeedMultiplier)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, MovementFrictionMultiplier)
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, MovementBrakingDecelerationMultiplier)

// Gravity
OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(UAttributeBaseSet, GravityScale)

//---------------------------------------------------------------------------------------
// Replication
//---------------------------------------------------------------------------------------

// Health
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, Health)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, HealthMax)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, HealthRegenRate)

// Energy
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, Energy)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, EnergyMax)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, EnergyRegenRate)

// Combat
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, AttackDamageMultiplier)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, DamageReductionMultiplier)

// Movement
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, MovementMaxSpeedMultiplier)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, MovementFrictionMultiplier)
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, MovementBrakingDecelerationMultiplier)

// Gravity
OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(UAttributeBaseSet, GravityScale)

void UAttributeBaseSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   #if UE_WITH_IRIS
   FDoRepLifetimeParams Params;
   Params.bIsPushBased = true;
   Params.RepNotifyCondition = REPNOTIFY_Always;
   // Health
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, Health, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, HealthMax, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, HealthRegenRate, Params);

   // Energy
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, Energy, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, EnergyMax, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, EnergyRegenRate, Params);

   // Combat
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, AttackDamageMultiplier, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, DamageReductionMultiplier, Params);

   // Movement
   // Could we get away with COND_OwnerOnly for this?
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, MovementMaxSpeedMultiplier, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, MovementFrictionMultiplier, Params);
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, MovementBrakingDecelerationMultiplier, Params);

   // Gravity
   DOREPLIFETIME_WITH_PARAMS_FAST(UAttributeBaseSet, GravityScale, Params);
   #else
   // Health
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, Health, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, HealthMax, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, HealthRegenRate, COND_None, REPNOTIFY_Always);

   // Energy
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, Energy, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, EnergyMax, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, EnergyRegenRate, COND_None, REPNOTIFY_Always);

   // Combat
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, AttackDamageMultiplier, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, DamageReductionMultiplier, COND_None, REPNOTIFY_Always);

   // Movement
   // Could we get away with COND_OwnerOnly for this?
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, MovementMaxSpeedMultiplier, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, MovementFrictionMultiplier, COND_None, REPNOTIFY_Always);
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, MovementBrakingDecelerationMultiplier, COND_None, REPNOTIFY_Always);

   // Gravity
   DOREPLIFETIME_CONDITION_NOTIFY(UAttributeBaseSet, GravityScale, COND_None, REPNOTIFY_Always);
   #endif
}

//---------------------------------------------------------------------------------------
// Attribute modification / evaluation
//---------------------------------------------------------------------------------------

// Initialization
UAttributeBaseSet::UAttributeBaseSet() : Super()
   , Health(1.0f)
   , HealthMax(1.0f)
   , HealthRegenRate(0.05f)
   , Energy(0.5f)
   , EnergyMax(1.0f)
   , EnergyRegenRate(0.065f)
   , HealthDamage(0.0f)
   , AttackDamage(0.0f)
   , AttackDamageMultiplier(1.0f)
   , DamageReductionMultiplier(1.0f)
   , MovementMaxSpeedMultiplier(1.0f)
   , MovementFrictionMultiplier(1.0f)
   , MovementBrakingDecelerationMultiplier(1.0f)
   , GravityScale(1.0f)
{
   
}

// Called just before any modification happens to an attribute
void UAttributeBaseSet::PreAttributeChange(const FGameplayAttribute& attribute, float& newValue)
{
   Super::PreAttributeChange(attribute, newValue);

   _ClampAttributes(attribute, newValue);
}

void UAttributeBaseSet::PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const
{
   Super::PreAttributeBaseChange(attribute, newValue);

   _ClampAttributes(attribute, newValue);
}

// Called just after a GameplayEffect is executed to modify the value of an attribute
void UAttributeBaseSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& data)
{
   // Handle health damage by reducing health
   if (data.EvaluatedData.Attribute == GetHealthDamageAttribute())
   {
      _UpdateHealthDamage();
   }
}

void UAttributeBaseSet::OnAttributeAggregatorCreated(const FGameplayAttribute& attribute, FAggregator* newAggregator) const
{
   Super::OnAttributeAggregatorCreated(attribute, newAggregator);

   if (!newAggregator)
   {
      return;
   }

   if (attribute == GetMovementMaxSpeedMultiplierAttribute()
      || attribute == GetMovementFrictionMultiplierAttribute()
      || attribute == GetMovementBrakingDecelerationMultiplierAttribute())
   {
      newAggregator->EvaluationMetaData = &FAggregatorEvaluateMetaDataLibrary::MostNegativeMod_AllPositiveMods;
   }
}

#if UE_WITH_IRIS
void UAttributeBaseSet::PostAttributeChange(const FGameplayAttribute& attribute, float oldValue, float newValue)
{
   Super::PostAttributeChange(attribute, oldValue, newValue);
   
   if (attribute == GetHealthAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, Health, this);
   }
   else if (attribute == GetHealthMaxAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, HealthMax, this);
   }
   else if(attribute == GetHealthRegenRateAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, HealthRegenRate, this);
   }
   else if (attribute == GetEnergyAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, Energy, this);
   }
   else if (attribute == GetEnergyMaxAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, EnergyMax, this);
   }
   else if (attribute == GetEnergyRegenRateAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, EnergyRegenRate, this);
   }
   else if (attribute == GetAttackDamageMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, AttackDamageMultiplier, this);
   }
   else if (attribute == GetDamageReductionMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, DamageReductionMultiplier, this);
   }
   else if (attribute == GetMovementMaxSpeedMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, MovementMaxSpeedMultiplier, this);
   }
   else if (attribute == GetMovementFrictionMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, MovementFrictionMultiplier, this);
   }
   else if (attribute == GetMovementBrakingDecelerationMultiplierAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, MovementBrakingDecelerationMultiplier, this);
   }
   else if (attribute == GetGravityScaleAttribute())
   {
      MARK_PROPERTY_DIRTY_FROM_NAME(UAttributeBaseSet, GravityScale, this);
   }
}
#endif

void UAttributeBaseSet::_UpdateHealthDamage()
{
   // This is a workaround for these issues:
   // https://udn.unrealengine.com/s/question/0D52L00005HAMFiSAP/currentmodcallbackdata-was-not-consumed-for-attribute
   // https://udn.unrealengine.com/s/question/0D52L00004lugEtSAI/gameplay-attributes-not-always-up-to-date-when-postgameplayeffectexecute-is-called
   // https://issues.unrealengine.com/issue/UE-109007

   // TL; DR: Epic has an optimization in place to batch attribute changes instead of applying them as gameplay effects are resolved,
   // and that means that UAttributeBaseSet::PostGameplayEffectExecute can be called for the attribute HealthDamage but the value is zero,
   // and then it's updated to a real value later in the frame.
   // This tick function catches HealthDamage applications that happen AFTER PostGameplayEffectExecute is called.
   // Hopefully we can just remove this code/tick once the bug is resolved and we've merged it

   // Store a local copy of the amount of damage done and clear the damage attribute
   const float localDamageDone = HealthDamage.GetBaseValue();
      
   if (localDamageDone > 0.0f)
   {
      // zero it out, we're consuming it now
      SetHealthDamage(0.f);

      // Apply the health change
      const float newHealth = GetHealth() - localDamageDone;
      SetHealth(FMath::Clamp(newHealth, 0.0f, GetHealthMax()));
   }
}

void UAttributeBaseSet::_ClampAttributes(const FGameplayAttribute& attribute, float& newValue) const
{
   if (attribute == GetHealthAttribute())
   {
      newValue = FMath::Clamp(newValue, 0.0f, GetHealthMax());
   }
   else if (attribute == GetEnergyAttribute())
   {
      newValue = FMath::Clamp(newValue, 0.0f, GetEnergyMax());
   }
}

// IAttributeBaseSystemInterface
TScriptInterface<IAttributeBaseInterface> UAttributeBaseSet::GetBaseAttributeInterface() const
{
   // Cast is needed only to satisfy blueprint code gen when returning an interface from a blueprint function
   return const_cast<UAttributeBaseSet*>(this);
}
