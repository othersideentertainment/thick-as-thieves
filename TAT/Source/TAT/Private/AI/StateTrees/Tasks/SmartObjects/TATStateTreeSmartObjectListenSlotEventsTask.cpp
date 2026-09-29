// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/SmartObjects/TATStateTreeSmartObjectListenSlotEventsTask.h"

// ue
#include "SmartObjectRuntime.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Annotations/SmartObjectSlotLinkAnnotation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeSmartObjectListenSlotEventsTask)

FTATStateTreeSmartObjectListenSlotEventsTask::FTATStateTreeSmartObjectListenSlotEventsTask()
{
   // No tick needed.
   bShouldCallTick = false;
   // No need to update bound properties after enter state.
   bShouldCopyBoundPropertiesOnTick = false;
   bShouldCopyBoundPropertiesOnExitState = false;
}

bool FTATStateTreeSmartObjectListenSlotEventsTask::Link(FStateTreeLinker& linker)
{
   linker.LinkExternalData(_SmartObjectSubsystemHandle);
   return true;
}

EStateTreeRunStatus FTATStateTreeSmartObjectListenSlotEventsTask::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	USmartObjectSubsystem& smartObjectSubsystem = context.GetExternalData(_SmartObjectSubsystemHandle);
	FInstanceDataType& instanceData = context.GetInstanceData(*this);
	
	if (instanceData.ReferenceSlot.IsValid() == false)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Error, TEXT("[FTATStateTreeSmartObjectListenSlotEventsTask] Expected valid TargetSlot handle."));
		return EStateTreeRunStatus::Failed;
	}

	instanceData.OnEventHandle.Reset();

	FOnSmartObjectEvent* onEventDelegate = smartObjectSubsystem.GetSlotEventDelegate(instanceData.ReferenceSlot);
	if (onEventDelegate == nullptr)
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Error, TEXT("[FTATStateTreeSmartObjectListenSlotEventsTask] Expected to find event delegate for the slot."));
		return EStateTreeRunStatus::Failed;
	}

	FStateTreeEventQueue& eventQueue = context.GetMutableEventQueue();

	// Start piping Smart Object slot events into State Tree.
	instanceData.OnEventHandle = onEventDelegate->AddLambda([TargetSlot = instanceData.ReferenceSlot, &eventQueue, Owner = context.GetOwner()](const FSmartObjectEventData& Data)
	{
		if (Data.SlotHandle == TargetSlot && Data.Reason == ESmartObjectChangeReason::OnEvent)
		{
			UE_VLOG_UELOG(Owner, LogStateTree, VeryVerbose, TEXT("[FTATStateTreeSmartObjectListenSlotEventsTask] Listen Slot Events: received %s"), *Data.Tag.ToString());

			eventQueue.SendEvent(Owner, Data.Tag, Data.EventPayload);
		}
	});

	return EStateTreeRunStatus::Running;
}

void FTATStateTreeSmartObjectListenSlotEventsTask::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	USmartObjectSubsystem& smartObjectSubsystem = context.GetExternalData(_SmartObjectSubsystemHandle);
	FInstanceDataType& instanceData = context.GetInstanceData(*this);

	if (instanceData.OnEventHandle.IsValid())
	{
		// Stop listening.
		if (FOnSmartObjectEvent* onEventDelegate = smartObjectSubsystem.GetSlotEventDelegate(instanceData.ReferenceSlot))
		{
			onEventDelegate->Remove(instanceData.OnEventHandle);
		}
	}

	instanceData.OnEventHandle.Reset();
}
