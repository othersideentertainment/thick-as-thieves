// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_WaitExternalTargetData.h"

#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitExternalTargetData)


UAbilityTask_WaitExternalTargetData* UAbilityTask_WaitExternalTargetData::WaitProvidedTargetData(
   UGameplayAbility* owningAbility, FName taskInstanceName, const FGameplayAbilityTargetDataHandle& handle)
{
   UAbilityTask_WaitExternalTargetData* myObj = NewAbilityTask<UAbilityTask_WaitExternalTargetData>(owningAbility, taskInstanceName);      //Register for task list here, providing a given FName as a key
   myObj->_providedTargetData = handle;
   myObj->_targetDataProvided = true;
   return myObj;
}

void UAbilityTask_WaitExternalTargetData::Activate()
{
   if(_targetDataProvided)
   {
      TryProvideTarget(_providedTargetData);
   }

   if (IsForRemoteClient())
   {
      RegisterTargetDataCallbacks();
   }
}

void UAbilityTask_WaitExternalTargetData::RegisterTargetDataCallbacks()
{
   if (!ensure(IsValid(this) == true))
   {
      return;
   }
   check(Ability);

   // If not locally controlled (server for remote client), see if TargetData was already sent
   // else register callback for when it does get here.
   FGameplayAbilitySpecHandle specHandle = GetAbilitySpecHandle();
   FPredictionKey activationPredictionKey = GetActivationPredictionKey();

   AbilitySystemComponent->AbilityTargetDataSetDelegate(specHandle, activationPredictionKey).AddUObject(this, &UAbilityTask_WaitExternalTargetData::OnTargetDataReplicatedCallback);
   AbilitySystemComponent->AbilityTargetDataCancelledDelegate(specHandle, activationPredictionKey).AddUObject(this, &UAbilityTask_WaitExternalTargetData::OnTargetDataReplicatedCancelledCallback);

   AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(specHandle, activationPredictionKey);

   SetWaitingOnRemotePlayerData();
}

/** Valid TargetData was replicated to use (we are server, was sent from client) */
void UAbilityTask_WaitExternalTargetData::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& data, FGameplayTag activationTag)
{
   check(AbilitySystemComponent.IsValid());

   FGameplayAbilityTargetDataHandle mutableData = data;
   AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());

   if (ShouldBroadcastAbilityTaskDelegates())
   {
      ValidData.Broadcast(mutableData);
   }

   EndTask();
}

/** Client canceled this Targeting Task (we are the server) */
void UAbilityTask_WaitExternalTargetData::OnTargetDataReplicatedCancelledCallback()
{
   check(AbilitySystemComponent.IsValid());
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      Cancelled.Broadcast(FGameplayAbilityTargetDataHandle());
   }
   EndTask();
}

void UAbilityTask_WaitExternalTargetData::TryProvideTarget(const FGameplayAbilityTargetDataHandle& data)
{
   // if we should not produce a local target
   if (!ShouldProduceLocalTarget()) return;

   // TODO: does a cancel even make sense here, since it is always being produced?
   if (data.IsValid(0))
   {
      OnLocalTargetDataReady(data);
   }
   else
   {
      OnLocalTargetDataCancelled(data);
   }
}

// Locally-produced target data has come in
void UAbilityTask_WaitExternalTargetData::OnLocalTargetDataReady(const FGameplayAbilityTargetDataHandle& data)
{
   check(AbilitySystemComponent.IsValid());
   if (!Ability)
   {
      return;
   }

   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   const FGameplayAbilityActorInfo* info = Ability->GetCurrentActorInfo();
   if (IsPredictingClient())
   {
      FGameplayTag applicationTag; // Fixme: where would this be useful?
      AbilitySystemComponent->CallServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(), data, applicationTag, AbilitySystemComponent->ScopedPredictionKey);
   }

   if (ShouldBroadcastAbilityTaskDelegates())
   {
      ValidData.Broadcast(data);
   }

   EndTask();
}

// The target data has been cancelled locally
void UAbilityTask_WaitExternalTargetData::OnLocalTargetDataCancelled(const FGameplayAbilityTargetDataHandle& data)
{
   check(AbilitySystemComponent.IsValid());

   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   if (IsPredictingClient())
   {
      AbilitySystemComponent->ServerSetReplicatedTargetDataCancelled(GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
   }
   Cancelled.Broadcast(data);
   EndTask();
}

/** Called when the ability is asked to confirm from an outside node. What this means depends on the individual task. By default, this does nothing other than ending if bEndTask is true. */
void UAbilityTask_WaitExternalTargetData::ExternalCancel()
{
   check(AbilitySystemComponent.IsValid());
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      Cancelled.Broadcast(FGameplayAbilityTargetDataHandle());
   }
   Super::ExternalCancel();
}

bool UAbilityTask_WaitExternalTargetData::ShouldProduceLocalTarget() const
{
   // This could in future include options about whether the server should produce the target itself
   return IsLocallyControlled();
}

