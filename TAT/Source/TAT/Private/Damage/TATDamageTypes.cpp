// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Damage/TATDamageTypes.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDamageTypes)

FTATDamageWithType::FTATDamageWithType(const FTATScalableDamageWithType& scalableDamage)
   : DamageAmount(scalableDamage.DamageAmount.GetValue())
   , DamageType(scalableDamage.DamageType)
{
}
