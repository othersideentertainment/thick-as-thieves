// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

enum class ETATDifficulty : uint8;
struct FGameplayTag;

namespace TATSessionParameters
{
   FName GetMapKey();
   FName GetDifficultyKey();

   TOptional<int32> EncodeMap(const FGameplayTag& mapTag);
   FGameplayTag DecodeMap(int32 encodedMap);

   inline int32 EncodeDifficulty(ETATDifficulty difficulty) { return static_cast<int32>(difficulty); }
   inline ETATDifficulty DecodeDifficulty(int32 encoded) { return static_cast<ETATDifficulty>(encoded); }
}
