// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

/** When you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list **/
#define COLLISION_PROJECTILE                      ECC_GameTraceChannel1
#define COLLISION_INTERACT                        ECC_GameTraceChannel2
#define COLLISION_OVERLAP_PLAYER_PAWN             ECC_GameTraceChannel3
#define COLLISION_AOE                             ECC_GameTraceChannel4
