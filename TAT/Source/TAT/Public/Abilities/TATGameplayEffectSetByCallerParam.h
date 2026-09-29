// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffectTypes.h"
#include "GameplayPrediction.h"

#include "TATGameplayEffectSetByCallerParam.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct TAT_API FTATGameplayEffectSetByCallerParam
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Gameplay Effect Set-By-Caller Parameter", Meta = (Categories = "SetByCaller"))
   FGameplayTag Tag;

   UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Gameplay Effect Set-By-Caller Parameter")
   float Value = 0.0f;

   static FActiveGameplayEffectHandle ApplyGameplayEffectWithParams(
      UAbilitySystemComponent* asc,
      TSubclassOf<UGameplayEffect> gameplayEffect,
      TConstArrayView<FTATGameplayEffectSetByCallerParam> setByCallerParams,
      float effectLevel = 1.0f,
      const FGameplayEffectContextHandle& effectContext = FGameplayEffectContextHandle{},
      FPredictionKey predictionKey = FPredictionKey{});
};

