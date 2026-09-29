// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_WaitInputWithTimeout.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitInputWithTimeout)

namespace WaitInputWithTimeoutImpl
{
   static constexpr EAbilityGenericReplicatedEvent::Type sGenericConfirmReleased = EAbilityGenericReplicatedEvent::GameCustom1;
   static constexpr EAbilityGenericReplicatedEvent::Type sGenericCancelReleased = EAbilityGenericReplicatedEvent::GameCustom2;
}
   
// Note on the VectorPayload:
// * bTimedOut is packed into the Y field to avoid taking up another event channel
//   that might conflict with another task
// * ElapsedTime is sent via the X field if bUseClientTime (this will be quantized
//   over the network, and perhaps the client should also do this locally so that
//   it matches
//

UAbilityTask_WaitInputWithTimeout::UAbilityTask_WaitInputWithTimeout(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   _startTime = 0.f;
   _testInitialState = false;
   _useClientTime = false;
}

UAbilityTask_WaitInputWithTimeout* UAbilityTask_WaitInputWithTimeout::WaitInputPressWithTimeout(
   UGameplayAbility* owningAbility, float timeoutSeconds, bool testAlreadyPressed, bool useClientTime)
{
   UAbilityTask_WaitInputWithTimeout* task = NewAbilityTask<UAbilityTask_WaitInputWithTimeout>(owningAbility);
   task->_testInitialState = testAlreadyPressed;
   task->_useClientTime = useClientTime;
   task->_timeoutDuration = timeoutSeconds;
   task->_eventType = EAbilityGenericReplicatedEvent::InputPressed;
   return task;
}

UAbilityTask_WaitInputWithTimeout* UAbilityTask_WaitInputWithTimeout::WaitInputReleaseWithTimeout(
   UGameplayAbility* owningAbility, float timeout, bool testAlreadyReleased, bool useClientTime)
{
   UAbilityTask_WaitInputWithTimeout* task = NewAbilityTask<UAbilityTask_WaitInputWithTimeout>(owningAbility);
   task->_testInitialState = testAlreadyReleased;
   task->_useClientTime = useClientTime;
   task->_timeoutDuration = timeout;
   task->_eventType = EAbilityGenericReplicatedEvent::InputReleased;
   return task;
}

UAbilityTask_WaitInputWithTimeout* UAbilityTask_WaitInputWithTimeout::WaitConfirmPressWithTimeout(
   UGameplayAbility* owningAbility, float timeoutSeconds, bool useClientTime)
{
   UAbilityTask_WaitInputWithTimeout* task = NewAbilityTask<UAbilityTask_WaitInputWithTimeout>(owningAbility);
   task->_useClientTime = useClientTime;
   task->_timeoutDuration = timeoutSeconds;
   task->_eventType = EAbilityGenericReplicatedEvent::GenericConfirm;
   return task;
}

UAbilityTask_WaitInputWithTimeout* UAbilityTask_WaitInputWithTimeout::WaitConfirmReleaseWithTimeout(
   UGameplayAbility* owningAbility, float timeoutSeconds, bool useClientTime)
{
   UAbilityTask_WaitInputWithTimeout* task = NewAbilityTask<UAbilityTask_WaitInputWithTimeout>(owningAbility);
   task->_useClientTime = useClientTime;
   task->_timeoutDuration = timeoutSeconds;
   task->_eventType = WaitInputWithTimeoutImpl::sGenericConfirmReleased;
   return task;
}

UAbilityTask_WaitInputWithTimeout* UAbilityTask_WaitInputWithTimeout::WaitCancelPressWithTimeout(
   UGameplayAbility* owningAbility, float timeoutSeconds, bool useClientTime)
{
   UAbilityTask_WaitInputWithTimeout* task = NewAbilityTask<UAbilityTask_WaitInputWithTimeout>(owningAbility);
   task->_useClientTime = useClientTime;
   task->_timeoutDuration = timeoutSeconds;
   task->_eventType = EAbilityGenericReplicatedEvent::GenericCancel;
   return task;
}

UAbilityTask_WaitInputWithTimeout* UAbilityTask_WaitInputWithTimeout::WaitCancelReleaseWithTimeout(
   UGameplayAbility* owningAbility, float timeoutSeconds, bool useClientTime)
{
   UAbilityTask_WaitInputWithTimeout* task = NewAbilityTask<UAbilityTask_WaitInputWithTimeout>(owningAbility);
   task->_useClientTime = useClientTime;
   task->_timeoutDuration = timeoutSeconds;
   task->_eventType = WaitInputWithTimeoutImpl::sGenericCancelReleased;
   return task;
}

