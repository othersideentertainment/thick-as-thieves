// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Alertness/TATAlertnessAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAlertnessAsset)

float FTATBamboozledSettings::GetBamboozledDecayRateMultiplier(int numBamboozledCharacters) const
{
   if (numBamboozledCharacters > 0)
   {
      float totalPercent = static_cast<float>(numBamboozledCharacters) * IncreaseDecayRatePercentPerAI;
      const float cappedPercent = FMath::Clamp(totalPercent, IncreaseDecayRatePercentPerAI, MaxDecayRatePercent);
      return cappedPercent / 100.0f;
   }
   return 1.0f;
}

