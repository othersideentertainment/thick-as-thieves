// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "AudioEnums.generated.h"

UENUM(BlueprintType)
enum class EAkComponentType : uint8
{
   Footsteps,
   Weapon,
   Voice,
};
