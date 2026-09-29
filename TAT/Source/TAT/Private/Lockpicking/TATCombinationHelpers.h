// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/TATMapVariationSeedHelpers.h"

namespace CombinationHelpers
{
   inline FText FormatCombination(int16 combination) 
   {
      FNumberFormattingOptions options;
      options.UseGrouping = false;
      options.MinimumIntegralDigits = 4;
      return FText::AsNumber(combination, &options);
   }

   inline int32 MakeSeedForName(FName combinationName, int32 mapSeed)
   {
      return SeedHelpers::MakeSeedForName(combinationName, mapSeed);
   }

   inline int16 GenerateCombination(int32 seed)
   {
      FRandomStream stream(seed);
      return stream.RandRange(0, 9999);
   }

   inline FText FormatCombinationFromName(FName combinationName, int32 mapSeed) 
   {
      return FormatCombination(GenerateCombination(MakeSeedForName(combinationName, mapSeed)));
   }
}
