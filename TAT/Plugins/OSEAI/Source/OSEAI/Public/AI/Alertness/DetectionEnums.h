// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "DetectionEnums.generated.h"

//////////////////////////////////////////////////////////////////////////
///        EActorDetectionState - per AI character per target actor
//////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class EActorDetectionState : uint8
{
   Observing,       // We are in a neutral/suspicious state and are doing our first detection ramp
   Identifying,     // We are in an alerted/combat state and are doing our second detection ramp for this target
   Identified       // We have fully identified this target
};
