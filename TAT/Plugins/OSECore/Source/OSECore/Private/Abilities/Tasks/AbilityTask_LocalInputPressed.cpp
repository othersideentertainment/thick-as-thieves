// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_LocalInputPressed.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_LocalInputPressed)

// Constructor
UAbilityTask_LocalInputPressed::UAbilityTask_LocalInputPressed(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

// Waits for local input pressed until the time expires
UAbilityTask_LocalInputPressed* UAbilityTask_LocalInputPressed::WaitForLocalInputPressed(UGameplayAbility* owningAbility, float duration /* = 0.05f */)
{
   UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(duration);

   UAbilityTask_LocalInputPressed* myTask = NewAbilityTask<UAbilityTask_LocalInputPressed>(owningAbility);
   myTask->_duration = FMath::Max(duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
   myTask->_elapsed = 0.0f;
   myTask->_hasTimedOut = false;
   return myTask;
}

// Called to trigger the actual task once the delegates have been set up
void UAbilityTask_LocalInputPressed::Activate()
{
   Super::Activate();

   if (IsLocallyControlled())
   {
      FGameplayAbilitySpec* spec = Ability->GetCurrentAbilitySpec();
      if (spec && spec->InputPressed)
      {
         OnPressedCallback();
         return;
      }
   }

   _delegateHandle.Reset();
   _delegateHandle = GetEventDelegate(EAbilityGenericReplicatedEvent::InputPressed).AddUObject(this, &UAbilityTask_LocalInputPressed::OnPressedCallback);
};

void UAbilityTask_LocalInputPressed::OnPressedCallback()
{
   GetEventDelegate(EAbilityGenericReplicatedEvent::InputPressed).Remove(_delegateHandle);
   _delegateHandle.Reset();

   ProcessReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, OnPressed);
}

