// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue4
#include "CoreMinimal.h"

// self
//#include "TATAlertnessUtl.generated.h"

namespace AlertnessUtl
{
   bool CanCharacterDecayAlertnessToNeutral(AActor* actor, EAlertnessLevel currentAlertnessLevel);
}
