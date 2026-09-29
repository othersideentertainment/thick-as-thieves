// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATRankingFunctionLibrary.generated.h"

struct FMatchPersistentData;

UCLASS()
class TAT_API UTATRankingFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   static int32 GenerateMatchRankingForPlayer(const FMatchPersistentData& matchPersistentData);
};

