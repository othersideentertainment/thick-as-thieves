// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Attributes/AsyncTaskAttributeChanged.h"

// ue4
#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskAttributeChanged)

DEFINE_LOG_CATEGORY(LogAsyncTaskAttributeChanged);

UAsyncTaskAttributeChanged* UAsyncTaskAttributeChanged::ListenForAttributeChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, FGameplayAttribute attribute)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);
   
   if (!IsValid(abilitySystemComponent))
   {
      UE_LOG(LogAsyncTaskAttributeChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAttributeChanged, null system component!"), *worldContextObject->GetName());
      return nullptr;
   }

   if (!attribute.IsValid())
   {
      UE_LOG(LogAsyncTaskAttributeChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAttributeChanged (%s), invalid attribute \"%s\"!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName(), *attribute.GetName());
      return nullptr;
   }

   UAsyncTaskAttributeChanged* waitForAttributeChangedTask = NewObject<UAsyncTaskAttributeChanged>();
   waitForAttributeChangedTask->_abilitySystemComponent = abilitySystemComponent;
   waitForAttributeChangedTask->_attributeToListenFor = attribute;

   abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attribute).AddUObject(waitForAttributeChangedTask, &UAsyncTaskAttributeChanged::_OnAttributeChanged);

   return waitForAttributeChangedTask;
}

UAsyncTaskAttributeChanged* UAsyncTaskAttributeChanged::ListenForAttributesChange(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent, TArray<FGameplayAttribute> attributes)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   if (!IsValid(abilitySystemComponent))
   {
      UE_LOG(LogAsyncTaskAttributeChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAttributeChanged, null system component!"), *worldContextObject->GetName());
      return nullptr;
   }

   if (attributes.Num() == 0)
   {
      UE_LOG(LogAsyncTaskAttributeChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAttributeChanged (%s), no attributes passed in!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName());
      return nullptr;
   }

   for (const FGameplayAttribute& attribute : attributes)
   {
      if (!attribute.IsValid())
      {
         UE_LOG(LogAsyncTaskAttributeChanged, Error, TEXT("%s: Cannot instantiate AsyncTaskAttributeChanged (%s), invalid attribute \"%s\"!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName(), *attribute.GetName());
         return nullptr;
      }      
   }

   UAsyncTaskAttributeChanged* waitForAttributeChangedTask = NewObject<UAsyncTaskAttributeChanged>();
   waitForAttributeChangedTask->_abilitySystemComponent = abilitySystemComponent;
   waitForAttributeChangedTask->_attributesToListenFor = attributes;

   for (const FGameplayAttribute& attribute : attributes)
   {
      abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attribute).AddUObject(waitForAttributeChangedTask, &UAsyncTaskAttributeChanged::_OnAttributeChanged);
   }

   return waitForAttributeChangedTask;
}

void UAsyncTaskAttributeChanged::EndTask()
{
   if (IsValid(_abilitySystemComponent))
   {
      _abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(_attributeToListenFor).RemoveAll(this);

      for (const FGameplayAttribute& attribute : _attributesToListenFor)
      {
         _abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attribute).RemoveAll(this);
      }
   }

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UAsyncTaskAttributeChanged::_OnAttributeChanged(const FOnAttributeChangeData& data)
{
   FGameplayTagContainer effectAssetTags;
   if (data.GEModData)
   {
      const FGameplayEffectSpec& spec = data.GEModData->EffectSpec;
      spec.GetAllAssetTags(effectAssetTags);
   }
   const float delta = data.NewValue - data.OldValue;
   OnAttributeChanged.Broadcast(data.Attribute, data.NewValue, data.OldValue, delta, effectAssetTags);
}

