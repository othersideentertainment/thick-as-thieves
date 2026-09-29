// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_EventSync.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_EventSync)

// Constructor
UAbilityTask_EventSync::UAbilityTask_EventSync(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , _duration(KINDA_SMALL_NUMBER)
   , _elapsed(0.0f)
   , _hasTimedOut(false)
{
   bTickingTask = true;
}

// Utility method that returns the event delegate to use
FSimpleMulticastDelegate& UAbilityTask_EventSync::GetEventDelegate(EAbilityGenericReplicatedEvent::Type eventType)
{
   check(Ability != nullptr);
   check(AbilitySystemComponent != nullptr);
   return AbilitySystemComponent->AbilityReplicatedEventDelegate(eventType, GetAbilitySpecHandle(), GetActivationPredictionKey());
}

// Utility method to process the delegate locally. This is called from ProcessDelegateRemote
void UAbilityTask_EventSync::_ProcessDelegateLocal(FEventSyncDelegate localDelegate, float elapsedTime)
{
   if (IsValid(this))
   {
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         localDelegate.Broadcast(elapsedTime);
      }

      EndTask();
   }
}

// Utility method to consume the event, and then process the delegate locally
void UAbilityTask_EventSync::_ProcessDelegateRemote(EAbilityGenericReplicatedEvent::Type eventType, FEventSyncDelegate localDelegate, float elapsedTime)
{
   if (AbilitySystemComponent.IsValid())
   {
      AbilitySystemComponent->ConsumeGenericReplicatedEvent(eventType, GetAbilitySpecHandle(), GetActivationPredictionKey());
   }

   _ProcessDelegateLocal(localDelegate, elapsedTime);
}

// Processes the generic replicated event. Fires the appropriate delegate and handles local, remote, and predicted network models
void UAbilityTask_EventSync::ProcessReplicatedEvent(EAbilityGenericReplicatedEvent::Type eventType, FEventSyncDelegate localDelegate, float elapsedTime)
{
   FScopedPredictionWindow scopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());

   if (AbilitySystemComponent.IsValid())
   {
      auto delegateLocal = FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UAbilityTask_EventSync::_ProcessDelegateLocal, localDelegate, elapsedTime);
      auto delegateRemote = FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UAbilityTask_EventSync::_ProcessDelegateRemote, eventType, localDelegate, elapsedTime);

      if (IsPredictingClient())
      {
         // We're a predicting client: We're not the authority, we're locally controlled, and the
         // net execution model is client predicted OR server initiated.
         //
         // Send the event to the server and execute it locally.
         AbilitySystemComponent->ServerSetReplicatedEvent(eventType, GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
         delegateLocal.Execute();
      }
      else if (IsForRemoteClient())
      {
         // We're the authority, but not locally controlled: a server executing this
         // ability task for a remote client.
         //
         // This method will execute the delegate if the event has already replicated
         // from client to server. Otherwise, it will add the delegate and wait. When
         // the event is replicated from the client, the delegate will be executed.
         CallOrAddReplicatedDelegate(eventType, delegateRemote);
      }
      else
      {
         // TODO: Do we need to handle additional cases here? For example, a simulated
         // task? Not locally controlled and not authority? No prediction, etc.?
         //
         // We're local; just execute it directly.
         delegateLocal.Execute();
      }
   }
}

// Called to trigger the actual task once the delegates have been set up
void UAbilityTask_EventSync::Activate()
{
   Super::Activate();

   _elapsed = 0.0f;
   _hasTimedOut = false;
};

// Tick function for this task, if bTickingTask == true
void UAbilityTask_EventSync::TickTask(float deltaTime)
{
   if (_hasTimedOut)
      return;

   _elapsed += deltaTime;

   // Time out if time has expired
   if (_elapsed >= _duration)
   {
      _hasTimedOut = true;
      ProcessReplicatedEvent(EAbilityGenericReplicatedEvent::GenericSignalFromClient, OnTimedOut);
   }
}

