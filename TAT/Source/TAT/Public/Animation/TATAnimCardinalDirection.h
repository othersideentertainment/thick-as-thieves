// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TATAnimCardinalDirection.generated.h"

// Cardinal direction originally adapted from Lyra ABPs
UENUM(BlueprintType)
enum class ETATAnimCardinalDirection : uint8
{
   Forward,
   Backward,
   Left,
   Right
};

UCLASS(meta = (BlueprintThreadSafe))
class TAT_API UTATAnimCardinalDirectionUtils : public UObject
{
   GENERATED_BODY()

   // native port of Lyra SelectCardinalDirectionFromAngle
   UFUNCTION(BlueprintPure, Category = "TAT|Anim")
   static ETATAnimCardinalDirection SelectCardinalDirectionFromAngle(float angle, float deadZone, ETATAnimCardinalDirection currentDirection, bool favorCurrentDirection);

   UFUNCTION(BlueprintPure, Category = "TAT|Anim")
   static ETATAnimCardinalDirection GetOppositeDirection(ETATAnimCardinalDirection direction);
};
