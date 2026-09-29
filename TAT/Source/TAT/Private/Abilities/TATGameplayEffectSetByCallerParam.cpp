// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayEffectSetByCallerParam.h"

// ue
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayEffectSetByCallerParam)

// static
FActiveGameplayEffectHandle FTATGameplayEffectSetByCallerParam::ApplyGameplayEffectWithParams(
   UAbilitySystemComponent* asc,
   TSubclassOf<UGameplayEffect> gameplayEffect,
   TConstArrayView<FTATGameplayEffectSetByCallerParam> setByCallerParams,
   float effectLevel,
   const FGameplayEffectContextHandle& effectContext,
   FPredictionKey predictionKey)
{
   if (asc != nullptr && gameplayEffect)
   {
      ensure(asc->IsOwnerActorAuthoritative());
      FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(gameplayEffect, effectLevel, effectContext);
      if (specHandle.IsValid())
      {
         if (FGameplayEffectSpec* spec = specHandle.Data.Get())
         {
            for (const FTATGameplayEffectSetByCallerParam& param : setByCallerParams)
            {
               if (param.Tag.IsValid())
               {
                  spec->SetSetByCallerMagnitude(param.Tag, param.Value);
               }
            }
         }

         return asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get(), predictionKey);
      }
   }

   return FActiveGameplayEffectHandle{};
}

