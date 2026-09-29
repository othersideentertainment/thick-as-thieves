// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATSessionParameters.h"

#include "Developer/TATProjectSettings.h"


namespace TATSessionParameters
{
   FName GetMapKey()
   {
      static const FName kMapNameKey = FName("Map");
      return kMapNameKey;
   }

   FName GetDifficultyKey()
   {
      static const FName kDifficultyKey = FName("Difficulty");
      return kDifficultyKey;
   }

   TOptional<int32> EncodeMap(const FGameplayTag& mapTag)
   {
      const int32 index = UTATProjectSettings::Get().Maps.IndexOfByPredicate([&mapTag](const FTATMapSettings& map) { return map.MapTag == mapTag; });
      if (index >= 0)
      {
         return index;
      }
      return NullOpt;
   }

   FGameplayTag DecodeMap(int32 encodedMap)
   {
      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      if (settings.Maps.IsValidIndex(encodedMap))
      {
         return settings.Maps[encodedMap].MapTag;
      }

      return FGameplayTag();
   }
}


