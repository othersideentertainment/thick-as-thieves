// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ue
#include "CoreMinimal.h"

#include "TATEndgameReason.generated.h"

UENUM(BlueprintType)
enum class ETATEndgameReason : uint8
{
   Mission,
   Timer,
   Loot,
};
