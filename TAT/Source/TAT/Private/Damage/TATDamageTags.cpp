// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Damage/TATDamageTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_DamageType, "DamageType");
UE_DEFINE_GAMEPLAY_TAG(TAG_DamageType_Physical, "DamageType.Physical");
UE_DEFINE_GAMEPLAY_TAG(TAG_DamageType_Magic, "DamageType.Magic");
UE_DEFINE_GAMEPLAY_TAG(TAG_DamageType_Shock, "DamageType.Shock");
UE_DEFINE_GAMEPLAY_TAG(TAG_DamageType_Burn, "DamageType.Burn");
UE_DEFINE_GAMEPLAY_TAG(TAG_DamageType_Poison, "DamageType.Poison");

UE_DEFINE_GAMEPLAY_TAG(TAG_SetByCaller_Damage, "SetByCaller.Damage");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_DamageContext, "DamageContext", "Tags associated with the way that damage was applied");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_DamageContext_Blocked, "DamageContext.Blocked", "To be applied to blocked damage");
