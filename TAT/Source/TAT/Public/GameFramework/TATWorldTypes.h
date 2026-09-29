// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "TATWorldTypes.generated.h"

// NOTE: TATProjectSettings saves settings using the integer value of this enum, so be careful about changes that would renumber it
UENUM(BlueprintType)
enum class ETATMapType : uint8
{
   Menu,
   Hub,
   Transition,
   Mission,
   ThievesDen,
   Tutorial,
   Developer    UMETA(Tooltip = "Internal developer map"),
   ServerStart,
   MAX          UMETA(Hidden)
};
