// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Attributes/AsyncTaskDamageTaken.h"
#include "Abilities/Attributes/AttributeBaseSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskDamageTaken)

DEFINE_LOG_CATEGORY(LogAsyncTaskDamageTaken);

UAsyncTaskDamageTaken* UAsyncTaskDamageTaken::ListenForDamageTaken(UObject* worldContextObject, UAbilitySystemComponent* abilitySystemComponent)
{
   // let's not use these in abilities, we want to use ability tasks instead
   check(worldContextObject && Cast<UGameplayAbility>(worldContextObject) == nullptr);

   if (!IsValid(abilitySystemComponent))
   {
      UE_LOG(LogAsyncTaskDamageTaken, Error, TEXT("%s: Cannot instantiate AsyncTaskDamageTaken, null system component!"), *worldContextObject->GetName());
      return nullptr;
   }

   FGameplayAttribute attribute = UAttributeBaseSet::GetHealthAttribute();
   if (!attribute.IsValid())
   {
      UE_LOG(LogAsyncTaskDamageTaken, Error, TEXT("%s: Cannot instantiate AsyncTaskDamageTaken (%s), invalid attribute \"%s\"!"), *worldContextObject->GetName(), *abilitySystemComponent->GetOwner()->GetName(), *attribute.GetName());
      return nullptr;
   }

   UAsyncTaskDamageTaken* waitForAttributeChangedTask = NewObject<UAsyncTaskDamageTaken>();
   waitForAttributeChangedTask->_abilitySystemComponent = abilitySystemComponent;
   waitForAttributeChangedTask->_attributeToListenFor = attribute;

   abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(attribute).AddUObject(waitForAttributeChangedTask, &UAsyncTaskDamageTaken::_OnHealthAttributeChanged);

   return waitForAttributeChangedTask;
}

void UAsyncTaskDamageTaken::EndTask()
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

void UAsyncTaskDamageTaken::_OnHealthAttributeChanged(const FOnAttributeChangeData& data)
{
   check(_abilitySystemComponent);

   FGameplayAttribute maxHealthAttribute = UAttributeBaseSet::GetHealthMaxAttribute();
   const float maxHealth = _abilitySystemComponent->GetNumericAttribute(maxHealthAttribute);

   if (data.NewValue >= 0.0f && 
       data.OldValue <= maxHealth && 
       data.NewValue < data.OldValue)
   {
      const float damageTaken = data.OldValue - data.NewValue;
      OnDamageTaken.Broadcast(data.OldValue, data.NewValue, maxHealth, damageTaken);
   }
}

