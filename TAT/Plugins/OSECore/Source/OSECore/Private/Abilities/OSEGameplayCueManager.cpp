// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/OSEGameplayCueManager.h"

// ue5
#include "AbilitySystemLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayCueManager)

static TAutoConsoleVariable<float> CVarMinPeriodForCueExecution(
   TEXT("OSE.GameplayEffect.MinPeriodForCueExecution"),
   1.f,
   TEXT("The minimum period length that gameplay cues will be executed for periodic gameplay cue execution"),
   ECVF_Default
);

bool UOSEGameplayCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
{
#if WITH_EDITOR
   // Async loading gameplay cues on startup adds a fair amount of time
   // to commandlet execution with little value. This does not affect
   // the mechanism it uses to force them to be cooked.
   if (IsRunningCommandlet())
   {
      return false;
   }
#endif

   // TODO: figure out what we want to for normal flow

   return Super::ShouldAsyncLoadRuntimeObjectLibraries();
}

void UOSEGameplayCueManager::InvokeGameplayCueExecuted_FromSpec(UAbilitySystemComponent* owningComponent, const FGameplayEffectSpec& spec, FPredictionKey predictionKey)
{
   // Copy early out from super
   if (spec.Def->GameplayCues.Num() == 0)
   {
      // This spec doesn't have any GCs, so early out
      ABILITY_LOG(Verbose, TEXT("No GCs in this Spec, so early out: %s"), *spec.Def->GetName());
      return;
   }

   // Skip executing gameplay cues for periodic executions that are short enough, to avoid spamming RPCs where we only
   // want gameplay cues for the start/stop of a periodic effect
   // TODO: Possibly allow individual effects to opt/in out if needed
   if (spec.GetPeriod() > FGameplayEffectConstants::NO_PERIOD && spec.GetPeriod() < CVarMinPeriodForCueExecution.GetValueOnGameThread())
   {
      ABILITY_LOG(Verbose, TEXT("Skipping gameplay cue for short periodic execution (period %f < %f): %s"), spec.GetPeriod(), CVarMinPeriodForCueExecution.GetValueOnGameThread(), *spec.Def->GetName());
      return;
   }

   Super::InvokeGameplayCueExecuted_FromSpec(owningComponent, spec, predictionKey);
}
