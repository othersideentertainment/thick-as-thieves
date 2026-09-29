// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CombatTypes.generated.h"

/// When checking a trace against a defender (e.g. LoS), what position should we use
UENUM(BlueprintType)
enum class EOSECombatDefenderTraceLogic : uint8
{
   /// Trace against the center of the defender's bounds
   TraceToCenter,

   /// Trace against the defender's eye position
   TraceToEyes
};

