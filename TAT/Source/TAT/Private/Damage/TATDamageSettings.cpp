// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Damage/TATDamageSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDamageSettings)


void UTATDamageSettings::AppendEffectsToPreload(TArray<FSoftObjectPath>& outPathsToLoad) const
{
   for (const TPair<FGameplayTag, TSoftClassPtr<UGameplayEffect>>& pair : DamageEffectByType)
   {
      outPathsToLoad.Add(pair.Value.ToSoftObjectPath());
   }
}
