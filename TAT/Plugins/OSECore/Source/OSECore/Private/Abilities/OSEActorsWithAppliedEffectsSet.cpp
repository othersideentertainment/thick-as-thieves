// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEActorsWithAppliedEffectsSet.h"

// ue5
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEActorsWithAppliedEffectsSet)

void FOSEActorsWithAppliedEffectsSet::Add(AActor* actor, FActiveGameplayEffectHandle effectHandle)
{
   if (effectHandle.IsValid())
   {
      EffectEntries.Emplace(actor, effectHandle);
   }

}

void FOSEActorsWithAppliedEffectsSet::AddMultiple(AActor* actor, TConstArrayView<FActiveGameplayEffectHandle> effectHandles)
{
   for (const FActiveGameplayEffectHandle& effectHandle : effectHandles)
   {
      if (effectHandle.IsValid())
      {
         EffectEntries.Emplace(actor, effectHandle);
      }
   }
}

bool FOSEActorsWithAppliedEffectsSet::CancelByActor(AActor* actor, int32 stacksToRemove /*= 1*/)
{
   int32 numRemoved = EffectEntries.RemoveAllSwap([actor, stacksToRemove](FOSEActorWithAppliedEffectEntry& entry) {
      if (entry.Actor != actor)
      {
         return false;
      }

      if (UAbilitySystemComponent* asc = entry.AppliedEffectHandle.GetOwningAbilitySystemComponent())
      {
         asc->RemoveActiveGameplayEffect(entry.AppliedEffectHandle, stacksToRemove);
      }

      return true;
   });

   return numRemoved > 0;
}

void FOSEActorsWithAppliedEffectsSet::CancelAll(int32 stacksToRemove /*= 1*/)
{
   for (FOSEActorWithAppliedEffectEntry& entry : EffectEntries)
   {
      if (UAbilitySystemComponent* asc = entry.AppliedEffectHandle.GetOwningAbilitySystemComponent())
      {
         asc->RemoveActiveGameplayEffect(entry.AppliedEffectHandle, stacksToRemove);
      }
   }
   EffectEntries.Reset();
}
