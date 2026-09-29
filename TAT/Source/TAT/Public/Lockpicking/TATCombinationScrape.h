// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

namespace CombinationScrape
{
#if WITH_EDITOR
   TAT_API TSet<FName> GetLockCombinationNamesInWorld(const UWorld* world);
   TAT_API void CheckForDuplicateCombinations(const UWorld* world, FMessageLog& msgLog);
   TAT_API TArray<UActorComponent*, TInlineAllocator<4>> FindLocksWithCombination(const UWorld* world, FName combination);
#endif
}
