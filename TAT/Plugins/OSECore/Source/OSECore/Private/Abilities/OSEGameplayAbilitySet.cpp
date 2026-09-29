// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSEGameplayAbilitySet.h"
#include "Abilities/OSEGameplayAbility.h"
#include "Abilities/OSEAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameplayAbilitySet)

TArray<FGameplayAbilitySpecHandle> UOSEGameplayAbilitySet::GiveAbilities(IAbilitySystemInterface* abilitySystemInterface) const
{
   TArray<FGameplayAbilitySpecHandle> result;

   if (abilitySystemInterface == nullptr)
      return result;

   return GiveAbilities(abilitySystemInterface->GetAbilitySystemComponent(), Cast<UObject>(abilitySystemInterface));
}

TArray<FGameplayAbilitySpecHandle> UOSEGameplayAbilitySet::GiveAbilities(UAbilitySystemComponent* origAbilitySystemComponent, UObject* ownerObj) const
{
   TArray<FGameplayAbilitySpecHandle> result;

   UOSEAbilitySystemComponent* abilitySystemComponent = Cast<UOSEAbilitySystemComponent>(origAbilitySystemComponent);

   if (abilitySystemComponent == nullptr || !abilitySystemComponent->IsOwnerActorAuthoritative())
      return result;

   for (const FOSEAbilityBindInfo& bindInfo : Abilities)
   {
      if (bindInfo.AbilityClass != nullptr)
      {
         if (!bindInfo.Requirements.IsMet(abilitySystemComponent))
         {
            continue;
         }

         const int32 inputBinding = bindInfo.bBindToInput ? (int32)bindInfo.InputCommand : INDEX_NONE;
         FGameplayAbilitySpecHandle specHandle = abilitySystemComponent->GiveAbility(
            FGameplayAbilitySpec(bindInfo.AbilityClass, 1, inputBinding, ownerObj));

         if (specHandle.IsValid())
         {
            result.Add(specHandle);
         }
      }
   }

   return result;
}

bool UOSEGameplayAbilitySet::IsReady(const UAbilitySystemComponent* abilitySystemComponent, const UObject* ownerObj) const
{
   const UOSEAbilitySystemComponent* oseAsc = Cast<UOSEAbilitySystemComponent>(abilitySystemComponent);

   if (!oseAsc)
      return false;

   // We're only ready if we're locally controlled. Will be true in single player.
   if (!oseAsc->IsLocallyControlled())
      return false;

   // Ensure all granted abilities are ready
   for (const FOSEAbilityBindInfo& bindInfo : Abilities)
   {
      //if no ability is bound, ignore it
      if (!bindInfo.AbilityClass)
      {
         continue;
      }
      // ignore any abilities whos requirements we don't meet
      if (!bindInfo.Requirements.IsMet(oseAsc))
      {
         continue;
      }

      // Look for our specific ability spec; this ensures all required data (source object)
      // has been set (or replicated)
      const FGameplayAbilitySpec* spec = oseAsc->FindAbilitySpec(bindInfo.AbilityClass, ownerObj);
      if (!spec)
         return false;
   }

   // all ready!
   return true;
}

void UOSEGameplayAbilitySet::ForEachAbility(TFunctionRef<void(const FOSEAbilityBindInfo&)> handler) const
{
   for (const FOSEAbilityBindInfo& bindInfo : Abilities)
   {
      handler(bindInfo);
   }
}

