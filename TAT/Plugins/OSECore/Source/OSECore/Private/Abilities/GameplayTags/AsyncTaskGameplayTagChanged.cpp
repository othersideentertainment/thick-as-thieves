// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/GameplayTags/AsyncTaskGameplayTagChanged.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskGameplayTagChanged)

DEFINE_LOG_CATEGORY_STATIC(LogAsyncTaskGameplayTagChanged, Log, All);

UAsyncTaskGameplayTagChanged* UAsyncTaskGameplayTagChanged::ListenForGameplayTagChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayTag tag)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   if (!IsValid(abilitySystemComponent))
   {
      UE_LOG(LogAsyncTaskGameplayTagChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskGameplayTagChanged, null system component!"), *worldContextObject->GetName());
      return nullptr;
   }

   if (!tag.IsValid())
   {
      UE_LOG(LogAsyncTaskGameplayTagChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskGameplayTagChanged (%s), invalid tag \"%s\"!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName(), *tag.GetTagName().ToString());
      return nullptr;
   }

   UAsyncTaskGameplayTagChanged* waitForAttributeChangedTask = NewObject<UAsyncTaskGameplayTagChanged>();
   waitForAttributeChangedTask->_abilitySystemComponent = abilitySystemComponent;

   FTagCallbackBindings binding;
   binding.Tag = tag;
   binding.Delegate = abilitySystemComponent->RegisterGameplayTagEvent(tag).AddUObject(waitForAttributeChangedTask, &UAsyncTaskGameplayTagChanged::_OnGameplayTagChanged);
   waitForAttributeChangedTask->_tagsToListenFor.Add(binding);

   return waitForAttributeChangedTask;
}

UAsyncTaskGameplayTagChanged* UAsyncTaskGameplayTagChanged::ListenForGameplayTagsChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, const FGameplayTagContainer& tags)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   if (!IsValid(abilitySystemComponent))
   {
      UE_LOG(LogAsyncTaskGameplayTagChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskGameplayTagChanged, null system component!"), *worldContextObject->GetName());
      return nullptr;
   }

   if (tags.Num() == 0)
   {
      UE_LOG(LogAsyncTaskGameplayTagChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskGameplayTagChanged (%s), no tags passed in!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName());
      return nullptr;
   }

   for (const FGameplayTag& tag : tags)
   {
      if (!tag.IsValid())
      {
         UE_LOG(LogAsyncTaskGameplayTagChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskGameplayTagChanged (%s), invalid tag \"%s\"!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName(), *tag.GetTagName().ToString());
         return nullptr;
      }      
   }

   UAsyncTaskGameplayTagChanged* waitForAttributeChangedTask = NewObject<UAsyncTaskGameplayTagChanged>();
   waitForAttributeChangedTask->_abilitySystemComponent = abilitySystemComponent;

   for (const FGameplayTag& tag : tags)
   {
      FTagCallbackBindings binding;
      binding.Tag = tag;
      binding.Delegate = abilitySystemComponent->RegisterGameplayTagEvent(tag).AddUObject(waitForAttributeChangedTask, &UAsyncTaskGameplayTagChanged::_OnGameplayTagChanged);
      waitForAttributeChangedTask->_tagsToListenFor.Add(binding);
   }

   return waitForAttributeChangedTask;
}

void UAsyncTaskGameplayTagChanged::EndTask()
{
   if (IsValid(_abilitySystemComponent))
   {
      for (const FTagCallbackBindings& binding : _tagsToListenFor)
      {
         _abilitySystemComponent->RegisterGameplayTagEvent(binding.Tag).Remove(binding.Delegate);
      }
   }

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UAsyncTaskGameplayTagChanged::_OnGameplayTagChanged(const FGameplayTag tag, int32 newCount)
{
   bool added = newCount > 0;
   OnGameplayTagChanged.Broadcast(tag, added, newCount);
}

