// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// Include core engine collision profiles
#include "OSECoreCollision.h"

// Add game specific collision profiles here
// deleted-but-should-reuse COLLISION_PAWN_SPIRIT        ECC_GameTraceChannel9
#define COLLISION_PROJECTION_PAWN    ECC_GameTraceChannel10
// deleted-but-should-reuse COLLISION_WANDERER           ECC_GameTraceChannel11
#define COLLISION_TOOL_WORLD_ACTOR   ECC_GameTraceChannel12
