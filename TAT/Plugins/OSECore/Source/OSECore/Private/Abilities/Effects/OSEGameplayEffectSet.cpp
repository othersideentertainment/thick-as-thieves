// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Effects/OSEGameplayEffectSet.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// ue5
#include "AbilitySystemInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayEffectSet)

DEFINE_LOG_CATEGORY_STATIC(LogOSEGameplayEffectSet, Log, All)

TArray<FActiveGameplayEffectHandle> UOSEGameplayEffectSet::ApplyEffects(IAbilitySystemInterface* abilitySystemInterface) const
{
   if (abilitySystemInterface == nullptr)
      return TArray<FActiveGameplayEffectHandle>();

   return ApplyEffects(abilitySystemInterface->GetAbilitySystemComponent());
}

TArray<FActiveGameplayEffectHandle> UOSEGameplayEffectSet::ApplyEffects(UAbilitySystemComponent* origAbilitySystemComponent) const
{
   TArray<FActiveGameplayEffectHandle> result;

   UOSEAbilitySystemComponent* abilitySystemComponent = Cast<UOSEAbilitySystemComponent>(origAbilitySystemComponent);
   if (abilitySystemComponent == nullptr || !abilitySystemComponent->IsOwnerActorAuthoritative())
      return result;

   FGameplayEffectContextHandle effectContext = abilitySystemComponent->MakeEffectContext();
   if (!effectContext.IsValid())
      return result;

   result.Reserve(EffectEntries.Num());

   for (const FOSEGameplayEffectSetEntry& entry : EffectEntries)
   {
      if (!IsValid(entry.Effect))
         continue;

      float effectLevel = UGameplayEffect::INVALID_LEVEL;
      if (entry.UseRequirements)
      {
         // Get the upgrade level and don't apply this effect if the current level is too low
         constexpr int32 fallbackUpgradeLevel = 0;
         const int32 upgradeLevel = entry.Requirements.UpgradeTag.IsValid()
            ? abilitySystemComponent->GetUpgradeValue(entry.Requirements.UpgradeTag, fallbackUpgradeLevel)
            : fallbackUpgradeLevel;
         if (upgradeLevel < entry.Requirements.RequiredUpgradeLevel)
         {
            continue;
         }

         if (entry.Requirements.UseUpgradeLevelAsEffectLevel)
         {
            effectLevel = static_cast<float>(upgradeLevel);
         }
      }

      // Use the default object of the specified class
      UGameplayEffect* gameplayEffect = entry.Effect->GetDefaultObject<UGameplayEffect>();
      FGameplayEffectSpec spec(gameplayEffect, effectContext, effectLevel);
      if (entry.UseCustomStackCount)
      {
         spec.SetStackCount( entry.StackCount.Calculate(abilitySystemComponent) );
      }

      FActiveGameplayEffectHandle effectHandle =
         abilitySystemComponent->ApplyGameplayEffectSpecToSelf(spec);

      if (effectHandle.IsValid())
         result.AddUnique(effectHandle);

      UE_LOG(LogOSEGameplayEffectSet, Verbose, TEXT("[%s] Adding effect entry '%s' at level %.2f"), *GetName(), *entry.Effect->GetName(), effectLevel);
   }

   return result;
}

#if WITH_EDITOR
void UOSEGameplayEffectSet::PostLoad()
{
   Super::PostLoad();

   // migrate from old list to new struct
   // TODO: remove this once all affected assets have been re-saved
   if (Effects_DEPRECATED.Num() > 0 && EffectEntries.Num() == 0)
   {
      EffectEntries.Reserve(Effects_DEPRECATED.Num());
      for (TSubclassOf<UGameplayEffect> effectClass : Effects_DEPRECATED)
      {
         EffectEntries.Emplace_GetRef().Effect = effectClass;
      }
      Effects_DEPRECATED.Empty();
   }
}
#endif

int32 FOSEGameplayEffectSetStackCount::Calculate(const UOSEAbilitySystemComponent* asc) const
{
   check(asc);
   const int32 upgradeLevel = asc->GetUpgradeValue(StackCountUpgradeTag);
   return StackCount.AsInteger(upgradeLevel);
}