void UAbilityTask_WaitInputWithTimeout::Activate()
{
   UOSEAbilitySystemComponent* asc = Cast<UOSEAbilitySystemComponent>(AbilitySystemComponent);
   UWorld* world = GetWorld();
   _startTime = world->GetTimeSeconds();
   if (Ability && asc)
   {
      if (_ShouldCompleteImmediately())
      {
         _OnInputCallback();
         return;
      }

      if (_timeoutDuration > 0 && IsLocallyControlled())
      {
         world->GetTimerManager().SetTimer(_timerHandle, this, &UAbilityTask_WaitInputWithTimeout::_OnTimeFinish, _timeoutDuration, false);
      }
      else if (_timeoutDuration == 0 && IsLocallyControlled())
      {
         _OnTimeFinish();
         return;
      }

      _delegateHandle = AbilitySystemComponent->AbilityReplicatedEventDelegate(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
      if (IsLocallyControlled())
      {
         // Local confirm/cancel uses separate delegate
         if (_eventType == EAbilityGenericReplicatedEvent::GenericConfirm)
         {
            asc->GenericLocalConfirmCallbacks.AddDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
         }
         else if (_eventType == WaitInputWithTimeoutImpl::sGenericConfirmReleased)
         {
            asc->GenericLocalConfirmReleasedCallbacks.AddDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
         }
         else if (_eventType == EAbilityGenericReplicatedEvent::GenericCancel)
         {
            asc->GenericLocalCancelCallbacks.AddDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
         }
         else if (_eventType == WaitInputWithTimeoutImpl::sGenericCancelReleased)
         {
            asc->GenericLocalCancelReleasedCallbacks.AddDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
         }
      }
      else if (IsForRemoteClient())
      {
         if (!AbilitySystemComponent->CallReplicatedEventDelegateIfSet(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey()))
         {
            SetWaitingOnRemotePlayerData();
         }
      }
   }
}

bool UAbilityTask_WaitInputWithTimeout::_ShouldCompleteImmediately() const
{
   if (!_testInitialState || !IsLocallyControlled())
      return false;

   FGameplayAbilitySpec* spec = Ability->GetCurrentAbilitySpec();
   if (spec == nullptr)
      return false;

   return (_eventType == EAbilityGenericReplicatedEvent::InputReleased && !spec->InputPressed) ||
          (_eventType == EAbilityGenericReplicatedEvent::InputPressed && spec->InputPressed);
}

void UAbilityTask_WaitInputWithTimeout::_OnInputCallback()
{
   if (!Ability || !AbilitySystemComponent.IsValid())
   {
      return;
   }

   float elapsedTime = GetWorld()->GetTimeSeconds() - _startTime;
   bool timedOut = false;

   if (IsForRemoteClient())
   {
      // Timed-out-ness and maybe client time may be packed into the vector payload
      // (see comment at top of file)
      FAbilityReplicatedData replicatedData = AbilitySystemComponent->GetReplicatedDataOfGenericReplicatedEvent(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey());
      checkf(replicatedData.bTriggered, TEXT("replicatedData.bTriggered in %s"), Ability ? *Ability->GetName() : TEXT("NO-ABILITY"));
      if (_useClientTime)
      {
         elapsedTime = replicatedData.VectorPayload.X;
      }
      timedOut = replicatedData.VectorPayload.Y != 0;
   }

   _HandleComplete(elapsedTime, timedOut);
}

void UAbilityTask_WaitInputWithTimeout::OnDestroy(bool abilityEnded)
{
   // Fire OnAbilityEnded if the task is ending because the ability ended, and it has
   // not already fired other events.
   if (abilityEnded && Ability && !_alreadyNotified)
   {
      // not calling ShouldBroadcastAbilityTaskDelegates, so be very careful
      OnAbilityEnded.Broadcast();
   }

   Super::OnDestroy(abilityEnded);
}

void UAbilityTask_WaitInputWithTimeout::_OnTimeFinish()
{
   if (!Ability || !AbilitySystemComponent.IsValid())
   {
      return;
   }

   _HandleComplete(_timeoutDuration, true);
}

// Called either:
// * When locally-controlled times out
// * When locally-controlled gets input event
// * When non-local get replicated event (from one of first two)
void UAbilityTask_WaitInputWithTimeout::_HandleComplete(float elapsedTime, bool timedOut)
{
   UOSEAbilitySystemComponent* asc = CastChecked<UOSEAbilitySystemComponent>(AbilitySystemComponent);

   GetWorld()->GetTimerManager().ClearTimer(_timerHandle);

   AbilitySystemComponent->AbilityReplicatedEventDelegate(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(_delegateHandle);

   if (IsLocallyControlled())
   {
      // Unbind local delegates if needed
      if (_eventType == EAbilityGenericReplicatedEvent::GenericConfirm)
      {
         asc->GenericLocalConfirmCallbacks.RemoveDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
      }
      else if (_eventType == WaitInputWithTimeoutImpl::sGenericConfirmReleased)
      {
         asc->GenericLocalConfirmReleasedCallbacks.RemoveDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
      }
      else if (_eventType == EAbilityGenericReplicatedEvent::GenericCancel)
      {
         asc->GenericLocalCancelCallbacks.RemoveDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
      }
      else if (_eventType == WaitInputWithTimeoutImpl::sGenericCancelReleased)
      {
         asc->GenericLocalCancelReleasedCallbacks.RemoveDynamic(this, &UAbilityTask_WaitInputWithTimeout::_OnInputCallback);
      }
   }

   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   if (IsPredictingClient())
   {
      // Tell the server about this
      // Q: is this an interesting optimization to only send payload if there is one?
      if(_useClientTime || timedOut)
      {
         // TODO: pre-quantize local time so they match?
         // (see comment at top of file)
         FVector_NetQuantize100 payload(elapsedTime, timedOut ? 1 : 0, 0);
         AbilitySystemComponent->ServerSetReplicatedEventWithPayload(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey, payload);
      }
      else
      {
         AbilitySystemComponent->ServerSetReplicatedEvent(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
      }
   }
   else
   {
      AbilitySystemComponent->ConsumeGenericReplicatedEvent(_eventType, GetAbilitySpecHandle(), GetActivationPredictionKey());
   }

   // We are done. Kill us so we don't keep getting broadcast messages
   if (ShouldBroadcastAbilityTaskDelegates())
   {
      if(timedOut)
      {
         OnTimeOut.Broadcast(_timeoutDuration, timedOut);
      }
      else
      {
         OnInput.Broadcast(elapsedTime, timedOut);
      }
      _alreadyNotified = true;
   }
   EndTask();
}

