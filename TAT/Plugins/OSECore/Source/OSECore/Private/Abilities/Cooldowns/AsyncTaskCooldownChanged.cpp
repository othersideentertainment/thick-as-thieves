// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Cooldowns/AsyncTaskCooldownChanged.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskCooldownChanged)

UAsyncTaskCooldownChanged* UAsyncTaskCooldownChanged::ListenForCooldownChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayTagContainer cooldownTags, bool checkInitialCooldown)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   UAsyncTaskCooldownChanged* listenForCooldownChange = NewObject<UAsyncTaskCooldownChanged>();
   listenForCooldownChange->_abilitySystemComponent = abilitySystemComponent;
   listenForCooldownChange->_cooldownTags = cooldownTags;
   listenForCooldownChange->_checkInitialCooldown = checkInitialCooldown;

   if (!IsValid(abilitySystemComponent) || cooldownTags.Num() < 1)
   {
      listenForCooldownChange->EndTask();
      return nullptr;
   }

   abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(listenForCooldownChange, &UAsyncTaskCooldownChanged::_OnActiveGameplayEffectAddedCallback);

   TArray<FGameplayTag> cooldownTagArray;
   cooldownTags.GetGameplayTagArray(cooldownTagArray);
   
   for (const FGameplayTag& cooldownTag : cooldownTagArray)
   {
      abilitySystemComponent->RegisterGameplayTagEvent(cooldownTag).AddUObject(listenForCooldownChange, &UAsyncTaskCooldownChanged::_OnCooldownTagChanged);
   }

   return listenForCooldownChange;
}

void UAsyncTaskCooldownChanged::Activate()
{
   Super::Activate();

   // If requested, check if we're already in cooldown when starting this task
   if (_checkInitialCooldown)
   {
      if (_abilitySystemComponent->HasAnyMatchingGameplayTags(_cooldownTags))
      {
         float initialEndTime = 0.0f, initialDuration = 0.0f;
         if (_GetCooldownRemainingForTag(_cooldownTags, initialEndTime, initialDuration))
         {
            for (const FGameplayTag& cooldownTag : _cooldownTags)
            {
               if (_abilitySystemComponent->HasMatchingGameplayTag(cooldownTag))
               {
                  _BroadcastCooldownBeginForTag(cooldownTag, initialEndTime, initialDuration);
               }
            }
         }
      }
   }
}

void UAsyncTaskCooldownChanged::EndTask()
{
   if (IsValid(_abilitySystemComponent))
   {
      _abilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.RemoveAll(this);

      TArray<FGameplayTag> cooldownTagArray;
      _cooldownTags.GetGameplayTagArray(cooldownTagArray);

      for (const FGameplayTag& cooldownTag : cooldownTagArray)
      {
         _abilitySystemComponent->RegisterGameplayTagEvent(cooldownTag).RemoveAll(this);
      }
   }

	SetReadyToDestroy();
	MarkAsGarbage();
}

void UAsyncTaskCooldownChanged::_OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* target, const FGameplayEffectSpec& specApplied, FActiveGameplayEffectHandle activeHandle)
{
   FGameplayTagContainer assetTags;
   specApplied.GetAllAssetTags(assetTags);
   
   FGameplayTagContainer grantedTags;
   specApplied.GetAllGrantedTags(grantedTags);

   float endTime = 0.0f;
   float duration = 0.0f;
   _GetCooldownRemainingForTag(_cooldownTags, endTime, duration);

   for (const FGameplayTag& cooldownTag : _cooldownTags)
   {
      if (assetTags.HasTagExact(cooldownTag) || grantedTags.HasTagExact(cooldownTag))
      {
         _BroadcastCooldownBeginForTag(cooldownTag, endTime, duration);
      }
   }
}

void UAsyncTaskCooldownChanged::_OnCooldownTagChanged(const FGameplayTag cooldownTag, int32 newCount)
{
   if (newCount == 0)
   {
      const bool isNewlyAddedEffect = false;
      OnCooldownEnd.Broadcast(cooldownTag, float(INDEX_NONE), float(INDEX_NONE), isNewlyAddedEffect);
      _activeCooldownEffectTags.Remove(cooldownTag);
   }
}

bool UAsyncTaskCooldownChanged::_GetCooldownRemainingForTag(const FGameplayTagContainer& cooldownTags, float& endTime, float& duration)
{
   if (IsValid(_abilitySystemComponent) && cooldownTags.Num() > 0)
   {
      const FGameplayEffectQuery query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(cooldownTags);
      return _abilitySystemComponent->GetActiveEffectsEndTimeAndDuration(query, endTime, duration);
   }
   return false;
}

void UAsyncTaskCooldownChanged::_BroadcastCooldownBeginForTag(FGameplayTag cooldownTag, float endTime, float duration)
{
   // Notify calling code if this is a cooldown we've already broadcast
   const bool isNewlyAddedEffect = !_activeCooldownEffectTags.Contains(cooldownTag);
   OnCooldownBegin.Broadcast(cooldownTag, endTime, duration, isNewlyAddedEffect);

   if (isNewlyAddedEffect)
   {
      _activeCooldownEffectTags.Add(cooldownTag);
   }
}
