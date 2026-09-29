// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_WaitConfirmReleased.h"
#include "AbilitySystemComponent.h"
#include "Abilities/OSEAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitConfirmReleased)

//TODO: Would have to modify engine file to change this - also abilities might need to use this in the future
static const EAbilityGenericReplicatedEvent::Type GenericConfirmReleased = EAbilityGenericReplicatedEvent::GameCustom1;

UAbilityTask_WaitConfirmReleased::UAbilityTask_WaitConfirmReleased(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   _registeredCallbacks = false;
}

void UAbilityTask_WaitConfirmReleased::OnConfirmReleasedCallback()
{
   if (AbilitySystemComponent.IsValid())
   {
      AbilitySystemComponent->ConsumeGenericReplicatedEvent(GenericConfirmReleased, GetAbilitySpecHandle(), GetActivationPredictionKey());
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         OnConfirmReleased.Broadcast();
      }
      EndTask();
   }
}

void UAbilityTask_WaitConfirmReleased::OnLocalConfirmReleasedCallback()
{
   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   if (AbilitySystemComponent.IsValid() && IsPredictingClient())
   {
      AbilitySystemComponent->ServerSetReplicatedEvent(GenericConfirmReleased, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
   }
   OnConfirmReleasedCallback();
}

UAbilityTask_WaitConfirmReleased* UAbilityTask_WaitConfirmReleased::WaitConfirmReleased(UGameplayAbility* owningAbility)
{
   return NewAbilityTask<UAbilityTask_WaitConfirmReleased>(owningAbility);
}

void UAbilityTask_WaitConfirmReleased::Activate()
{
   UOSEAbilitySystemComponent* oseAbilitySystemComponent = Cast<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   if (oseAbilitySystemComponent)
   {
      const FGameplayAbilityActorInfo* info = Ability->GetCurrentActorInfo();

      if (info->IsLocallyControlled())
      {
         // We have to wait for the callback from the AbilitySystemComponent.
         oseAbilitySystemComponent->GenericLocalConfirmReleasedCallbacks.AddDynamic(this, &UAbilityTask_WaitConfirmReleased::OnLocalConfirmReleasedCallback);   // Tell me if the cancel input is released

         _registeredCallbacks = true;
      }
      else
      {
         if (CallOrAddReplicatedDelegate(GenericConfirmReleased, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UAbilityTask_WaitConfirmReleased::OnConfirmReleasedCallback)))
         {
            // GenericConfirmReleased was already received from the client and we just called OnCancelCallback. The task is done.
            return;
         }
      }
   }
}

void UAbilityTask_WaitConfirmReleased::OnDestroy(bool abilityEnding)
{
   UOSEAbilitySystemComponent* oseAbilitySystemComponent = Cast<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   if (_registeredCallbacks && oseAbilitySystemComponent)
   {
      oseAbilitySystemComponent->GenericLocalConfirmReleasedCallbacks.RemoveDynamic(this, &UAbilityTask_WaitConfirmReleased::OnLocalConfirmReleasedCallback);
   }

   Super::OnDestroy(abilityEnding);
}


