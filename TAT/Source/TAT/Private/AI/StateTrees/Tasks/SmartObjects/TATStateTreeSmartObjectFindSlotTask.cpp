// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Tasks/SmartObjects/TATStateTreeSmartObjectFindSlotTask.h"

// ue
#include "SmartObjectDefinition.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Annotations/SmartObjectSlotLinkAnnotation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeSmartObjectFindSlotTask)

FTATStateTreeSmartObjectFindSlotTask::FTATStateTreeSmartObjectFindSlotTask()
{
   // No tick needed.
   bShouldCallTick = false;
   // No need to update bound properties after enter state.
   bShouldCopyBoundPropertiesOnTick = false;
   bShouldCopyBoundPropertiesOnExitState = false;
}

bool FTATStateTreeSmartObjectFindSlotTask::Link(FStateTreeLinker& linker)
{
   linker.LinkExternalData(_SmartObjectSubsystemHandle);
   return true;
}

EStateTreeRunStatus FTATStateTreeSmartObjectFindSlotTask::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
   const USmartObjectSubsystem& smartObjectSubsystem = context.GetExternalData(_SmartObjectSubsystemHandle);
	FInstanceDataType& instanceData = context.GetInstanceData(*this);

	if (!instanceData.ReferenceSlot.IsValid())
	{
		UE_VLOG_UELOG(context.GetOwner(), LogStateTree, Error, TEXT("[TATStateTreeSmartObjectFindSlotTask] Expected valid ReferenceSlot handle."));
		return EStateTreeRunStatus::Failed;
	}

	instanceData.ResultSlot = FSmartObjectSlotHandle();

	// Acquire the target slot based on a link
	instanceData.ResultSlot = FSmartObjectSlotHandle();
		
	const FSmartObjectSlotView slotView = smartObjectSubsystem.GetSlotView(instanceData.ReferenceSlot);
	const FSmartObjectSlotDefinition& slotDefinition = slotView.GetDefinition();

	for (const FSmartObjectDefinitionDataProxy& dataProxy : slotDefinition.DefinitionData)
	{
		if (const FSmartObjectSlotLinkAnnotation* link = dataProxy.Data.GetPtr<FSmartObjectSlotLinkAnnotation>())
		{
			if (link->Tag.MatchesTag(_FindByTag))
			{
				TArray<FSmartObjectSlotHandle> slots;
				smartObjectSubsystem.GetAllSlots(slotView.GetOwnerRuntimeObject(), slots);

				const int32 slotIndex = link->LinkedSlot.GetIndex();
				if (slots.IsValidIndex(slotIndex))
				{
					instanceData.ResultSlot = slots[slotIndex];
					break;
				}
			}
		}
	}

	return instanceData.ResultSlot.IsValid() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}
