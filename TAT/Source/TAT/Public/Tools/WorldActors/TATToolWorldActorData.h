// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATToolWorldActorData.generated.h"

// Holds customizable data for Tool World Actors
USTRUCT(BlueprintType)
struct TAT_API FTATToolWorldActorData
{
   GENERATED_BODY()

   UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
   int32 OptionalIntValue = 0;
};
