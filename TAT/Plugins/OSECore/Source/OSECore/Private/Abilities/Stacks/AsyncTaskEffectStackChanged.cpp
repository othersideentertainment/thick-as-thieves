// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Stacks/AsyncTaskEffectStackChanged.h"

// ue5
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskEffectStackChanged)

UAsyncTaskEffectStackChanged* UAsyncTaskEffectStackChanged::ListenForGameplayEffectStackChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayTag inEffectGameplayTag)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   if (!IsValid(abilitySystemComponent) || !inEffectGameplayTag.IsValid())
   {
      return nullptr;
   }

   UAsyncTaskEffectStackChanged* stackChangeObj = NewObject<UAsyncTaskEffectStackChanged>();
   stackChangeObj->_abilitySystemComponent = abilitySystemComponent;
   stackChangeObj->_effectGameplayTag = inEffectGameplayTag;

   // bind for abilities that come and go
   abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(stackChangeObj, &UAsyncTaskEffectStackChanged::_OnActiveGameplayEffectAddedCallback);
   abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(stackChangeObj, &UAsyncTaskEffectStackChanged::_OnRemoveGameplayEffectCallback);

   return stackChangeObj;
}

void UAsyncTaskEffectStackChanged::Activate()
{
   Super::Activate();

   // find abilities that already exist
   FGameplayTagContainer container;
   container.AddTag(_effectGameplayTag);
   FGameplayEffectQuery query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(container);
   TArray<FActiveGameplayEffectHandle> effectHandles = _abilitySystemComponent->GetActiveEffects(query);
   for (const FActiveGameplayEffectHandle& activeHandle : effectHandles)
   {
      _abilitySystemComponent->OnGameplayEffectStackChangeDelegate(activeHandle)->AddUObject(this, &UAsyncTaskEffectStackChanged::_OnGameplayEffectStackChanged);
      const int currentStackCount = _abilitySystemComponent->GetCurrentStackCount(activeHandle);
      OnGameplayEffectStackChange.Broadcast(_effectGameplayTag, activeHandle, currentStackCount, 0);
   }
}

void UAsyncTaskEffectStackChanged::EndTask()
{
   if (IsValid(_abilitySystemComponent))
   {
      _abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);
      _abilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().RemoveAll(this);
   }

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UAsyncTaskEffectStackChanged::_OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* target, const FGameplayEffectSpec& specApplied, FActiveGameplayEffectHandle activeHandle)
{
   FGameplayTagContainer assetTags;
   specApplied.GetAllAssetTags(assetTags);

   FGameplayTagContainer grantedTags;
   specApplied.GetAllGrantedTags(grantedTags);

   if (assetTags.HasTagExact(_effectGameplayTag) || grantedTags.HasTagExact(_effectGameplayTag))
   {
      _abilitySystemComponent->OnGameplayEffectStackChangeDelegate(activeHandle)->AddUObject(this, &UAsyncTaskEffectStackChanged::_OnGameplayEffectStackChanged);
      const int currentStackCount = _abilitySystemComponent->GetCurrentStackCount(activeHandle);
      OnGameplayEffectStackChange.Broadcast(_effectGameplayTag, activeHandle, currentStackCount, 0);
   }
}

void UAsyncTaskEffectStackChanged::_OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& effectRemoved)
{
   FGameplayTagContainer assetTags;
   effectRemoved.Spec.GetAllAssetTags(assetTags);

   FGameplayTagContainer grantedTags;
   effectRemoved.Spec.GetAllGrantedTags(grantedTags);

   if (assetTags.HasTagExact(_effectGameplayTag) || grantedTags.HasTagExact(_effectGameplayTag))
   {
      OnGameplayEffectStackChange.Broadcast(_effectGameplayTag, effectRemoved.Handle, 0, 1);
   }
}

void UAsyncTaskEffectStackChanged::_OnGameplayEffectStackChanged(FActiveGameplayEffectHandle effectHandle, int32 newStackCount, int32 previousStackCount)
{
   OnGameplayEffectStackChange.Broadcast(_effectGameplayTag, effectHandle, newStackCount, previousStackCount);
}

