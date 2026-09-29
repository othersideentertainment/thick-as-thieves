// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "AlertnessEnums.generated.h"

//////////////////////////////////////////////////////////////////////////
///        EAlertnessLevel - per AI character
//////////////////////////////////////////////////////////////////////////
UENUM(BlueprintType)
enum class EAlertnessLevel : uint8
{
   Neutral     = 0,
   Suspicious  = 1,
   Alerted     = 2,
   Combat      = 3,
   MAX         = 4 UMETA(Hidden)
};
