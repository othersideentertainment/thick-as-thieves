// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/SmartObjects/TATStateTreeSmartObjectSendSlotEventTask.h"

// ue
#include "SmartObjectSubsystem.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Annotations/SmartObjectSlotLinkAnnotation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeSmartObjectSendSlotEventTask)

#define LOCTEXT_NAMESPACE "TATStateTreeSmartObjects"

FTATStateTreeSmartObjectSendSlotEventTask::FTATStateTreeSmartObjectSendSlotEventTask()
{
	// No tick needed.
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

bool FTATStateTreeSmartObjectSendSlotEventTask::Link(FStateTreeLinker& linker)
{
	linker.LinkExternalData(_SmartObjectSubsystemHandle);

	bShouldStateChangeOnReselect = _bShouldTriggerOnReselect;
	// Copy properties on exit state if the event is sent then.
	bShouldCopyBoundPropertiesOnExitState = _Trigger == ETATStateTreeSmartObjectTaskTrigger::OnExitState;

	return true;
}

EDataValidationResult FTATStateTreeSmartObjectSendSlotEventTask::Compile(FStateTreeDataView instanceDataView, TArray<FText>& validationMessages)
{
	EDataValidationResult result = EDataValidationResult::Valid;
	
	if (_EventTag.IsValid() == false && _Payload.IsValid() == false)
	{
		validationMessages.Add(LOCTEXT("MissingEventData", "EventTag and Payload properties are empty, expecting valid tag."));
		result = EDataValidationResult::Invalid;
	}

	return result;
}

EStateTreeRunStatus FTATStateTreeSmartObjectSendSlotEventTask::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	if (_Trigger == ETATStateTreeSmartObjectTaskTrigger::OnEnterState)
	{
		USmartObjectSubsystem& smartObjectSubsystem = context.GetExternalData(_SmartObjectSubsystemHandle);
		const FInstanceDataType& instanceData = context.GetInstanceData(*this);

		if (instanceData.ReferenceSlot.IsValid())
		{
			// Send the event
			smartObjectSubsystem.SendSlotEvent(instanceData.ReferenceSlot, _EventTag, _Payload);
		}
		else
		{
			UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Error, TEXT("[TATStateTreeSmartObjectSendSlotEventTask] Expected valid TargetSlot handle."));
		}
	}

	return EStateTreeRunStatus::Succeeded;
}

void FTATStateTreeSmartObjectSendSlotEventTask::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	const bool bLastStateFailed = transition.CurrentRunStatus == EStateTreeRunStatus::Failed
								|| (_bHandleExternalStopAsFailure &&  transition.CurrentRunStatus == EStateTreeRunStatus::Stopped);
	
	if (_Trigger == ETATStateTreeSmartObjectTaskTrigger::OnExitState
		|| (bLastStateFailed && _Trigger == ETATStateTreeSmartObjectTaskTrigger::OnExitStateFailed)
		|| (bLastStateFailed == false && _Trigger == ETATStateTreeSmartObjectTaskTrigger::OnExitStateSucceeded))
	{
		USmartObjectSubsystem& smartObjectSubsystem = context.GetExternalData(_SmartObjectSubsystemHandle);
		const FInstanceDataType& instanceData = context.GetInstanceData(*this);

		if (instanceData.ReferenceSlot.IsValid())
		{
			// Send the event
			smartObjectSubsystem.SendSlotEvent(instanceData.ReferenceSlot, _EventTag, _Payload);
		}
		else
		{
			UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Error, TEXT("[TATStateTreeSmartObjectSendSlotEventTask] Expected valid TargetSlot handle."));
		}
	}
}

#undef LOCTEXT_NAMESPACE
