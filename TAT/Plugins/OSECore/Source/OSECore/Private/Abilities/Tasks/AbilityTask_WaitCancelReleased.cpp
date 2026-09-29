// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_WaitCancelReleased.h"
#include "AbilitySystemComponent.h"
#include "Abilities/OSEAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitCancelReleased)

//TODO: Would have to modify engine file to change this - also abilities might need to use this in the future
static const EAbilityGenericReplicatedEvent::Type GenericCancelReleased = EAbilityGenericReplicatedEvent::GameCustom2;

UAbilityTask_WaitCancelReleased::UAbilityTask_WaitCancelReleased(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   _registeredCallbacks = false;
}

void UAbilityTask_WaitCancelReleased::OnCancelReleasedCallback()
{
   if (AbilitySystemComponent.IsValid())
   {
      AbilitySystemComponent->ConsumeGenericReplicatedEvent(GenericCancelReleased, GetAbilitySpecHandle(), GetActivationPredictionKey());
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         OnCancelReleased.Broadcast();
      }
      EndTask();
   }
}

void UAbilityTask_WaitCancelReleased::OnLocalCancelReleasedCallback()
{
   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   if (AbilitySystemComponent.IsValid() && IsPredictingClient())
   {
      AbilitySystemComponent->ServerSetReplicatedEvent(GenericCancelReleased, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
   }
   OnCancelReleasedCallback();
}

UAbilityTask_WaitCancelReleased* UAbilityTask_WaitCancelReleased::WaitCancelReleased(UGameplayAbility* owningAbility)
{
   return  NewAbilityTask<UAbilityTask_WaitCancelReleased>(owningAbility);
}

void UAbilityTask_WaitCancelReleased::Activate()
{
   UOSEAbilitySystemComponent* oseAbilitySystemComponent = Cast<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   if (oseAbilitySystemComponent)
   {
      const FGameplayAbilityActorInfo* info = Ability->GetCurrentActorInfo();

      if (info->IsLocallyControlled())
      {
         // We have to wait for the callback from the AbilitySystemComponent.
         oseAbilitySystemComponent->GenericLocalCancelReleasedCallbacks.AddDynamic(this, &UAbilityTask_WaitCancelReleased::OnLocalCancelReleasedCallback);   // Tell me if the cancel input is released

         _registeredCallbacks = true;
      }
      else
      {
         if (CallOrAddReplicatedDelegate(GenericCancelReleased, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UAbilityTask_WaitCancelReleased::OnCancelReleasedCallback)))
         {
            // GenericCancelReleased was already received from the client and we just called OnCancelCallback. The task is done.
            return;
         }
      }
   }
}

void UAbilityTask_WaitCancelReleased::OnDestroy(bool AbilityEnding)
{
   UOSEAbilitySystemComponent* oseAbilitySystemComponent = Cast<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   if (_registeredCallbacks && oseAbilitySystemComponent)
   {
      oseAbilitySystemComponent->GenericLocalCancelReleasedCallbacks.RemoveDynamic(this, &UAbilityTask_WaitCancelReleased::OnLocalCancelReleasedCallback);
   }

   Super::OnDestroy(AbilityEnding);
}

