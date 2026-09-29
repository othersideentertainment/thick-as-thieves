// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_LocalInputReleased.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_LocalInputReleased)

// Constructor
UAbilityTask_LocalInputReleased::UAbilityTask_LocalInputReleased(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
}

// Waits for local input released until the time expires
UAbilityTask_LocalInputReleased* UAbilityTask_LocalInputReleased::WaitForLocalInputReleased(UGameplayAbility* owningAbility, float duration /* = 0.05f */)
{
   UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Duration(duration);

   UAbilityTask_LocalInputReleased* myTask = NewAbilityTask<UAbilityTask_LocalInputReleased>(owningAbility);
   myTask->_duration = FMath::Max(duration, KINDA_SMALL_NUMBER); // Avoid negative or divide-by-zero cases
   myTask->_elapsed = 0.0f;
   myTask->_hasTimedOut = false;
   return myTask;
}

// Called to trigger the actual task once the delegates have been set up
void UAbilityTask_LocalInputReleased::Activate()
{
   Super::Activate();

   if (IsLocallyControlled())
   {
      FGameplayAbilitySpec* spec = Ability->GetCurrentAbilitySpec();
      if (spec && !spec->InputPressed)
      {
         OnReleasedCallback();
         return;
      }
   }

   _delegateHandle.Reset();
   _delegateHandle = GetEventDelegate(EAbilityGenericReplicatedEvent::InputReleased).AddUObject(this, &UAbilityTask_LocalInputReleased::OnReleasedCallback);
};

void UAbilityTask_LocalInputReleased::OnReleasedCallback()
{
   GetEventDelegate(EAbilityGenericReplicatedEvent::InputReleased).Remove(_delegateHandle);
   _delegateHandle.Reset();

   ProcessReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, OnReleased);
}

