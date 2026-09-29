// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

//---------------------------------------------------------------------------------------
/// Helper macros to minimize copy/paste errors
//---------------------------------------------------------------------------------------

// Defines macro to access attribute values
#define OSE_ATTRIBUTE_MODIFIERS(ClassName, PropertyName) \
   GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
   GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
   GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// Macro for rote property value access
#define OSE_ATTRIBUTE_PROPERTY_VALUE_IMPL(ClassName, PropertyName) \
   float ClassName::Get##PropertyName() const { return PropertyName.GetCurrentValue(); }

// Macro for percentage property value access
#define OSE_ATTRIBUTE_PROPERTY_PERCENTAGE(ClassName, PropertyName, PropertyNameMax) \
   float ClassName::Get##PropertyName##Percent() const \
   { \
      float Denom = Get##PropertyNameMax(); \
      return (Denom > 0.0f) ? Get##PropertyName() / Denom : 0.0f; \
   }

// Macro for rote property value replication
#define OSE_ATTRIBUTE_PROPERTY_REPLICATION_IMPL(ClassName, PropertyName) \
   void ClassName::OnRep_##PropertyName(const FGameplayAttributeData& OldValue) { GAMEPLAYATTRIBUTE_REPNOTIFY(ClassName, PropertyName, OldValue); }

