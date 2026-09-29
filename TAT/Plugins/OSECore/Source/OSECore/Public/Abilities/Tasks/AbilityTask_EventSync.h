// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEAbilityTask.h"
#include "AbilityTask_EventSync.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEventSyncDelegate, float, ElapsedTime);


/// Base ability task with built-in support for syncing generic replicated events.
/// This is used to ensure that output delegates we use are correctly replicated
/// even if the same logic can't run on both client and server.
UCLASS(Abstract)
class OSECORE_API UAbilityTask_EventSync : public UOSEAbilityTask
{
   GENERATED_BODY()
   
   /// Delegate called when the task times out (no input released in the time limit)
   UPROPERTY(BlueprintAssignable)
   FEventSyncDelegate OnTimedOut;

public:

   /// Constructor
   UAbilityTask_EventSync(const FObjectInitializer& objectInitializer);

   /// Tick function for this task, if bTickingTask == true
   virtual void TickTask(float deltaTime) override;

protected:

   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

   /// Utility method that returns the event delegate to use
   FSimpleMulticastDelegate& GetEventDelegate(EAbilityGenericReplicatedEvent::Type event);

   /// Processes the generic replicated event. Fires the appropriate delegate and handles local, remote, and predicted network models
   void ProcessReplicatedEvent(EAbilityGenericReplicatedEvent::Type eventType, FEventSyncDelegate localDelegate, float elapsedTime);
   void ProcessReplicatedEvent(EAbilityGenericReplicatedEvent::Type eventType, FEventSyncDelegate localDelegate) { ProcessReplicatedEvent(eventType, localDelegate, _elapsed); }

   /// Timer variables
   float _duration;
   float _elapsed;
   bool _hasTimedOut;

private:

   /// Utility method to process the delegate locally. This is called from ProcessDelegateRemote
   void _ProcessDelegateLocal(FEventSyncDelegate localDelegate, float elapsedTime);

   /// Utility method to consume the event, and then process the delegate locally
   void _ProcessDelegateRemote(EAbilityGenericReplicatedEvent::Type eventType, FEventSyncDelegate localDelegate, float elapsedTime);
};
